#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "fat_internal.h"
#include "number_io.h"
#include "fatfile_native.h"
#include "fatfile_rde.h"

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
	if (!disk_read2(disk, start_sector, 0, 512, bpb)) {
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

FATFILE* fat_openfile(FATINFO* fi, FATFILE* curdir, const char* path, int usage) {
	if (path == NULL) return NULL;
	if (strncmp(path, "disk:", 5) == 0) {
		/* open file in disk image */
		FATFILE* cur = curdir;
		size_t cur_start = 5, i;
		char *fname_buf = malloc(strlen(path) + 1);
		if (fname_buf == NULL) return NULL;
		for (i = 5; ; i++) {
			if (path[i] == '/' || path[i] == '\\' || path[i] == '\0') {
				if (cur_start == i) {
					cur_start = i + 1;
				} else {
					/* open next directory or file */
					FATFILE* next;
					if (cur == NULL) {
						cur = fatfile_openrde(fi, FATFILE_WILL_READ);
						if (cur == NULL) {
							free(fname_buf);
							return NULL;
						}
					}
					memcpy(fname_buf, path + cur_start, i - cur_start);
					fname_buf[i - cur_start] = '\0';
					next = fat_diropenfile(cur, fname_buf, path[i] == '\0' ? usage : FATFILE_WILL_READ);
					if (cur != curdir) fat_closefile(cur);
					if (next == NULL) {
						free(fname_buf);
						return NULL;
					}
					cur = next;
				}
				if (path[i] == '\0') break;
			}
		}
		if (cur_start >= i) {
			/* end with directory separator, or the path is empty */
			if (cur == NULL) {
				cur = fatfile_openrde(fi, usage);
			} else {
				FATFILE* next = fat_diropenfile(cur, ".", usage);
				if (cur != curdir) fat_closefile(cur);
				cur = next;
			}
		}
		free(fname_buf);
		return cur;
	} else {
		/* open file in native file system */
		return fatfile_opennative(path, usage);
	}
}

int fat_closefile(FATFILE* ff) {
	if (ff != NULL) {
		if (ff->close != NULL && !ff->close(ff->data)) return 0;
		free(ff);
	}
	return 1;
}

ssize_t fat_readfile(FATFILE* ff, void* buffer, size_t size) {
	if (ff == NULL || ff->read == NULL) return -1;
	return ff->read(ff->data, buffer, size);
}

ssize_t fat_writefile(FATFILE* ff, const void* buffer, size_t size) {
	if (ff == NULL || ff->write == NULL) return -1;
	return ff->write(ff->data, buffer, size);
}

ssize_t fat_filesize(FATFILE* ff) {
	if (ff == NULL || ff->size == NULL) return -1;
	return ff->size(ff->data);
}

ssize_t fat_seekfile(FATFILE* ff, ssize_t value, int is_absolute) {
	if (ff == NULL || ff->seek == NULL) return -1;
	return ff->seek(ff->data, value, is_absolute);
}

int fat_truncatefile(FATFILE* ff) {
	if (ff == NULL || ff->truncate == NULL) return 0;
	return ff->truncate(ff->data);
}

int fat_deletefile(FATFILE* ff) {
	if (ff == NULL || ff->remove == NULL) return 0;
	return ff->remove(ff->data);
}

int fat_getfileattr(FATFILE* ff) {
	if (ff == NULL || ff->get_attr == NULL) return -1;
	return ff->get_attr(ff->data);
}

int fat_setfileattr(FATFILE* ff, int attr) {
	if (ff == NULL || ff->set_attr == NULL) return 0;
	return ff->set_attr(ff->data, attr);
}

time_t fat_getfilelmtime(FATFILE* ff) {
	if (ff == NULL || ff->get_lmtime == NULL) return 0;
	return ff->get_lmtime(ff->data);
}

int fat_setfilelmtime(FATFILE* ff, time_t lmtime) {
	if (ff == NULL || ff->set_lmtime == NULL) return 0;
	return ff->set_lmtime(ff->data, lmtime);
}

int fat_dirbegin(FATFILE* ff) {
	if (ff == NULL || ff->dir_begin == NULL) return 0;
	return ff->dir_begin(ff->data);
}

int fat_dirnext(FATFILE* ff, char* name, size_t name_max) {
	if (ff == NULL || ff->dir_next == NULL) return 0;
	return ff->dir_next(ff->data, name, name_max);
}

FATFILE* fat_diropenprev(FATFILE* ff, int usage) {
	if (ff == NULL || ff->dir_openprev == NULL) return NULL;
	return ff->dir_openprev(ff->data, usage);
}

FATFILE* fat_diropenfile(FATFILE* ff, const char* name, int usage) {
	if (ff == NULL || ff->dir_openfile == NULL) return NULL;
	return ff->dir_openfile(ff->data, name, usage);
}

int fat_dirend(FATFILE* ff) {
	if (ff == NULL || ff->dir_end == NULL) return 0;
	return ff->dir_end(ff->data);
}
