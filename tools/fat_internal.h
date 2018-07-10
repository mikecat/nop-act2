#ifndef FAT_INTERNAL_H_GUARD_63C1A43F_8C21_4AD9_B969_90A3066A6B3C
#define FAT_INTERNAL_H_GUARD_63C1A43F_8C21_4AD9_B969_90A3066A6B3C

#include "fat.h"

#define SECTOR_SIZE 512
#define ONE_RDE_SIZE 32
#define RDE_PER_SECTOR (SECTOR_SIZE / ONE_RDE_SIZE)

enum fattype_t {
	FAT12,
	FAT16,
	FAT32
};

struct fatinfo_t {
	/* disk & partition info */
	DISK* disk;
	size_t disk_start_sector;
	size_t disk_sector_num;
	/* FAT info */
	unsigned int sectors_per_cluster;
	unsigned int reserved_sectors;
	unsigned int number_of_fats;
	unsigned int root_entries;
	unsigned int total_sectors;
	unsigned int sectors_per_fat;
	/* FAT info cache */
	size_t first_fat_sector;
	size_t first_rde_sector;
	size_t first_data_sector;
	size_t data_cluster_num;
	enum fattype_t fat_type;
};

struct fatfile_common_t {
	/* implementation-defined data to handle the file */
	void* data;
	/* read size bytes to buffer and move file pointer forward */
	ssize_t (*read)(void* data, void* buffer, size_t size);
	/* write size bytes from buffer and move file pointer forward */
	ssize_t (*write)(void* data, const void* buffer, size_t size);
	/* get file size */
	ssize_t (*size)(void* data);
	/* move file pointer */
	ssize_t (*seek)(void* data, ssize_t value, int is_absolute);
	/* truncate file at current file pointer */
	int (*truncate)(void* data);
	/* close this file and destruct data */
	int (*close)(void* data);

	/* get file attributes */
	int (*get_attr)(void* data);
	/* set file attributes */
	int (*set_attr)(void* data, int attr);
	/* get last modify time */
	time_t (*get_lmtime)(void* data);
	/* set last modify time */
	int (*set_lmtime)(void* data, time_t lmtime);

	/* directory access need not be synchronized with byte access */
	/* start to deal with contents of this file as directory */
	int (*dir_begin)(void* data);
	/* get next directory entry (name unneeded -> put NULL) */
	int (*dir_next)(void* data, char* name, size_t name_max);
	/* open previously got directory entry */
	FATFILE* (*dir_openprev)(void* data, int usage);
	/* open file in this directory */
	FATFILE* (*dir_openfile)(void* data, const char* name, int usage);
	/* end dealing with contents of this file as directory */
	int (*dir_end)(void* data);
};

#endif
