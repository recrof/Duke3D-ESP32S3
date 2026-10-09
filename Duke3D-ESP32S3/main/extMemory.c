/*
 *  External memory for the ESP32-S3 port: see extMemory.h for the layout.
 *
 *  The GRP is never copied anywhere: it is read on demand from the SD card in
 *  16 kB pages, kept in a PSRAM cache with clock (second chance) replacement.
 *  FatFs fast-seek keeps random page reads from walking the FAT chain.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_partition.h"
#include "freertos/idf_additions.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "ff.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#include "maincfg.h"
#include "extMemory.h"
#include "graphics.h"
#include "board.h"
#include "grpconvert.h"

#define GRP_VIRT_OFFSET             4
#define PAGE_SHIFT                  14
#define PAGE_SIZE                   (1 << PAGE_SHIFT)
#define NO_SLOT                     (-1)
#define NVM_PARTITION_SUBTYPE       0x40
#define NVM_PARTITION_LABEL         "dukenvm"
#define NVM_ERASE_CHUNK             (64 * 1024)
#define CLMT_ENTRIES                1024
#define BOUNCE_SIZE                 4096    // internal RAM is scarce: read pages in chunks

static const char *TAG = "extmem";

uint8_t *ext_memory = (uint8_t*) EXT_FLASH_BASE;

// Allocated in PSRAM: internal RAM is needed for the renderer and Bluetooth.
// Page reads go through the internal bounce buffer, so these are rarely touched.
static FATFS *fatfsPtr;
static FIL *grpFilePtr;
#define fatfs (*fatfsPtr)
#define grpFile (*grpFilePtr)
#if FF_USE_FASTSEEK
static DWORD *clmt;
#endif
static sdmmc_card_t *card;
static uint32_t grpSize;
static uint32_t numPages;
static uint8_t *bounce;             // DMA-capable: PSRAM can't be an SD DMA target

static uint8_t *cacheMem;
static int numSlots;
static int16_t *pageToSlot;
static int32_t *slotToPage;
static uint8_t *slotReferenced;
static int clockHand;
static uint32_t lastPage = UINT32_MAX;
static uint8_t *lastPageData;

static const esp_partition_t *nvmPartition;
static uint32_t nvmBase;            // virtual offset where the partition starts
// Flash access (read, erase, write) disables the PSRAM cache, so it must not
// run on a task whose stack is in PSRAM (the game task's may be): a helper
// task with an internal stack does it. NVM accesses are rare (settings and
// savegames), so the hand-over costs nothing noticeable.
typedef enum
{
    NVM_READ, NVM_ERASE, NVM_WRITE
} nvmOp_t;
typedef struct
{
    nvmOp_t op;
    uint32_t offset;
    const void *data;
    uint32_t size;
    esp_err_t result;
} nvmRequest_t;
static nvmRequest_t nvmRequest;
static SemaphoreHandle_t nvmStart, nvmDone;

static uint32_t currentAddress;
static void (*busyTask)(void);

static uint32_t cacheMisses;

static bool registerCard(void)
{
    ESP_LOGI(TAG, "SD card at %d kHz", card->real_freq_khz);
    BYTE pdrv = 0xFF;
    if (ff_diskio_get_drive(&pdrv) != ESP_OK || pdrv == 0xFF)
    {
        return false;
    }
    ff_diskio_register_sdmmc(pdrv, card);
    char drive[3] = {(char) ('0' + pdrv), ':', 0};
    return f_mount(&fatfs, drive, 1) == FR_OK;
}

#if SD_MODE == SD_MODE_SPI
static bool mountSd(void)
{
    // The bus is the display's: displayInit() has set it up.
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = DISPLAY_SPI_HOST;
    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = PIN_SD_CS;
    slot.host_id = DISPLAY_SPI_HOST;
    sdspi_dev_handle_t handle;
    if (sdspi_host_init() != ESP_OK)
    {
        return false;
    }
    // Most cards manage 40 MHz over SPI; retry at 20 MHz for the rest.
    const int speeds[] = {SDMMC_FREQ_HIGHSPEED, SDMMC_FREQ_DEFAULT};
    for (unsigned int i = 0; i < sizeof(speeds) / sizeof(speeds[0]); i++)
    {
        if (sdspi_host_init_device(&slot, &handle) != ESP_OK)
        {
            continue;
        }
        host.slot = handle;
        host.max_freq_khz = speeds[i];
        card = calloc(1, sizeof(sdmmc_card_t));
        if (sdmmc_card_init(&host, card) == ESP_OK)
        {
            return registerCard();
        }
        free(card);
        card = NULL;
        sdspi_host_remove_device(handle);
    }
    return false;
}
#else
static bool mountSd(void)
{
    if (sdmmc_host_init() != ESP_OK)
    {
        return false;
    }
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.clk = PIN_SD_CLK;
    slot.cmd = PIN_SD_CMD;
    slot.d0 = PIN_SD_D0;
    slot.width = 1;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    if (sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slot) != ESP_OK)
    {
        return false;
    }
    // High speed (40 MHz) first, then the default 20 MHz.
    const int speeds[] = {SDMMC_FREQ_HIGHSPEED, SDMMC_FREQ_DEFAULT};
    for (unsigned int i = 0; i < sizeof(speeds) / sizeof(speeds[0]); i++)
    {
        sdmmc_host_t host = SDMMC_HOST_DEFAULT();
        host.slot = SDMMC_HOST_SLOT_1;
        host.max_freq_khz = speeds[i];
        card = calloc(1, sizeof(sdmmc_card_t));
        if (sdmmc_card_init(&host, card) == ESP_OK)
        {
            return registerCard();
        }
        free(card);
        card = NULL;
    }
    return false;
}
#endif

static bool openGrp(void)
{
    char drive[3] = {(char) (fatfs.pdrv + '0'), ':', 0};
    char path[32];
    if (!prepareGrp(drive, path, sizeof(path)))
    {
        return false;
    }
    if (f_open(&grpFile, path, FA_READ) != FR_OK)
    {
        return false;
    }
    grpSize = f_size(&grpFile);
#if FF_USE_FASTSEEK
    grpFile.cltbl = clmt;
    clmt[0] = CLMT_ENTRIES;
    if (f_lseek(&grpFile, CREATE_LINKMAP) != FR_OK)
    {
        // Too fragmented for the table: fall back to plain seeking.
        grpFile.cltbl = NULL;
        ESP_LOGW(TAG, "GRP too fragmented for fast seek (%lu entries needed)", (unsigned long) clmt[0]);
    }
#endif
    return true;
}

static bool allocCache(void)
{
    numPages = (grpSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
    // Everything in PSRAM not needed by the level cache arena (allocated later
    // by the game) goes to the GRP cache.
    int32_t budget = (int32_t) heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)
                     - INTERNAL_FLASH_ARENA_SIZE - PSRAM_RESERVE
                     - (int32_t) (numPages * sizeof(*pageToSlot));
    numSlots = budget > 0 ? budget / (PAGE_SIZE + sizeof(*slotToPage) + 1) : 0;
    if (numSlots < 16)
    {
        return false;
    }
    if ((uint32_t) numSlots > numPages)
    {
        numSlots = numPages;
    }
    cacheMem = heap_caps_malloc((size_t) numSlots * PAGE_SIZE, MALLOC_CAP_SPIRAM);
    pageToSlot = heap_caps_malloc(numPages * sizeof(*pageToSlot), MALLOC_CAP_SPIRAM);
    slotToPage = heap_caps_malloc(numSlots * sizeof(*slotToPage), MALLOC_CAP_SPIRAM);
    slotReferenced = heap_caps_calloc(numSlots, 1, MALLOC_CAP_SPIRAM);
    bounce = heap_caps_malloc(BOUNCE_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!cacheMem || !pageToSlot || !slotToPage || !slotReferenced || !bounce)
    {
        return false;
    }
    for (uint32_t p = 0; p < numPages; p++)
    {
        pageToSlot[p] = NO_SLOT;
    }
    for (int s = 0; s < numSlots; s++)
    {
        slotToPage[s] = NO_SLOT;
    }
    return true;
}

static void nvmTask(void *arg)
{
    (void) arg;
    while (1)
    {
        xSemaphoreTake(nvmStart, portMAX_DELAY);
        switch (nvmRequest.op)
        {
            case NVM_READ:
                nvmRequest.result = esp_partition_read(nvmPartition, nvmRequest.offset, (void*) nvmRequest.data, nvmRequest.size);
                break;
            case NVM_ERASE:
                nvmRequest.result = esp_partition_erase_range(nvmPartition, nvmRequest.offset, nvmRequest.size);
                break;
            case NVM_WRITE:
                nvmRequest.result = esp_partition_write(nvmPartition, nvmRequest.offset, nvmRequest.data, nvmRequest.size);
                break;
        }
        xSemaphoreGive(nvmDone);
    }
}

static esp_err_t nvmRun(nvmOp_t op, uint32_t offset, const void *data, uint32_t size)
{
    nvmRequest = (nvmRequest_t) {.op = op, .offset = offset, .data = data, .size = size};
    xSemaphoreGive(nvmStart);
    xSemaphoreTake(nvmDone, portMAX_DELAY);
    return nvmRequest.result;
}

static bool openNvm(void)
{
    nvmPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, NVM_PARTITION_SUBTYPE, NVM_PARTITION_LABEL);
    if (!nvmPartition)
    {
        return false;
    }
    nvmBase = EXT_MEMORY_SIZE - nvmPartition->size;
    nvmStart = xSemaphoreCreateBinary();
    nvmDone = xSemaphoreCreateBinary();
    // The data to write may sit on the caller's PSRAM stack; the flash driver
    // copies such buffers through internal RAM itself.
    xTaskCreatePinnedToCoreWithCaps(nvmTask, "nvm", 3072, NULL, 6, NULL, 0, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    return true;
}

bool extMemInit(void)
{
    fatfsPtr = heap_caps_calloc(1, sizeof(FATFS), MALLOC_CAP_SPIRAM);
    grpFilePtr = heap_caps_calloc(1, sizeof(FIL), MALLOC_CAP_SPIRAM);
#if FF_USE_FASTSEEK
    clmt = heap_caps_calloc(CLMT_ENTRIES, sizeof(DWORD), MALLOC_CAP_SPIRAM);
#endif
    if (!openNvm())
    {
        displayPrintln(1, "No '" NVM_PARTITION_LABEL "' partition!");
        return false;
    }
    displayPrintln(1, "Mounting SD card...");
    if (!mountSd())
    {
        displayPrintln(1, "Can't mount the SD card.");
        return false;
    }
    displayPrintln(1, "SD card: %d MB, %d kHz", (int) ((uint64_t) card->csd.capacity * card->csd.sector_size >> 20), card->real_freq_khz);
    if (!openGrp())
    {
        displayPrintln(1, "Can't open " DATA_FILE_NAME " on the SD card.");
        return false;
    }
    if (grpSize + GRP_VIRT_OFFSET > nvmBase)
    {
        displayPrintln(1, DATA_FILE_NAME " is too large (%d bytes).", (int) grpSize);
        return false;
    }
    if (!allocCache())
    {
        displayPrintln(1, "Out of memory for the GRP cache (PSRAM %d kB, internal %d kB free).",
                       (int) (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
                       (int) (heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
        return false;
    }
    displayPrintln(1, "GRP: %d bytes, cache %d kB", (int) grpSize, numSlots * PAGE_SIZE / 1024);
    return true;
}

uint32_t extMemGetGrpSize(void)
{
    return grpSize;
}

static uint8_t *loadPage(uint32_t page)
{
    // Clock replacement: skip slots referenced since the hand last passed.
    while (slotToPage[clockHand] != NO_SLOT && slotReferenced[clockHand])
    {
        slotReferenced[clockHand] = 0;
        clockHand = (clockHand + 1) % numSlots;
    }
    int slot = clockHand;
    clockHand = (clockHand + 1) % numSlots;
    if (slotToPage[slot] != NO_SLOT)
    {
        pageToSlot[slotToPage[slot]] = NO_SLOT;
    }
    uint8_t *data = cacheMem + (size_t) slot * PAGE_SIZE;
    uint32_t start = page << PAGE_SHIFT;
    UINT toRead = grpSize - start < PAGE_SIZE ? grpSize - start : PAGE_SIZE;
    UINT total = 0;
    FRESULT res = f_lseek(&grpFile, start);
    while (res == FR_OK && total < toRead)
    {
        UINT chunk = toRead - total < BOUNCE_SIZE ? toRead - total : BOUNCE_SIZE;
        UINT got = 0;
        res = f_read(&grpFile, bounce, chunk, &got);
        memcpy(data + total, bounce, got);
        total += got;
        if (got != chunk)
        {
            break;
        }
    }
    if (res != FR_OK || total != toRead)
    {
        FIXME("GRP read failed at %lu (err %d, %u/%u bytes)\r\n", (unsigned long) start, res, total, toRead);
        memset(data + total, 0, PAGE_SIZE - total);
    }
    slotToPage[slot] = page;
    pageToSlot[page] = slot;
    slotReferenced[slot] = 1;
    cacheMisses++;
    return data;
}

static inline uint8_t *getPage(uint32_t page)
{
    if (page == lastPage)
    {
        return lastPageData;
    }
    int slot = pageToSlot[page];
    uint8_t *data;
    if (slot != NO_SLOT)
    {
        slotReferenced[slot] = 1;
        data = cacheMem + (size_t) slot * PAGE_SIZE;
    }
    else
    {
        data = loadPage(page);
    }
    lastPage = page;
    lastPageData = data;
    return data;
}

static void readGrp(uint8_t *dest, uint32_t offset, uint32_t length)
{
    while (length)
    {
        if (offset >= grpSize)
        {
            // The engine reads aligned blocks that may run past the end.
            memset(dest, 0, length);
            return;
        }
        uint32_t inPage = offset & (PAGE_SIZE - 1);
        uint32_t chunk = PAGE_SIZE - inPage;
        if (chunk > length)
        {
            chunk = length;
        }
        if (chunk > grpSize - offset)
        {
            chunk = grpSize - offset;
        }
        memcpy(dest, getPage(offset >> PAGE_SHIFT) + inPage, chunk);
        dest += chunk;
        offset += chunk;
        length -= chunk;
    }
}

static void readVirtual(uint8_t *dest, uint32_t address, uint32_t length)
{
    address &= EXT_MEMORY_ADDRESS_MASK;
    if (address >= nvmBase)
    {
        uint32_t offset = address - nvmBase;
        if (offset + length > nvmPartition->size)
        {
            FIXME("NVM read out of range: 0x%08lx + %lu\r\n", (unsigned long) address, (unsigned long) length);
            memset(dest, 0xFF, length);
            return;
        }
        nvmRun(NVM_READ, offset, dest, length);
        return;
    }
    if (address < GRP_VIRT_OFFSET)
    {
        uint32_t pad = GRP_VIRT_OFFSET - address;
        if (pad > length)
        {
            pad = length;
        }
        memset(dest, 0, pad);
        dest += pad;
        address += pad;
        length -= pad;
    }
    readGrp(dest, address - GRP_VIRT_OFFSET, length);
}

void extMemSetCurrentAddress(uint32_t address)
{
    currentAddress = address & EXT_MEMORY_ADDRESS_MASK;
}

void* extMemGetDataFromCurrentAddress(void *dest, unsigned int length)
{
    if (length)
    {
        readVirtual(dest, currentAddress, length);
        currentAddress += length;
    }
    return dest;
}

void* extMemGetDataAligned(void *dest, unsigned int length)
{
    return extMemGetDataFromCurrentAddress(dest, length);
}

void* extMemGetDataFromAddress(void *dest, const void *src, unsigned int length)
{
    extMemSetCurrentAddress((uint32_t) src);
    return extMemGetDataFromCurrentAddress(dest, length);
}

uint8_t extMemGetByteFromAddress(const void *addr)
{
    uint8_t b;
    extMemGetDataFromAddress(&b, addr, 1);
    return b;
}

short extMemFlashGetShortFromAddress(const void *addr)
{
    short s;
    extMemGetDataFromAddress(&s, addr, sizeof(s));
    return s;
}

void* extMemStartAsynchDataRead(uint32_t address, void *dest, uint32_t cnt)
{
    readVirtual(dest, address, cnt);
    return (uint8_t*) dest + (address & 3);
}

void extMemSetMemBusyBackgroundTask(void *task)
{
    busyTask = task;
}

static bool nvmRange(uint32_t address, uint32_t size, uint32_t *offset)
{
    address &= EXT_MEMORY_ADDRESS_MASK;
    if (address < nvmBase || address - nvmBase + size > nvmPartition->size)
    {
        FIXME("Write outside NVM: 0x%08lx + %lu\r\n", (unsigned long) address, (unsigned long) size);
        return false;
    }
    *offset = address - nvmBase;
    return true;
}

void extMemErase(uint32_t address, uint32_t size)
{
    uint32_t offset;
    if (!size || !nvmRange(address, size, &offset))
    {
        return;
    }
    uint32_t sector = nvmPartition->erase_size;
    uint32_t start = offset & ~(sector - 1);
    uint32_t end = (offset + size + sector - 1) & ~(sector - 1);
    while (start < end)
    {
        uint32_t chunk = end - start < NVM_ERASE_CHUNK ? end - start : NVM_ERASE_CHUNK;
        // Large erases are only possible on 64 kB boundaries.
        if ((start & (NVM_ERASE_CHUNK - 1)) && chunk > sector)
        {
            chunk = sector;
        }
        nvmRun(NVM_ERASE, start, NULL, chunk);
        start += chunk;
        if (busyTask)
        {
            busyTask();
        }
    }
}

void extMemProgram(uint32_t address, uint8_t *buffer, uint32_t size)
{
    uint32_t offset;
    if (!size || !nvmRange(address, size, &offset))
    {
        return;
    }
    nvmRun(NVM_WRITE, offset, buffer, size);
}

void extMemWrite(uint32_t address, void *buffer, uint32_t size)
{
    extMemErase(address, size);
    extMemProgram(address, buffer, size);
}

void extMemEraseNvm(void)
{
    nvmRun(NVM_ERASE, 0, NULL, nvmPartition->size);
}
