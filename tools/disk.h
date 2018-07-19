#ifndef DISK_H_GUARD_891D9C68_72A2_40C8_9421_EE2395A9FA82
#define DISK_H_GUARD_891D9C68_72A2_40C8_9421_EE2395A9FA82

#include <stddef.h>

typedef struct disk_t DISK;

/* success: return opened handle, fail: return NULL */
DISK* open_disk(const char* disk_file);
/* success: return nonzero, fail: return zero */
int close_disk(DISK* disk);
/* success: return # of sector, fail: return zero */
size_t get_disk_sector_num(DISK* disk);
/* success: return nonzero, fail: return zero */
int disk_read(DISK* disk, size_t offset, size_t len, void* data);
/* success: return nonzero, fail: return zero */
int disk_write(DISK* disk, size_t offset, size_t len, const void* data);
/* success: return nonzero, fail: return zero */
int disk_read2(DISK* disk, size_t start_sector, size_t offset, size_t len, void* data);
/* success: return nonzero, fail: return zero */
int disk_write2(DISK* disk, size_t start_sector, size_t offset, size_t len, const void* data);

#endif
