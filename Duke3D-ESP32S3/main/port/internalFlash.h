#ifndef INTERNAL_FLASH_H
#define INTERNAL_FLASH_H
void eraseInternalFlash(int sections);
#if WIN32
void * storeToInternalFlash2(void *buffer, int size, char *function, int line);
#define storeToInternalFlash(b, s) storeToInternalFlash2(b, s, __FUNCTION__, __LINE__)
#else
void * storeToInternalFlash(const void *buffer, int size);
#endif
void storeToInternalFlashAndFree(void **buffer, int size);
void *getCurrentInternalFlashPtr(void);
void * storeToInternalFlashAlign(void *buffer, int size);
void *reserveInternalFlashSize(uint32_t size);
// note: also destination (flashpos) is const, because it will be on flash anyway afterwards.
void * storeToInternalFlashAtPointer(const void *buffer, const void *flashPos, int size);
//
void internalFlashResetToCommonZoneEnd(void);
void internalFlashSetCommonZone(void);
void internalFlashInit(void);
void flashErasePage(uint32_t *pageAddress);
int internalFlashSpaceLeft(void);
#endif // INTERNAL_FLASH_H
