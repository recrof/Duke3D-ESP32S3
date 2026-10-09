#include "maincfg.h"
#include <stdint.h>
#include "internalFlash.h"
#if !WIN32 && MG24
#include "em_device.h"
#include "i_memory.h"
#include "printf.h"
#endif
// Note: subject to change
#define DEBUG_INTERNAL_FLASH        1
#if DEBUG_INTERNAL_FLASH
#define internalFlashDbgPrintf printf
#else
#define internalFlashDbgPrintf(...)
#endif
#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE 8192
#endif
#define LET_ME_JUST_WORK       1
#if WIN32
  #define APP_SIZE    (((1024 * 640) + FLASH_PAGE_SIZE - 1) & ~(FLASH_PAGE_SIZE - 1))
#else
  #define APP_SIZE    (((FLASH_CODE_SIZE & (0xFFFFFF))  + FLASH_PAGE_SIZE - 1)  & ~(FLASH_PAGE_SIZE - 1))
#endif
#define CHECK_ALIGN(size)     if ((uint32_t)size & 3)  FIXME("Error, storing non word-aligned object (%d) at line %d", size, __LINE__);
#if WIN32
#if LET_ME_JUST_WORK
  #define FLASH_SIZE (15360 * 1024)
#else
  #define FLASH_SIZE (1536 * 1024)
#endif // LET_ME_JUST_WORK
  uint8_t  internalFlash[FLASH_SIZE];
  static uint32_t pointer = APP_SIZE;
#else
  static uint32_t pointer = 0;
#endif // WIN32_T
static int commonSize = 0;
void internalFlashInit(void)
{
    pointer = APP_SIZE;
    commonSize = 0;
}
#if WIN32
void eraseInternalFlash(int sections)
{
    FIXME("Erasing flash\r\n");
}
void *getCurrentInternalFlashPtr(void)
{
        return &internalFlash[pointer];
}
void setCurrentInternalFlashPtr(void *ptr)
{
    pointer = (uint8_t *) ptr - (uint8_t *) internalFlash;
}

void *reserveInternalFlashSize(int size)
{
    CHECK_ALIGN(size);
    int oldPointer = pointer;
    #if WIN32
        pointer += size;
        return &internalFlash[oldPointer];
    #endif // WIN32
}
/**
*   brief: write the buffer to internal flash and free allocated buffer
*/

void storeToInternalFlashAndFree(void **buffer, int size)
{
    void *ptr = *buffer;
    *buffer = storeToInternalFlash(*buffer, size);
    free(ptr);
}

#if WIN32
void * storeToInternalFlash2(void *buffer, int size, char *function, int line)
#else
void * storeToInternalFlash(void *buffer, int size)
#endif
{
    // TODO: distinguish if buffer is on external flash.
    // TODO: if page is dirty, save and erase
    if (size & 3)
    {
        #if WIN32
            printf("Caching to flash. Current %d, requested %d, remaining %d (real: %d), Line %d, function %s\r\n", pointer, size, FLASH_SIZE - size - pointer, 1536 * 1024 - size - pointer, line, function);
        #endif
        FIXME("UNALIGNED STORE TO INTERNAL FLASH!");

    }
    CHECK_ALIGN(size);
#if WIN32

    printf("Caching to flash. Current %d, requested %d, remaining %d (real: %d), Line %d, function %s\r\n", pointer, size, FLASH_SIZE - size - pointer, 1536 * 1024 - size - pointer, line, function);
        if (size + pointer > FLASH_SIZE)
    {
        while (1)
        {
            FIXME("FLASH OVERFLOW");
        }
    }
    else
    {
        memcpy(&internalFlash[pointer], buffer, size);
    }
    uint8_t *ptr = &internalFlash[pointer];
    pointer += size;
    return ptr;
#endif
}
void * storeToInternalFlashAtPointer(void *buffer, void *flashPos, int size)
{
    CHECK_ALIGN(flashPos);
    CHECK_ALIGN(size);
    // TODO: distinguish if buffer is on external flash.
    // TODO: if page is dirty, save and erase
#if WIN32
  //  printf("Storing %d bytes to position fixed flash %p. Better to be reserverd!\r\n", size, flashPos);
    memcpy(flashPos, buffer, size);
    return flashPos;
#endif
}

#else


#if USE_MSC
// MSC will eat ram.
void flashErasePage(uint32_t *pageAddress)
{
    __disable_irq();
    MSC_ErasePage(pageAddress);
    __enable_irq();
}
void programFlashWord(uint32_t *address, uint32_t word)
{
    if (*address == 0xFFFFFFFF)
    {
        __disable_irq();
        MSC_WriteWord(address, &word, 4 );
        __enable_irq();
    }
}
#else
__attribute__ ((section(".ram"), noinline)) void flashEraseCurrentPage()
{
    MSC->WRITECMD = MSC_WRITECMD_ERASEPAGE;
    while (!(MSC->IF & MSC_IF_ERASE));    // wait until erase operations is done
}
void flashErasePage(uint32_t *pageAddress)
{
    __disable_irq();
    // enable write
    MSC->WRITECTRL_SET = MSC_WRITECTRL_WREN;
    // set address
    MSC->ADDRB = (uint32_t) pageAddress & ~(FLASH_PAGE_SIZE - 1);
    uint32_t status = MSC->STATUS;
    if (!(status & (MSC_STATUS_BUSY | MSC_STATUS_INVADDR | MSC_STATUS_LOCKED)))
    {
        // wait the power up sequence to be finished
        while (!(MSC->STATUS & MSC_STATUS_WREADY));
        // issue erase CMD
        flashEraseCurrentPage();
    }
    MSC->WRITECTRL_CLR = MSC_WRITECTRL_WREN;
    __enable_irq();
}
__attribute__ ((section(".ram"), noinline)) void flashProgramCurrentWord(uint32_t word)
{
    MSC->WDATA = word;
    while (!(MSC->STATUS & MSC_STATUS_WDATAREADY));    // wait until write operation is done
}
void programFlashWord(uint32_t *address, uint32_t word)
{
#if 1
    if (*address == 0xFFFFFFFF)
    {
        __disable_irq();

        MSC->LOCK = MSC_LOCK_LOCKKEY_UNLOCK;
        // enable write
        MSC->WRITECTRL_SET = MSC_WRITECTRL_WREN;
        // set address
        MSC->ADDRB = (uint32_t) address;
        uint32_t status = MSC->STATUS;


        if (!(status & (MSC_STATUS_BUSY | MSC_STATUS_INVADDR | MSC_STATUS_LOCKED)))
        {
            // wait the power up sequence to be finished
            while (!(MSC->STATUS & MSC_STATUS_WREADY));
            while (!(MSC->STATUS & MSC_STATUS_WDATAREADY));
            // issue erase CMD
            flashProgramCurrentWord(word);
        }
        MSC->WRITECTRL_CLR = MSC_WRITECTRL_WREN;
        __enable_irq();
    }
#else
    if (*address == 0xFFFFFFFF)
    {
        __disable_irq();
        MSC_WriteWord(address, &word, 4 );
        __enable_irq();
    }

#endif
}
#endif
/**
 * @brief Store a word to flash. It checks if erase must be triggered.
 *        In such a case, it will copy also the previous header values, and the
 *        data PAST the header region.
 * @param dest
 * @param word
 * @param flashRegion
 * @param isHeader
 */
#define PROCESS_ACTION_TIME_MICROSEC    100000UL
static void storeWordToFlash(uint32_t *dest, uint32_t word)
{
    dest = (uint32_t*) ((uint32_t)dest | FLASH_PTR_BASE); // make sure to add the upper flash byte.

    // first check if dest is compatible, i.e. is the same as source or it is erased
    if (!(*dest == word || *dest == 0xFFFFFFFF))
    {
        // we need to erase before writing.
        uint32_t pageAddress = ((uint32_t) dest) & ~(FLASH_PAGE_SIZE - 1); // page that will be erased
        uint32_t startCopyAddress = pageAddress;
        uint32_t stopCopyAddress = (uint32_t) dest; // copy up to the previous word.
        // now that we know from where to where (stop non inclusive) we need to copy, we can allocate enough buffer
        // note! addresses are 4-byte aligned!
        uint32_t tmp [FLASH_PAGE_SIZE / sizeof(uint32_t)];
        for (uint32_t i = 0; i < (stopCopyAddress - startCopyAddress) / sizeof(uint32_t);  i++)
        {
            tmp[i] = *((uint32_t*) startCopyAddress + i);
        }

        // erase
        internalFlashDbgPrintf("Erasing page 0x%08X Start Copy Addr 0x%08X Stop Copy Addr 0x%08X\r\n. ", pageAddress, startCopyAddress, stopCopyAddress);
        flashErasePage((uint32_t*) pageAddress);
        // write back everything
        for (uint32_t i = 0; i < (stopCopyAddress - startCopyAddress) / sizeof(uint32_t); i++)
        {
            programFlashWord(((uint32_t*) startCopyAddress + i), tmp[i]);
        }
    }
    // write word
    if (!(*dest == word))
    {
        if (*dest == 0xFFFFFFFF)
        {
            programFlashWord(dest, word);
        }
        else
        {
            printf("trying to overwrite a non blank word: addr: 0x%08X old: 0x%08X new 0x%08X. Blocking", dest, *dest, word);
            while (1);
        }
    }
    //
}
#if USE_DYNAMIC_MEMORY
/**
*   brief: write the buffer to internal flash and free allocated buffer
*/

void storeToInternalFlashAndFree(void **buffer, int size)
{
    void *ptr = *buffer;
    *buffer = storeToInternalFlash(*buffer, size);
    free(ptr);
}
#endif
/**
 * @brief write the buffer to the flash region
 */

////////////////////////////////

void * storeToInternalFlash(const void *buffer, int size)
{
    // TODO: distinguish if buffer is on external flash.
    // TODO: if page is dirty, save and erase
    if (size & 3)
        FIXME("UNALIGNED...");
    CHECK_ALIGN(size);
//    internalFlashDbgPrintf("Caching to flash. Current %d, requested %d, remaining %d (real: %d)\r\n", pointer, size, FLASH_SIZE - size - pointer, 1536 * 1024 - size - pointer);
    printf("Caching to flash. Add: %xh, rq: %xh, lf: %xh\r\n", pointer, size, FLASH_SIZE - size - pointer);

    if (size + pointer > FLASH_SIZE)
    {
        while (1)
        {
            FIXME("FLASH OVERFLOW");
        }
    }
    else
    {
        for (int i = 0; i < size / 4; i++)
        {
          if (isOnExternalFlash(buffer))
          {
            uint32_t data;
            extMemGetDataFromAddress(&data, (uint32_t*) buffer + i, sizeof(data));
            storeWordToFlash((uint32_t*)pointer + i, data);
          }
          else
          {
              storeWordToFlash((uint32_t*)pointer + i, ((uint32_t*)buffer)[i]);
          }
        }
    }
    uint8_t *ptr = (uint8_t*) FLASH_PTR_BASE + pointer;
    pointer += size;
    return ptr;
}

void * storeToInternalFlashAtPointer(const void *buffer, const void *flashPos, int size)
{
    CHECK_ALIGN(flashPos);
    CHECK_ALIGN(size);
    // TODO: distinguish if buffer is on external flash.
    // TODO: if page is dirty, save and erase
    internalFlashDbgPrintf("SiF %d b at %p.\r\n", size, flashPos);
        if (size + ((uint32_t) flashPos & 0xFFFFFF) > FLASH_SIZE)
    {
        while (1)
        {
            FIXME("FLASH OVERFLOW");
        }
    }
    else
    {
        for (int i = 0; i < size / 4; i++)
        {
          if (isOnExternalFlash(buffer))
          {
            uint32_t data;
            extMemGetDataFromAddress(&data, (uint32_t*) buffer + i, sizeof(data));
            storeWordToFlash((uint32_t*)flashPos + i, data);
          }
          else
          {
            storeWordToFlash((uint32_t*)flashPos + i, ((uint32_t*)buffer)[i]);
          }
        }
    }
    return (void*) flashPos;
}
void *getCurrentInternalFlashPtr(void)
{
  return (uint8_t*) FLASH_PTR_BASE + pointer;
}
void *reserveInternalFlashSize(uint32_t size)
{
    CHECK_ALIGN(size);
    uint32_t oldPointer = pointer;
    #if WIN32
        pointer += size;
        return &internalFlash[oldPointer];
    #endif // WIN32
    pointer += size;
    for (uint32_t i = oldPointer; i < pointer; i += 4)
    {
      storeWordToFlash((uint32_t*)i, 0xFFFFFFFF);   // this will take care of erasing and restoring previous data.
    }
    return (uint8_t *) FLASH_PTR_BASE +  oldPointer;
}
///
#endif
// called after common stuff has been initialized
void internalFlashSetCommonZone(void)
{
    commonSize = (pointer - APP_SIZE + FLASH_PAGE_SIZE-1) & ~(FLASH_PAGE_SIZE -1);
}
//
void internalFlashResetToCommonZoneEnd(void)
{
    // sets the pointer to the end to common zone
    pointer = APP_SIZE + commonSize;
}
void * storeToInternalFlashAlign(void *buffer, int size)
{
    return storeToInternalFlash(buffer, (size + 3) & ~3);
}

int internalFlashSpaceLeft(void)
{
    return FLASH_SIZE - pointer;
}
