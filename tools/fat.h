#ifndef FAT_H_GUARD_5472980D_3181_497F_8800_678ACCC59F87
#define FAT_H_GUARD_5472980D_3181_497F_8800_678ACCC59F87

#include <time.h>
#include <sys/types.h>
#include "disk.h"

typedef struct fatinfo_t FATINFO;
typedef struct fatfile_common_t FATFILE;

/* file attribute flags */
#define FATFILE_ATTR_READONLY 0x01
#define FATFILE_ATTR_HIDDEN   0x02
#define FATFILE_ATTR_SYSTEM   0x04
#define FATFILE_ATTR_DIR      0x10
#define FATFILE_ATTR_ARCHIVE  0x20
/* file usage flags */
#define FATFILE_WILL_READ  1
#define FATFILE_WILL_WRITE 2

/* return NULL if fails */
FATINFO* fat_open(DISK* disk, size_t start_sector, size_t sector_num);
/* return nonzero if success, return zero if fails */
int fat_close(FATINFO* fi);
/* return nonzero if success, return zero if fails */
int fat_printinfo(FATINFO* fi);

/* return NULL if fails */
FATFILE* fat_openfile(FATINFO* fi, FATFILE* curdir, const char* path, int usage);
/* return nonzero if success, return zero if fails */
int fat_closefile(FATFILE* ff);
/* return size read if success, return -1 if fails */
ssize_t fat_readfile(FATFILE* ff, void* buffer, size_t size);
/* return size wrote if success, return -1 if fails */
ssize_t fat_writefile(FATFILE* ff, const void* buffer, size_t size);
/* return -1 if fails */
ssize_t fat_filesize(FATFILE* ff);
/* return new position if success, -1 if fails */
ssize_t fat_seekfile(FATFILE* ff, ssize_t value, int is_absolute);
/* return nonzero if success, return zero if fails */
int fat_truncatefile(FATFILE* ff);
/* return nonzero if success, return zero if fails */
int fat_deletefile(FATFILE* ff);
/* return -1 if fails */
int fat_getfileattr(FATFILE* ff);
/* return nonzero if success, return zero if fails */
int fat_setfileattr(FATFILE* ff, int attr);
/* return 0 if fails */
time_t fat_getfilelmtime(FATFILE* ff);
/* return nonzero if success, return zero if fails */
int fat_setfilelmtime(FATFILE* ff, time_t lmtime);
/* return nonzero if success, return zero if fails */
int fat_dirbegin(FATFILE* ff);
/* return nonzero if success, return zero if fails */
int fat_dirnext(FATFILE* ff, char* name, size_t name_max);
/* return NULL if fails */
FATFILE* fat_diropenprev(FATFILE* ff, int usage);
/* return NULL if fails */
FATFILE* fat_diropenfile(FATFILE* ff, const char* name, int usage);
/* return nonzero if success, return zero if fails */
int fat_dirend(FATFILE* ff);

#endif
