#ifndef FAT_H_GUARD_5472980D_3181_497F_8800_678ACCC59F87
#define FAT_H_GUARD_5472980D_3181_497F_8800_678ACCC59F87

#include <stddef.h>
#include "disk.h"

typedef struct fatinfo_t FATINFO;

/* return NULL if fails */
FATINFO* fat_open(DISK* disk, size_t start_sector, size_t sector_num);
/* return nonzero if success, return zero if fails */
int fat_close(FATINFO* fi);
/* return nonzero if success, return zero if fails */
int fat_printinfo(FATINFO* fi);

#endif
