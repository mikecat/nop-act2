#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include "fatfile_native.h"
#include "fat_internal.h"

struct native_file_t {
	int fd;
	int dirmode;
	DIR* dir;
	char prev_name[2048];
	/* for use in unlinkat */
	char* my_path;
	int my_dir_fd;
};

static ssize_t native_read(void* data, void* buffer, size_t size) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL || buffer == NULL || nf->dirmode) return -1;
	return read(nf->fd, buffer, size);
}

static ssize_t native_write(void* data, const void* buffer, size_t size) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL || buffer == NULL || nf->dirmode) return -1;
	return write(nf->fd, buffer, size);
}

static ssize_t native_size(void* data) {
	struct native_file_t* nf = (struct native_file_t*)data;
	struct stat st;
	if (nf == NULL) return -1;
	if (fstat(nf->fd, &st) == -1) return -1;
	return st.st_size;
}

static ssize_t native_seek(void* data, ssize_t value, int is_absolute) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL || nf->dirmode) return -1;
	return lseek(nf->fd, value, is_absolute ? SEEK_SET : SEEK_CUR);
}

static int native_truncate(void* data) {
	struct native_file_t* nf = (struct native_file_t*)data;
	off_t length;
	if (nf == NULL || nf->dirmode) return -1;
	if ((length = lseek(nf->fd, 0, SEEK_CUR)) == (off_t)-1) return -1;
	return ftruncate(nf->fd, length) == 0;
}

static int native_remove_recur(int dir_fd, const char* path) {
	struct stat st;
	if (fstatat(dir_fd, path, &st, 0) == -1) return 0;
	if ((st.st_mode & S_IFMT) == S_IFDIR) {
		int fd = openat(dir_fd, path, O_RDWR | O_DIRECTORY), fd2;
		DIR* dir;
		if (fd == -1) return 0;
		if ((fd2 = dup(fd)) == -1) {
			close(fd);
			return 0;
		}
		if ((dir = fdopendir(fd)) == NULL) {
			int e = errno; close(fd); close(fd2); errno = e;
			return 0;
		}
		for (;;) {
			struct dirent *de;
			errno = 0;
			de = readdir(dir);
			if (de == NULL) {
				if (errno != 0) {
					/* error */
					int e = errno; closedir(dir); close(fd2); errno = e;
					return 0;
				} else {
					/* end of directory stream */
					break;
				}
			}
			if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
				if (!native_remove_recur(fd2, de->d_name)) {
					int e = errno; closedir(dir); close(fd2); errno = e;
					return 0;
				}
			}
		}
		closedir(dir);
		close(fd2);
		return unlinkat(dir_fd, path, AT_REMOVEDIR) == 0;
	} else {
		return unlinkat(dir_fd, path, 0) == 0;
	}
}

static int native_remove(void* data) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL) return 0;
	return native_remove_recur(nf->my_dir_fd, nf->my_path);
}

static int native_close(void* data) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL) return 0;
	if (nf->dirmode) {
		if (closedir(nf->dir) == -1) return 0;
	}
	if (close(nf->fd) == -1) return 0;
	free(nf->my_path);
	close(nf->my_dir_fd);
	free(data);
	return 1;
}

static int native_get_attr(void* data) {
	struct native_file_t* nf = (struct native_file_t*)data;
	struct stat st;
	int attr = 0;
	if (nf == NULL) return -1;
	if (fstat(nf->fd, &st) == -1) return -1;
	if ((st.st_mode & S_IFMT) == S_IFDIR) attr |= FATFILE_ATTR_DIR;
	if (!(st.st_mode & S_IWUSR)) attr |= FATFILE_ATTR_READONLY;
	return attr;
}

static int native_set_attr(void* data, int attr) {
	struct native_file_t* nf = (struct native_file_t*)data;
	struct stat st;
	mode_t mode;
	if (nf == NULL) return 0;
	if (fstat(nf->fd, &st) == -1) return 0;
	mode = st.st_mode & 07777;
	if (attr & FATFILE_ATTR_READONLY) mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	else mode |= S_IWUSR;
	return fchmod(nf->fd, mode) == 0;
}

static time_t native_get_lmtime(void* data) {
	struct native_file_t* nf = (struct native_file_t*)data;
	struct stat st;
	if (nf == NULL) return 0;
	if (fstat(nf->fd, &st) == -1) return 0;
	return st.st_mtime;
}

static int native_set_lmtime(void* data, time_t lmtime) {
	struct native_file_t* nf = (struct native_file_t*)data;
	struct stat st;
	struct timeval times[2];
	if (nf == NULL) return 0;
	if (fstat(nf->fd, &st) == -1) return 0;
	times[0].tv_sec = st.st_atime; times[0].tv_usec = 0;
	times[1].tv_sec = lmtime; times[1].tv_usec = 0;
	return futimes(nf->fd, times) == 0;
}

static int native_dir_begin(void* data) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL) return 0;
	if (nf->dirmode) {
		if (nf->dir == NULL) return 0;
		rewinddir(nf->dir);
	} else {
		int fd2 = dup(nf->fd);
		if (fd2 == -1) return 0;
		if ((nf->dir = fdopendir(fd2)) == NULL) return 0;
		nf->dirmode = 1;
	}
	nf->prev_name[0] = '\0';
	return 1;
}

static int native_dir_next(void* data, char* name, size_t name_max) {
	struct native_file_t* nf = (struct native_file_t*)data;
	struct dirent* dirent;
	if (nf == NULL || !nf->dirmode || nf->dir == NULL) return 0;
	errno = 0;
	dirent = readdir(nf->dir);
	if (dirent == NULL && errno != 0) return 0;
	if (name != NULL) {
		if (dirent == NULL) {
			if (name_max > 0) name[0] = '\0';
		} else {
			strncpy(name, dirent->d_name, name_max);
			strncpy(nf->prev_name, dirent->d_name, sizeof(nf->prev_name));
		}
	}
	return 1;
}

FATFILE* fatfile_opennative_common(int dirfd, const char* path, int usage);

static FATFILE* native_dir_openprev(void* data, int usage) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL || !nf->dirmode) return 0;
	if (nf->prev_name[0] == '\0' || strpbrk(nf->prev_name, "\\/") != NULL) return 0;
	return fatfile_opennative_common(nf->fd, nf->prev_name, usage);
}

static FATFILE* native_dir_openfile(void* data, const char* name, int usage) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL || name == NULL || !nf->dirmode) return 0;
	if (strpbrk(name, "\\/") != NULL) return 0;
	return fatfile_opennative_common(nf->fd, name, usage);
}

static int native_dir_end(void* data) {
	struct native_file_t* nf = (struct native_file_t*)data;
	if (nf == NULL || !nf->dirmode || nf->dir == NULL) return 0;
	if (closedir(nf->dir) != 0) return 0;
	nf->dirmode = 0;
	nf->dir = NULL;
	nf->prev_name[0] = '\0';
	return 1;
}

FATFILE* fatfile_opennative_common(int dirfd, const char* path, int usage) {
	int open_flag = 0;
	struct native_file_t* nf = malloc(sizeof(*nf));
	FATFILE* ff = malloc(sizeof(*ff));
	struct stat st;
	if (nf == NULL || ff == NULL) {
		int e = errno; free(nf); free(ff); errno = e;
		return NULL;
	}
	if (path == NULL) {
		free(nf); free(ff);
		return NULL;
	}
	if ((usage & FATFILE_WILL_READ) && (usage & FATFILE_WILL_WRITE)) {
		open_flag = O_RDWR | O_CREAT;
	} else if (usage & FATFILE_WILL_READ) {
		open_flag = O_RDONLY;
	} else if (usage & FATFILE_WILL_WRITE) {
		open_flag = O_WRONLY | O_CREAT;
	} else {
		free(nf); free(ff);
		return NULL;
	}
	if ((nf->my_path = malloc(strlen(path) + 1)) == NULL) {
		free(nf); free(ff);
		return NULL;
	}
	strcpy(nf->my_path, path);
	nf->my_dir_fd = (dirfd == -1 ? open(".", O_RDONLY) : dup(dirfd));
	if (nf->my_dir_fd == -1) {
		free(nf->my_path); free(nf); free(ff);
		return NULL;
	}
	if (fstatat(nf->my_dir_fd, path, &st, 0) == -1) {
		if (errno != ENOENT) {
			int e = errno; close(nf->my_dir_fd); free(nf->my_path); free(nf); free(ff); errno = e;
			return NULL;
		}
	} else {
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			open_flag = O_RDONLY;
		}
	}
	if ((nf->fd = openat(nf->my_dir_fd, path, open_flag, 0644)) == -1) {
		int e = errno; close(nf->my_dir_fd); free(nf->my_path); free(nf); free(ff); errno = e;
		return NULL;
	}
	nf->dirmode = 0;
	nf->dir = NULL;
	nf->prev_name[0] = '\0';

	ff->data = nf;
	ff->read = native_read;
	ff->write = native_write;
	ff->size = native_size;
	ff->seek = native_seek;
	ff->truncate = native_truncate;
	ff->remove = native_remove;
	ff->close = native_close;
	ff->get_attr = native_get_attr;
	ff->set_attr = native_set_attr;
	ff->get_lmtime = native_get_lmtime;
	ff->set_lmtime = native_set_lmtime;
	ff->dir_begin = native_dir_begin;
	ff->dir_next = native_dir_next;
	ff->dir_openprev = native_dir_openprev;
	ff->dir_openfile = native_dir_openfile;
	ff->dir_end = native_dir_end;

	return ff;
}

FATFILE* fatfile_opennative(const char* path, int usage) {
	return fatfile_opennative_common(-1, path, usage);
}
