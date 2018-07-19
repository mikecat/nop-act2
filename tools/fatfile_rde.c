#include <stdlib.h>
#include <errno.h>
#include "fatfile_rde.h"
#include "fat_internal.h"
#include "disk.h"

struct rde_file_t {
	FATINFO* fi;
	int usage;
	size_t size;
	size_t fp;
	int dirmode;
	size_t dir_fp;
	size_t prev_dir_fp;
};

static ssize_t rde_read(void* data, void* buffer, size_t size) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	size_t size_to_read;
	if (rf == NULL || buffer == NULL || rf->dirmode) return -1;
	if (!(rf->usage & FATFILE_WILL_READ)) return -1;
	size_to_read = size;
	if (rf->fp > rf->size) size_to_read = 0;
	else if (size_to_read > rf->size - rf->fp) size_to_read = rf->size - rf->fp;
	if (size_to_read > 0) {
		if (!disk_read2(rf->fi->disk, rf->fi->first_rde_sector, rf->fp, size_to_read, buffer)) {
			return -1;
		}
		rf->fp += size_to_read;
	}
	return size_to_read;
}

static ssize_t rde_write(void* data, const void* buffer, size_t size) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	size_t size_to_write;
	if (rf == NULL || buffer == NULL || rf->dirmode) return -1;
	if (!(rf->usage & FATFILE_WILL_WRITE)) return -1;
	size_to_write = size;
	if (rf->fp > rf->size) size_to_write = 0;
	else if (size_to_write > rf->size - rf->fp) size_to_write = rf->size - rf->fp;
	if (size_to_write > 0) {
		if (!disk_write2(rf->fi->disk, rf->fi->first_rde_sector, rf->fp, size_to_write, buffer)) {
			return -1;
		}
		rf->fp += size_to_write;
	}
	return size_to_write;
}

static ssize_t rde_size(void* data) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	if (rf == NULL) return -1;
	return rf->size;
}

static ssize_t rde_seek(void* data, ssize_t value, int is_absolute) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	ssize_t dest;
	if (rf == NULL || rf->dirmode) return -1;
	dest = is_absolute ? value : rf->fp + value;
	if (dest < 0) dest = 0;
	if (dest > rf->size) dest = rf->size;
	rf->fp = dest;
	return rf->fp;
}

static int rde_truncate(void* data) {
	/* unsupported for RDE */
	(void)data;
	return 0;
}

static int rde_remove(void* data) {
	/* unsupported for RDE */
	(void)data;
	return 0;
}

static int rde_close(void* data) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	if (rf == NULL) return 0;
	free(data);
	return 1;
}

static int rde_get_attr(void* data) {
	/* RDE is a directory */
	(void)data;
	return FATFILE_ATTR_DIR;
}

static int rde_set_attr(void* data, int attr) {
	/* unsupported for RDE */
	(void)data;
	(void)attr;
	return 0;
}

static time_t rde_get_lmtime(void* data) {
	/* unsupported for RDE */
	(void)data;
	return 0;
}

static int rde_set_lmtime(void* data, time_t lmtime) {
	/* unsupported for RDE */
	(void)data;
	(void)lmtime;
	return 0;
}

static int rde_dir_begin(void* data) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	if (rf == NULL) return 0;
	rf->dirmode = 1;
	rf->dir_fp = 0;
	rf->prev_dir_fp = rf->size;
	return 1;
}

static int rde_dir_next(void* data, char* name, size_t name_max) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	struct dirent* dirent;
	if (rf == NULL || !rf->dirmode) return 0;
	if (!(rf->usage & FATFILE_WILL_READ)) return 0;
	while (rf->dir_fp + ONE_RDE_SIZE <= rf->size) {
		char rde[ONE_RDE_SIZE];
		if (disk_read2(rf->fi->disk, rf->fi->first_rde_sector, rf->dir_fp, ONE_RDE_SIZE, rde)) {
			if (rde[0] == 0x00) {
				/* end of the table */
				break;
			} else if ((unsigned char)rde[0] != 0xe5) {
				/* not deleted */
				if (name != NULL && name_max > 0) {
					unsigned int i, name_pos = 0;
					for (i = 0; i < 11; i++) {
						if (name_pos < name_max) {
							if (rde[i] == '\0') {
								if (i < 8) i = 7; else break;
							} else {
								name[name_pos++] = rde[i];
							}
						}
						if (i == 7 && name_pos < name_max) name[name_pos++] = '.';
					}
					if (name_pos >= name_max) name_pos = name_max - 1;
					name[name_pos] = '\0';
					if (name[0] == 0x05) name[0] = 0xe5;
				}
				rf->prev_dir_fp = rf->dir_fp;
				rf->dir_fp += ONE_RDE_SIZE;
				return 1;
			}
		} else {
			return 0;
		}
		rf->dir_fp += ONE_RDE_SIZE;
	}
	if (name != NULL && name_max > 0) name[0] = '\0';
	return 1;
}

static FATFILE* rde_dir_openprev(void* data, int usage) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	if (rf == NULL || !rf->dirmode) return 0;
	/* not implemented yet */
	(void)usage;
	return NULL;
}

static FATFILE* rde_dir_openfile(void* data, const char* name, int usage) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	if (rf == NULL || name == NULL || !rf->dirmode) return 0;
	/* not implemented yet */
	(void)usage;
	return NULL;
}

static int rde_dir_end(void* data) {
	struct rde_file_t* rf = (struct rde_file_t*)data;
	if (rf == NULL || !rf->dirmode) return 0;
	rf->dirmode = 0;
	return 1;
}

FATFILE* fatfile_openrde(FATINFO* fi, int usage) {
	struct rde_file_t* rf = malloc(sizeof(*rf));
	FATFILE* ff = malloc(sizeof(*ff));
	if (rf == NULL || ff == NULL) {
		int e = errno; free(rf); free(ff); errno = e;
		return NULL;
	}
	if (fi == NULL) {
		free(rf); free(ff);
		return NULL;
	}
	if (fi->fat_type != FAT12 && fi->fat_type != FAT16) {
		/* unsupported FAT type */
		free(rf); free(ff);
		return NULL;
	}
	rf->fi = fi;
	rf->usage = usage;
	rf->size = ONE_RDE_SIZE * fi->root_entries;
	rf->fp = 0;
	rf->dirmode = 0;
	rf->dir_fp = 0;
	rf->prev_dir_fp = rf->size;

	ff->data = rf;
	ff->read = rde_read;
	ff->write = rde_write;
	ff->size = rde_size;
	ff->seek = rde_seek;
	ff->truncate = rde_truncate;
	ff->remove = rde_remove;
	ff->close = rde_close;
	ff->get_attr = rde_get_attr;
	ff->set_attr = rde_set_attr;
	ff->get_lmtime = rde_get_lmtime;
	ff->set_lmtime = rde_set_lmtime;
	ff->dir_begin = rde_dir_begin;
	ff->dir_next = rde_dir_next;
	ff->dir_openprev = rde_dir_openprev;
	ff->dir_openfile = rde_dir_openfile;
	ff->dir_end = rde_dir_end;

	return ff;
}
