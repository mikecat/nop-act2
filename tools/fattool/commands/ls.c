#include <stdio.h>
#include "../fat.h"
#include "commands.h"

void process_ls(FATINFO* fi, FATFILE* curdir, int argc, char* argv[]) {
	if (argc >= 1) {
		FATFILE* ff = fat_openfile(fi, curdir, argv[0], FATFILE_WILL_READ);
		if (ff == NULL) {
			fprintf(stderr, "failed to open %s\n", argv[0]);
		} else {
			int attr = fat_getfileattr(ff);
			if (attr < 0) {
				fprintf(stderr, "failed to get attribute\n");
			} else if (attr & FATFILE_ATTR_DIR) {
				char name[4096];
				if (fat_dirbegin(ff)) {
					while (fat_dirnext(ff, name, sizeof(name)) && name[0] != '\0') {
						printf("%s\n", name);
					}
					fat_dirend(ff);
				} else {
					fprintf(stderr, "fat_dirbegin() failed\n");
				}
			} else {
				printf("%s\n", argv[0]);
			}
			fat_closefile(ff);
		}
	}
}
