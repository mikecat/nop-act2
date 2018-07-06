#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "fat.h"
#include "number_io.h"

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

FATINFO* fat_open(DISK* disk, size_t start_sector, size_t sector_num) {
	FATINFO* fi;
	size_t disk_all_sector;
	char bpb[512];
	unsigned int bytes_per_sector;
	if (disk == NULL) { fprintf(stderr, "invalid disk handle\n"); return NULL; }
	if ((disk_all_sector = get_disk_sector_num(disk)) == 0) {
		fprintf(stderr, "failed to get disk size or disk too small\n"); return NULL;
	}
	if (disk_all_sector <= start_sector || disk_all_sector - start_sector < sector_num) {
		fprintf(stderr, "invald disk range\n"); return NULL;
	}
	if (!disk_read(disk, start_sector * 512, 512, bpb)) {
		fprintf(stderr, "BPB read failed\n"); return NULL;
	}
	if ((bytes_per_sector = read_number(bpb + 11, 2)) != 512) {
		fprintf(stderr, "sorry, non 512-byte (%u-byte) sector not supported.\n", bytes_per_sector);
		return NULL;
	}
	if ((fi = malloc(sizeof(*fi))) == NULL) { perror("malloc"); return NULL; }
	fi->disk_start_sector = start_sector;
	fi->disk_sector_num = sector_num;
	/* read BPB */
	fi->sectors_per_cluster = read_number(bpb + 13, 1);
	fi->reserved_sectors = read_number(bpb + 14, 2);
	fi->number_of_fats = read_number(bpb + 16, 1);
	fi->root_entries = read_number(bpb + 17, 2);
	fi->total_sectors = read_number(bpb + 19, 2);
	if (fi->total_sectors == 0) fi->total_sectors = read_number(bpb + 32, 4);
	fi->sectors_per_fat = read_number(bpb + 22, 2);
	/* check BPB parameters read */
	if (fi->sectors_per_cluster == 0) { fprintf(stderr, "sectors per cluster is 0\n"); free(fi); return NULL; }
	if (fi->reserved_sectors == 0) { fprintf(stderr, "reserved sectors is 0\n"); free(fi); return NULL; }
	if (fi->number_of_fats == 0) { fprintf(stderr, "number of FATs is 0\n"); free(fi); return NULL; }
	if (fi->root_entries == 0) { fprintf(stderr, "number of root entries is 0\n"); free(fi); return NULL; }
	if (fi->total_sectors == 0) { fprintf(stderr, "total sector number is 0\n"); free(fi); return NULL; }
	if (fi->sectors_per_fat == 0) { fprintf(stderr, "sectors per FAT is 0\n"); free(fi); return NULL; }
	if (fi->total_sectors > sector_num) {
		fprintf(stderr, "total sector number %u is larger than disk size\n", fi->total_sectors);
		free(fi); return NULL;
	}
	/* calculate parameter */
	if (fi->reserved_sectors > fi->total_sectors) {
		fprintf(stderr, "reserved sectors are more than total sectors\n");
		free(fi); return NULL;
	}
	fi->first_fat_sector = fi->reserved_sectors;
	if ((size_t)fi->sectors_per_fat * fi->number_of_fats > fi->total_sectors - fi->first_fat_sector) {
		fprintf(stderr, "FAT goes beyond end of total sectors\n");
		free(fi); return NULL;
	}
	fi->first_rde_sector = fi->first_fat_sector + (size_t)fi->sectors_per_fat * fi->number_of_fats;
	if ((fi->root_entries  + RDE_PER_SECTOR - 1) / RDE_PER_SECTOR > fi->total_sectors - fi->first_rde_sector) {
		fprintf(stderr, "RDE goes beyond end of total sectors\n");
		free(fi); return NULL;
	}
	fi->first_data_sector = fi->first_rde_sector + (fi->root_entries  + RDE_PER_SECTOR - 1) / RDE_PER_SECTOR;
	fi->data_cluster_num = (fi->total_sectors - fi->first_data_sector) / fi->sectors_per_cluster;
	fi->first_fat_sector += start_sector;
	fi->first_rde_sector += start_sector;
	fi->first_data_sector += start_sector;
	if (fi->data_cluster_num <= 4085) fi->fat_type = FAT12;
	else if (fi->data_cluster_num <= 65525) fi->fat_type = FAT16;
	else fi->fat_type = FAT32;

	return fi;
}

int fat_close(FATINFO* fi) {
	free(fi);
	return 1;
}

static void print_size(size_t size) {
	if (size < 1024)
		printf("%u bytes", (unsigned int)size);
	else if (size < 1024.0 * 1024.0)
		printf("%.1f KiB", (double)size / 1024.0);
	else if (size < 1024.0 * 1024.0 * 1024.0)
		printf("%.1f MiB", (double)size / (1024.0 * 1024.0));
	else if (size < 1024.0 * 1024.0 * 1024.0 * 1024.0)
		printf("%.1f GiB", (double)size / (1024.0 * 1024.0 * 1024.0));
	else
		printf("%.1f TiB", (double)size / (1024.0 * 1024.0 * 1024.0 * 1024.0));
}

int fat_printinfo(FATINFO* fi) {
	if (fi == NULL) return 0;
	printf("total size             : %u sectors (", fi->total_sectors);
	print_size((size_t)SECTOR_SIZE * fi->total_sectors); printf(")\n");
	printf("cluster size           : %u sectors (", fi->sectors_per_cluster);
	print_size((size_t)SECTOR_SIZE * fi->sectors_per_cluster); printf(")\n");
	printf("number of clusters     : %zu (", fi->data_cluster_num);
	print_size((size_t)SECTOR_SIZE * fi->sectors_per_cluster * fi->data_cluster_num); printf(")\n");
	printf("\n");
	printf("reserved sectors       : %u (", fi->reserved_sectors);
	print_size((size_t)SECTOR_SIZE * fi->reserved_sectors); printf(")\n");
	printf("one FAT size           : %u sectors (", fi->sectors_per_fat);
	print_size((size_t)SECTOR_SIZE * fi->sectors_per_fat); printf(")\n");
	printf("number of FATs         : %u\n", fi->number_of_fats);
	printf("total FAT size         : %u sectors (", fi->sectors_per_fat * fi->number_of_fats);
	print_size((size_t)SECTOR_SIZE * fi->sectors_per_fat * fi->number_of_fats); printf(")\n");
	printf("number of root entries : %u (", fi->root_entries);
	print_size((size_t)ONE_RDE_SIZE * fi->root_entries); printf(")\n");
	printf("FAT type               : ");
	switch (fi->fat_type) {
		case FAT12: printf("FAT12\n"); break;
		case FAT16: printf("FAT16\n"); break;
		case FAT32: printf("FAT32\n"); break;
		default: printf("(unknown)\n"); break;
	}
	printf("\n");
	printf("BPB        sector      : 0x%zX\n", fi->disk_start_sector);
	printf("FAT  first sector      : 0x%zX (offset: 0x%zX)\n", fi->first_fat_sector,
		fi->first_fat_sector - fi->disk_start_sector);
	printf("RDE  first sector      : 0x%zX (offset: 0x%zX)\n", fi->first_rde_sector,
		fi->first_rde_sector - fi->disk_start_sector);
	printf("data first sector      : 0x%zX (offset: 0x%zX)\n", fi->first_data_sector,
		fi->first_data_sector - fi->disk_start_sector);
	return 1;
}
