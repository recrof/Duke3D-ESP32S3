/*
 *  On-device GRP conversion for the ESP32-S3 port.
 *
 *  The MCU game code expects a GRP prepared by Tools/PythonCVT/mcugrpcvt.py:
 *  every .VOC sound converted to an 8-bit .WAV (sounds.c looks the sounds up
 *  as .WAV and skips a 44-byte header), and every type 1 MIDI merged into a
 *  single type 0 track (nh_midi.c plays only the first track). Instead of
 *  requiring that PC step, the first boot converts DUKE3D.GRP into
 *  DN3DCONV.GRP on the SD card, following the same rules as the Python tool.
 *  The original file is never modified.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "ff.h"
#include "main.h"
#include "graphics.h"
#include "grpconvert.h"

#define CONV_FILE_NAME          "DN3DCONV.GRP"
#define CONV_TEMP_NAME          "DN3DCONV.TMP"
#define CONV_STAMP_NAME         "DN3DCONV.SRC"  // identifies the source the conversion came from
#define IO_CHUNK                4096
#define GRP_HEADER_SIZE         16
#define GRP_ENTRY_SIZE          16
#define WAV_HEADER_SIZE         44

static const char *TAG = "grpconv";

typedef struct
{
    char name[13];
    uint32_t size;
    uint32_t offset;
} grpEntry_t;

typedef struct
{
    uint8_t *data;
    uint32_t len;
    uint32_t cap;
} buf_t;

static uint8_t *ioBuffer;   // internal, DMA capable: PSRAM buffers make SD transfers sector by sector

static bool bufReserve(buf_t *b, uint32_t extra)
{
    if (b->len + extra <= b->cap)
    {
        return true;
    }
    uint32_t cap = b->cap ? b->cap : 4096;
    while (cap < b->len + extra)
    {
        cap *= 2;
    }
    uint8_t *p = heap_caps_realloc(b->data, cap, MALLOC_CAP_SPIRAM);
    if (!p)
    {
        return false;
    }
    b->data = p;
    b->cap = cap;
    return true;
}

static bool bufPut(buf_t *b, const void *data, uint32_t len)
{
    if (!bufReserve(b, len))
    {
        return false;
    }
    memcpy(b->data + b->len, data, len);
    b->len += len;
    return true;
}

static bool bufPutByte(buf_t *b, uint8_t v)
{
    return bufPut(b, &v, 1);
}

static bool bufPutVarLen(buf_t *b, uint32_t v)
{
    uint8_t tmp[5];
    int n = 0;
    tmp[n++] = v & 0x7F;
    while (v >>= 7)
    {
        tmp[n++] = 0x80 | (v & 0x7F);
    }
    while (n--)
    {
        if (!bufPutByte(b, tmp[n]))
        {
            return false;
        }
    }
    return true;
}

static void bufFree(buf_t *b)
{
    free(b->data);
    memset(b, 0, sizeof(*b));
}

static bool readAt(FIL *f, uint32_t offset, void *dest, uint32_t len)
{
    if (f_lseek(f, offset) != FR_OK)
    {
        return false;
    }
    uint8_t *d = dest;
    while (len)
    {
        UINT chunk = len < IO_CHUNK ? len : IO_CHUNK;
        UINT got;
        if (f_read(f, ioBuffer, chunk, &got) != FR_OK || got != chunk)
        {
            return false;
        }
        memcpy(d, ioBuffer, chunk);
        d += chunk;
        len -= chunk;
    }
    return true;
}

static bool writeAll(FIL *f, const void *src, uint32_t len)
{
    const uint8_t *s = src;
    while (len)
    {
        UINT chunk = len < IO_CHUNK ? len : IO_CHUNK;
        UINT put;
        memcpy(ioBuffer, s, chunk);
        if (f_write(f, ioBuffer, chunk, &put) != FR_OK || put != chunk)
        {
            return false;
        }
        s += chunk;
        len -= chunk;
    }
    return true;
}

static bool hasSuffix(const char *name, const char *suffix)
{
    size_t n = strlen(name), s = strlen(suffix);
    return n >= s && !strcasecmp(name + n - s, suffix);
}

static uint32_t le24(const uint8_t *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16);
}

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t) p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static void putLe16(uint8_t *p, uint16_t v)
{
    p[0] = v;
    p[1] = v >> 8;
}

static void putLe32(uint8_t *p, uint32_t v)
{
    putLe16(p, v);
    putLe16(p + 2, v >> 16);
}

/*
 *  VOC -> 8-bit mono WAV, block handling as in mcugrpcvt.py (including its
 *  header, which reports the source bits per sample even after 16 -> 8 bit
 *  conversion; the game only uses the sample rate).
 */
static bool vocToWav(const uint8_t *voc, uint32_t len, buf_t *out)
{
    if (len < 26 || memcmp(voc, "Creative Voice File\x1A", 20))
    {
        return false;
    }
    uint8_t header[WAV_HEADER_SIZE] = {0};
    if (!bufPut(out, header, sizeof(header)))
    {
        return false;
    }
    uint32_t sampleRate = 11025;
    uint32_t bitsPerSample = 8;
    int blocks9 = 0;
    uint32_t pos = 26;
    while (pos < len)
    {
        uint8_t type = voc[pos++];
        if (type == 0)
        {
            break;
        }
        if (pos + 3 > len)
        {
            break;
        }
        uint32_t size = le24(&voc[pos]);
        pos += 3;
        if (pos + size > len)
        {
            size = len - pos;       // truncated file: keep what is there
        }
        const uint8_t *b = &voc[pos];
        switch (type)
        {
            case 1:     // sound data
                if (size < 2)
                {
                    break;
                }
                sampleRate = 1000000 / (256 - b[0]);
                if (b[1] != 0)
                {
                    ESP_LOGW(TAG, "compressed VOC block skipped");
                    break;
                }
                bufPut(out, b + 2, size - 2);
                break;
            case 2:     // continuation
                bufPut(out, b, size);
                break;
            case 5:     // text
                break;
            case 9:     // extended sound data
            {
                if (size < 12 || ++blocks9 > 1)
                {
                    break;
                }
                sampleRate = b[0] | (b[1] << 8) | (b[2] << 16) | ((uint32_t) b[3] << 24);
                bitsPerSample = b[4];
                const uint8_t *data = b + 12;
                uint32_t dataLen = size - 12;
                if (bitsPerSample == 8)
                {
                    bufPut(out, data, dataLen);
                }
                else if (bitsPerSample == 16)
                {
                    if (!bufReserve(out, dataLen / 2))
                    {
                        return false;
                    }
                    for (uint32_t i = 0; i + 1 < dataLen; i += 2)
                    {
                        int16_t s = (int16_t) (data[i] | (data[i + 1] << 8));
                        out->data[out->len++] = (uint8_t) ((s >> 8) + 128);
                    }
                }
                // bits == 0: a known corrupted file (SNAKRM.VOC); adds nothing.
                break;
            }
            default:    // silence, markers, repeats: not used by Duke3D's sounds
                ESP_LOGW(TAG, "VOC block type %d ignored", type);
                break;
        }
        pos += size;
    }
    uint32_t dataSize = out->len - WAV_HEADER_SIZE;
    uint8_t *h = out->data;
    memcpy(h, "RIFF", 4);
    putLe32(h + 4, 36 + dataSize);
    memcpy(h + 8, "WAVEfmt ", 8);
    putLe32(h + 16, 16);
    putLe16(h + 20, 1);             // PCM
    putLe16(h + 22, 1);             // mono
    putLe32(h + 24, sampleRate);
    putLe32(h + 28, sampleRate);    // byte rate: 8-bit mono
    putLe16(h + 32, 1);             // block align
    putLe16(h + 34, bitsPerSample);
    memcpy(h + 36, "data", 4);
    putLe32(h + 40, dataSize);
    return true;
}

/*
 *  Type 1 -> type 0 MIDI, with mido.merge_tracks semantics: all events on one
 *  absolute timeline (stable: equal times keep track order), every track's
 *  end-of-track dropped (its delta carried over) and a single one appended.
 */
typedef struct
{
    uint32_t time;          // absolute ticks
    uint32_t seq;           // original order, for a stable sort
    uint32_t offset;        // event bytes (status included) in the events buffer
    uint32_t length;
    bool endOfTrack;
} midiEvent_t;

static int compareEvents(const void *a, const void *b)
{
    const midiEvent_t *x = a, *y = b;
    if (x->time != y->time)
    {
        return x->time < y->time ? -1 : 1;
    }
    return x->seq < y->seq ? -1 : (x->seq > y->seq);
}

static bool readVarLen(const uint8_t *p, uint32_t end, uint32_t *pos, uint32_t *value)
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++)
    {
        if (*pos >= end)
        {
            return false;
        }
        uint8_t c = p[(*pos)++];
        v = (v << 7) | (c & 0x7F);
        if (!(c & 0x80))
        {
            *value = v;
            return true;
        }
    }
    return false;
}

static int midiDataBytes(uint8_t status)
{
    switch (status & 0xF0)
    {
        case 0xC0:
        case 0xD0:
            return 1;
        default:
            return 2;
    }
}

// Returns 1 if converted into out, 0 if the file is not type 1 (copy as is), -1 on error.
static int midiToType0(const uint8_t *mid, uint32_t len, buf_t *out)
{
    if (len < 14 || memcmp(mid, "MThd", 4) || be32(mid + 4) < 6)
    {
        return 0;
    }
    uint16_t format = (mid[8] << 8) | mid[9];
    uint16_t numTracks = (mid[10] << 8) | mid[11];
    uint16_t division = (mid[12] << 8) | mid[13];
    if (format != 1)
    {
        return 0;
    }
    buf_t events = {0};             // canonical event bytes (no running status)
    buf_t list = {0};               // midiEvent_t array
    uint32_t seq = 0;
    uint32_t pos = 8 + be32(mid + 4);
    int result = -1;
    for (int t = 0; t < numTracks; t++)
    {
        if (pos + 8 > len || memcmp(mid + pos, "MTrk", 4))
        {
            goto done;
        }
        uint32_t end = pos + 8 + be32(mid + pos + 4);
        if (end > len)
        {
            end = len;
        }
        pos += 8;
        uint32_t time = 0;
        uint8_t running = 0;
        while (pos < end)
        {
            uint32_t delta;
            if (!readVarLen(mid, end, &pos, &delta) || pos >= end)
            {
                break;
            }
            time += delta;
            midiEvent_t e = {.time = time, .seq = seq++, .offset = events.len};
            uint8_t status = mid[pos];
            if (status == 0xFF)     // meta: FF type len data
            {
                uint32_t start = pos;
                uint32_t mlen;
                pos += 2;
                if (pos > end || !readVarLen(mid, end, &pos, &mlen) || pos + mlen > end)
                {
                    goto done;
                }
                e.endOfTrack = mid[start + 1] == 0x2F;
                pos += mlen;
                bufPut(&events, mid + start, pos - start);
                running = 0;
            }
            else if (status == 0xF0 || status == 0xF7)  // sysex: copied raw
            {
                uint32_t start = pos++;
                uint32_t slen;
                if (!readVarLen(mid, end, &pos, &slen) || pos + slen > end)
                {
                    goto done;
                }
                pos += slen;
                bufPut(&events, mid + start, pos - start);
                running = 0;
            }
            else
            {
                if (status & 0x80)
                {
                    running = status;
                    pos++;
                }
                else if (!running)
                {
                    goto done;      // data byte without a status
                }
                int n = midiDataBytes(running);
                if (pos + n > end)
                {
                    goto done;
                }
                bufPutByte(&events, running);
                bufPut(&events, mid + pos, n);
                pos += n;
            }
            e.length = events.len - e.offset;
            if (!bufPut(&list, &e, sizeof(e)))
            {
                goto done;
            }
        }
        pos = end;
    }
    midiEvent_t *ev = (midiEvent_t*) list.data;
    uint32_t count = list.len / sizeof(midiEvent_t);
    qsort(ev, count, sizeof(midiEvent_t), compareEvents);
    // header + track header (length patched below)
    const uint8_t mthd[] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, division >> 8, division & 0xFF};
    const uint8_t mtrk[] = {'M', 'T', 'r', 'k', 0, 0, 0, 0};
    bufPut(out, mthd, sizeof(mthd));
    bufPut(out, mtrk, sizeof(mtrk));
    uint32_t trackStart = out->len;
    uint32_t lastTime = 0;
    uint32_t carried = 0;
    uint8_t running = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t delta = ev[i].time - lastTime + carried;
        lastTime = ev[i].time;
        if (ev[i].endOfTrack)
        {
            carried = delta;
            continue;
        }
        carried = 0;
        const uint8_t *bytes = events.data + ev[i].offset;
        bufPutVarLen(out, delta);
        if (bytes[0] < 0xF0)
        {
            // channel message: use running status like mido does
            if (bytes[0] == running)
            {
                bufPut(out, bytes + 1, ev[i].length - 1);
            }
            else
            {
                bufPut(out, bytes, ev[i].length);
                running = bytes[0];
            }
        }
        else
        {
            bufPut(out, bytes, ev[i].length);
            running = 0;
        }
    }
    bufPutVarLen(out, carried);
    const uint8_t eot[] = {0xFF, 0x2F, 0x00};
    bufPut(out, eot, sizeof(eot));
    uint32_t trackLen = out->len - trackStart;
    out->data[trackStart - 4] = trackLen >> 24;
    out->data[trackStart - 3] = trackLen >> 16;
    out->data[trackStart - 2] = trackLen >> 8;
    out->data[trackStart - 1] = trackLen;
    result = 1;
done:
    bufFree(&events);
    bufFree(&list);
    return result;
}

static bool readDirectory(FIL *f, grpEntry_t **entries, uint32_t *numFiles)
{
    uint8_t header[GRP_HEADER_SIZE];
    if (!readAt(f, 0, header, sizeof(header)) || memcmp(header, "KenSilverman", 12))
    {
        return false;
    }
    uint32_t n = header[12] | (header[13] << 8) | (header[14] << 16) | ((uint32_t) header[15] << 24);
    if (n == 0 || n > 4096)
    {
        return false;
    }
    uint8_t *dir = heap_caps_malloc(n * GRP_ENTRY_SIZE, MALLOC_CAP_SPIRAM);
    grpEntry_t *e = heap_caps_calloc(n, sizeof(grpEntry_t), MALLOC_CAP_SPIRAM);
    if (!dir || !e || !readAt(f, GRP_HEADER_SIZE, dir, n * GRP_ENTRY_SIZE))
    {
        free(dir);
        free(e);
        return false;
    }
    uint32_t offset = GRP_HEADER_SIZE + n * GRP_ENTRY_SIZE;
    for (uint32_t i = 0; i < n; i++)
    {
        memcpy(e[i].name, dir + i * GRP_ENTRY_SIZE, 12);
        e[i].name[12] = 0;
        const uint8_t *s = dir + i * GRP_ENTRY_SIZE + 12;
        e[i].size = s[0] | (s[1] << 8) | (s[2] << 16) | ((uint32_t) s[3] << 24);
        e[i].offset = offset;
        offset += e[i].size;
    }
    free(dir);
    *entries = e;
    *numFiles = n;
    return true;
}

// True if any file still needs the MCU conversion.
static bool needsConversion(FIL *f, const grpEntry_t *e, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        if (hasSuffix(e[i].name, ".VOC"))
        {
            return true;
        }
        if (hasSuffix(e[i].name, ".MID") && e[i].size >= 14)
        {
            uint8_t h[14];
            if (readAt(f, e[i].offset, h, sizeof(h)) && !memcmp(h, "MThd", 4) && h[8] == 0 && h[9] == 1)
            {
                return true;
            }
        }
    }
    return false;
}

static void makeStamp(const FILINFO *src, char *stamp, size_t len)
{
    snprintf(stamp, len, "%lu %u %u v1", (unsigned long) src->fsize, src->fdate, src->ftime);
}

static bool stampMatches(const char *drive, const char *stamp)
{
    char path[32], buf[64] = {0};
    FIL f;
    UINT got;
    snprintf(path, sizeof(path), "%s/" CONV_STAMP_NAME, drive);
    if (f_open(&f, path, FA_READ) != FR_OK)
    {
        return false;
    }
    bool ok = f_read(&f, buf, sizeof(buf) - 1, &got) == FR_OK && !strcmp(buf, stamp);
    f_close(&f);
    return ok;
}

static bool convert(const char *drive, FIL *src, grpEntry_t *e, uint32_t n)
{
    char tmpPath[32], convPath[32], stampPath[32];
    snprintf(tmpPath, sizeof(tmpPath), "%s/" CONV_TEMP_NAME, drive);
    snprintf(convPath, sizeof(convPath), "%s/" CONV_FILE_NAME, drive);
    snprintf(stampPath, sizeof(stampPath), "%s/" CONV_STAMP_NAME, drive);
    FIL dst;
    if (f_open(&dst, tmpPath, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        displayPrintln(1, "Can't create " CONV_TEMP_NAME);
        return false;
    }
    // Header and a placeholder directory; sizes are patched at the end.
    uint32_t dirSize = GRP_HEADER_SIZE + n * GRP_ENTRY_SIZE;
    uint8_t *dir = heap_caps_calloc(1, dirSize, MALLOC_CAP_SPIRAM);
    bool ok = dir != NULL;
    if (ok)
    {
        memcpy(dir, "KenSilverman", 12);
        putLe32(dir + 12, n);
        ok = writeAll(&dst, dir, dirSize);
    }
    uint32_t copied = 0, lastShown = 0, converted = 0;
    uint32_t total = e[n - 1].offset + e[n - 1].size;
    for (uint32_t i = 0; ok && i < n; i++)
    {
        uint8_t *entry = dir + GRP_HEADER_SIZE + i * GRP_ENTRY_SIZE;
        char name[13];
        strcpy(name, e[i].name);
        uint32_t outSize = e[i].size;
        bool isVoc = hasSuffix(name, ".VOC");
        bool isMid = hasSuffix(name, ".MID");
        if (isVoc || isMid)
        {
            uint8_t *data = heap_caps_malloc(e[i].size ? e[i].size : 1, MALLOC_CAP_SPIRAM);
            buf_t out = {0};
            ok = data && readAt(src, e[i].offset, data, e[i].size);
            int r = 0;
            if (ok && isVoc)
            {
                r = vocToWav(data, e[i].size, &out) ? 1 : -1;
                if (r > 0)
                {
                    memcpy(name + strlen(name) - 3, "WAV", 3);
                }
            }
            else if (ok)
            {
                r = midiToType0(data, e[i].size, &out);
            }
            if (ok && r > 0)
            {
                ok = writeAll(&dst, out.data, out.len);
                outSize = out.len;
                converted++;
            }
            else if (ok)
            {
                if (r < 0)
                {
                    ESP_LOGW(TAG, "%s: not converted, copied as is", name);
                }
                ok = writeAll(&dst, data, e[i].size);
            }
            free(data);
            bufFree(&out);
        }
        else
        {
            // Plain copy, streamed.
            ok = f_lseek(src, e[i].offset) == FR_OK;
            for (uint32_t left = e[i].size; ok && left;)
            {
                UINT chunk = left < IO_CHUNK ? left : IO_CHUNK;
                UINT got, put;
                ok = f_read(src, ioBuffer, chunk, &got) == FR_OK && got == chunk &&
                     f_write(&dst, ioBuffer, chunk, &put) == FR_OK && put == chunk;
                left -= chunk;
            }
        }
        memset(entry, 0, 12);
        memcpy(entry, name, strlen(name));
        putLe32(entry + 12, outSize);
        copied += e[i].size;
        if (copied - lastShown > 4 * 1024 * 1024 || i == n - 1)
        {
            displayPrintln(1, "  %d%%  (%d MB)", (int) ((uint64_t) copied * 100 / total), (int) (copied >> 20));
            lastShown = copied;
        }
    }
    if (ok)
    {
        ok = f_lseek(&dst, 0) == FR_OK && writeAll(&dst, dir, dirSize);
    }
    ok = (f_close(&dst) == FR_OK) && ok;
    free(dir);
    if (!ok)
    {
        f_unlink(tmpPath);
        displayPrintln(1, "Conversion failed (card full or read error).");
        return false;
    }
    f_unlink(convPath);
    if (f_rename(tmpPath, convPath) != FR_OK)
    {
        return false;
    }
    displayPrintln(1, "Converted %d sound/music files.", (int) converted);
    return true;
}

bool prepareGrp(const char *drive, char *grpPath, size_t grpPathLen)
{
    char srcPath[32], convPath[32];
    snprintf(srcPath, sizeof(srcPath), "%s/" DATA_FILE_NAME, drive);
    snprintf(convPath, sizeof(convPath), "%s/" CONV_FILE_NAME, drive);
    FILINFO srcInfo, convInfo;
    bool haveSrc = f_stat(srcPath, &srcInfo) == FR_OK;
    bool haveConv = f_stat(convPath, &convInfo) == FR_OK;
    if (!haveSrc)
    {
        if (haveConv)
        {
            snprintf(grpPath, grpPathLen, "%s", convPath);
            return true;
        }
        return false;
    }
    char stamp[64];
    makeStamp(&srcInfo, stamp, sizeof(stamp));
    if (haveConv && stampMatches(drive, stamp))
    {
        snprintf(grpPath, grpPathLen, "%s", convPath);
        return true;
    }
    ioBuffer = heap_caps_malloc(IO_CHUNK, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    FIL src;
    if (!ioBuffer || f_open(&src, srcPath, FA_READ) != FR_OK)
    {
        free(ioBuffer);
        ioBuffer = NULL;
        return false;
    }
    grpEntry_t *entries = NULL;
    uint32_t n = 0;
    bool ok = readDirectory(&src, &entries, &n);
    bool useConverted = false;
    if (ok && needsConversion(&src, entries, n))
    {
        DWORD freeClusters;
        FATFS *fs;
        uint64_t freeBytes = 0;
        if (f_getfree(drive, &freeClusters, &fs) == FR_OK)
        {
            freeBytes = (uint64_t) freeClusters * fs->csize * 512;
        }
        if (freeBytes < (uint64_t) srcInfo.fsize + (4 << 20))
        {
            displayPrintln(1, "Not enough free space on the card to convert.");
        }
        else
        {
            displayPrintln(1, "First run: converting sounds and music");
            displayPrintln(1, "for the MCU engine (one time, ~3 min)...");
            useConverted = convert(drive, &src, entries, n);
            if (useConverted)
            {
                char stampPath[32];
                FIL f;
                UINT put;
                snprintf(stampPath, sizeof(stampPath), "%s/" CONV_STAMP_NAME, drive);
                if (f_open(&f, stampPath, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
                {
                    f_write(&f, stamp, strlen(stamp), &put);
                    f_close(&f);
                }
            }
        }
    }
    f_close(&src);
    free(entries);
    free(ioBuffer);
    ioBuffer = NULL;
    snprintf(grpPath, grpPathLen, "%s", useConverted ? convPath : srcPath);
    if (ok && !useConverted)
    {
        ESP_LOGI(TAG, "using %s as is", srcPath);
    }
    return true;
}
