#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include "disk.h"

#define SECTOR_SIZE 512

#define DISK_CACHE_SIZE 64

struct disk_t {
#ifdef _WIN32
	HANDLE hFile;
#else
	int fd;
#endif
	size_t size;

	ssize_t cached_sector[DISK_CACHE_SIZE];
	unsigned char cache_order[DISK_CACHE_SIZE];
	unsigned char cache_dirty[DISK_CACHE_SIZE];
	char cache[DISK_CACHE_SIZE][SECTOR_SIZE];
};

/* reset cache entry (no write back even if dirty) */
static int clear_cache(DISK* disk) {
	int i;
	if (disk == NULL) return 0;
	for (i = 0; i < DISK_CACHE_SIZE; i++) {
		disk->cached_sector[i] = -1;
		disk->cache_order[i] = DISK_CACHE_SIZE;
		disk->cache_dirty[i] = 0;
	}
	return 1;
}

/* write all ditry cache entry to the file */
static int flash_cache(DISK* disk) {
	int i;
	if (disk == NULL) return 0;
	for (i = 0; i < DISK_CACHE_SIZE; i++) {
		if (disk->cache_dirty[i]) {
#ifdef _WIN32
			off_t seekTo = disk->cached_sector[i] * (off_t)SECTOR_SIZE;
			LONG seekLow = seekTo & 0xffffffffu, seekHigh = (seekTo >> 16) >> 16;
			DWORD sizeWritten = 0;
			if (SetFilePointer(disk->hFile, seekLow, &seekHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
			GetLastError() != NO_ERROR) {
				return 0;
			}
 			if (!WriteFile(disk->hFile, disk->cache[i], SECTOR_SIZE, &sizeWritten, NULL) ||
 			sizeWritten != SECTOR_SIZE) {
 				return 0;
 			}
 #else
			if (lseek(disk->fd, disk->cached_sector[i] * (off_t)SECTOR_SIZE, SEEK_SET) == (off_t)-1) {
				return 0;
			}
			if (write(disk->fd, disk->cache[i], SECTOR_SIZE) != SECTOR_SIZE) {
				return 0;
			}
#endif
			disk->cache_dirty[i] = 0;
		}
	}
	return 1;
}

/* get cache entry from sector number */
enum dirty_level {
	CLEAN = 0, /* will just read this sector */
	DIRTY_PART, /* will write a part of this sector */
	DIRTY_ALL /* will write a whole sector */
};
static char* get_cache(DISK* disk, ssize_t sector, enum dirty_level dirty_flag) {
	int i, j;
	int max_id = 0;
	if (disk == NULL || sector < 0 || (disk->size / SECTOR_SIZE) <= sector) return 0;
	for (i = 0; i < DISK_CACHE_SIZE; i++) {
		if (disk->cached_sector[i] == sector) {
			/* hit */
			for (j = 0; j < DISK_CACHE_SIZE; j++) {
				if (disk->cache_order[j] < disk->cache_order[i]) disk->cache_order[j]++;
			}
			disk->cache_order[i] = 0;
			if (dirty_flag != CLEAN) disk->cache_dirty[i] = 1;
			return disk->cache[i];
		} else {
			/* search for least recently used entry */
			if (disk->cache_order[i] > disk->cache_order[max_id]) {
				max_id = i;
			}
		}
	}
	/* miss */
	i = max_id;
	for (j = 0; j < DISK_CACHE_SIZE; j++) {
		if (disk->cache_order[j] < DISK_CACHE_SIZE) disk->cache_order[j]++;
	}
	disk->cache_order[i] = 0;
	if (disk->cache_dirty[i]) {
		/* write updated data into file */
#ifdef _WIN32
		off_t seekTo = disk->cached_sector[i] * (off_t)SECTOR_SIZE;
		LONG seekLow = seekTo & 0xffffffffu, seekHigh = (seekTo >> 16) >> 16;
		DWORD sizeWritten = 0;
		if (SetFilePointer(disk->hFile, seekLow, &seekHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
		GetLastError() != NO_ERROR) {
			return NULL;
		}
		if (!WriteFile(disk->hFile, disk->cache[i], SECTOR_SIZE, &sizeWritten, NULL) ||
		sizeWritten != SECTOR_SIZE) {
			return NULL;
		}
#else
		if (lseek(disk->fd, disk->cached_sector[i] * (off_t)SECTOR_SIZE, SEEK_SET) == (off_t)-1) {
			return NULL;
		}
		if (write(disk->fd, disk->cache[i], SECTOR_SIZE) != SECTOR_SIZE) return NULL;
#endif
	}
	disk->cache_dirty[i] = (dirty_flag != CLEAN);
	disk->cached_sector[i] = sector;
	if (dirty_flag != DIRTY_ALL) {
		/* read new data from file */
#ifdef _WIN32
		off_t seekTo = disk->cached_sector[i] * (off_t)SECTOR_SIZE;
		LONG seekLow = seekTo & 0xffffffffu, seekHigh = (seekTo >> 16) >> 16;
		DWORD sizeRead = 0;
		if (SetFilePointer(disk->hFile, seekLow, &seekHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
		GetLastError() != NO_ERROR) {
			return NULL;
		}
		if (!ReadFile(disk->hFile, disk->cache[i], SECTOR_SIZE, &sizeRead, NULL) ||
		sizeRead != SECTOR_SIZE) {
			return NULL;
		}
#else
		if (lseek(disk->fd, disk->cached_sector[i] * (off_t)SECTOR_SIZE, SEEK_SET) == (off_t)-1) {
			return NULL;
		}
		if (read(disk->fd, disk->cache[i], SECTOR_SIZE) != SECTOR_SIZE) return NULL;
#endif
	}
	return disk->cache[i];
}

DISK* open_disk(const char* disk_file) {
#ifdef _WIN32
	DWORD sizeLow, sizeHigh = 0;
#else
	struct stat st;
#endif
	DISK* disk = malloc(sizeof(*disk));
	if (disk == NULL) return NULL;
#ifdef _WIN32
	disk->hFile = CreateFileA(disk_file, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (disk->hFile == INVALID_HANDLE_VALUE) {
		free(disk);
		return NULL;
	}
	sizeLow = GetFileSize(disk->hFile, &sizeHigh);
	if (sizeLow == (DWORD)-1 && GetLastError() != NO_ERROR) {
		CloseHandle(disk->hFile);
		free(disk);
		return NULL;
	}
	disk->size = sizeLow | (((size_t)sizeHigh << 16) << 16);
#else
	if ((disk->fd = open(disk_file, O_RDWR)) == -1) {
		int e = errno;
		free(disk);
		errno = e;
		return NULL;
	}
	if (fstat(disk->fd, &st) == -1) {
		int e = errno;
		close(disk->fd);
		free(disk);
		errno = e;
		return NULL;
	}
	disk->size = st.st_size;
#endif
	if (!clear_cache(disk)) {
#ifdef _WIN32
		CloseHandle(disk->hFile);
#else
		close(disk->fd);
#endif
		free(disk);
		return NULL;
	}
	return disk;
}

int close_disk(DISK* disk) {
	if (disk == NULL) return 0;
	if (!flash_cache(disk)) return 0;
#ifdef _WIN32
	if (!CloseHandle(disk->hFile)) return 0;
#else
	if (close(disk->fd) == -1) return 0;
#endif
	free(disk);
	return 1;
}

size_t get_disk_sector_num(DISK* disk) {
	if (disk == NULL) return 0;
	return disk->size / SECTOR_SIZE;
}

int disk_read(DISK* disk, size_t offset, size_t len, void* data) {
	char* cdata = (char*)data;
	size_t sector = offset / SECTOR_SIZE;
	char* sector_cache;
	if (disk == NULL || cdata == NULL) return 0;
	if (offset % SECTOR_SIZE != 0) {
		size_t sector_offset = offset % SECTOR_SIZE;
		size_t left_size = SECTOR_SIZE - sector_offset;
		if ((sector_cache = get_cache(disk, sector, CLEAN)) == NULL) return 0;
		if (len < left_size) {
			memcpy(cdata, sector_cache + sector_offset, len);
			len = 0;
		} else {
			memcpy(cdata, sector_cache + sector_offset, left_size);
			cdata += left_size;
			len -= left_size;
		}
		sector++;
	}
	while (len > 0) {
		if ((sector_cache = get_cache(disk, sector, CLEAN)) == NULL) return 0;
		if (len < SECTOR_SIZE) {
			memcpy(cdata, sector_cache, len);
			len = 0;
		} else {
			memcpy(cdata, sector_cache, SECTOR_SIZE);
			cdata += SECTOR_SIZE;
			len -= SECTOR_SIZE;
		}
		sector++;
	}
	return 1;
}

int disk_write(DISK* disk, size_t offset, size_t len, const void* data) {
	char* cdata = (char*)data;
	size_t sector = offset / SECTOR_SIZE;
	char* sector_cache;
	if (disk == NULL || cdata == NULL) return 0;
	if (offset % SECTOR_SIZE != 0) {
		size_t sector_offset = offset % SECTOR_SIZE;
		size_t left_size = SECTOR_SIZE - sector_offset;
		if ((sector_cache = get_cache(disk, sector, DIRTY_PART)) == NULL) return 0;
		if (len < left_size) {
			memcpy(sector_cache + sector_offset, cdata, len);
			len = 0;
		} else {
			memcpy(sector_cache + sector_offset, cdata, left_size);
			cdata += left_size;
			len -= left_size;
		}
		sector++;
	}
	while (len > 0) {
		if ((sector_cache = get_cache(disk, sector,
			len >= SECTOR_SIZE ? DIRTY_ALL : DIRTY_PART)) == NULL) return 0;
		if (len < SECTOR_SIZE) {
			memcpy(sector_cache, cdata, len);
			len = 0;
		} else {
			memcpy(sector_cache, cdata, SECTOR_SIZE);
			cdata += SECTOR_SIZE;
			len -= SECTOR_SIZE;
		}
		sector++;
	}
	return 1;
}
int disk_read2(DISK* disk, size_t start_sector, size_t offset, size_t len, void* data) {
	return disk_read(disk, start_sector * SECTOR_SIZE + offset, len, data);
}

int disk_write2(DISK* disk, size_t start_sector, size_t offset, size_t len, const void* data) {
	return disk_write(disk, start_sector * SECTOR_SIZE + offset, len, data);
}
