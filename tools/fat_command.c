#include <stdio.h>
#include <string.h>
#include "fat_command.h"

void fat_process_command(FATINFO* fi, int argc, char* argv[]) {
	if (argc >= 1) {
		if (strcmp(argv[0], "info") == 0) {
			fat_printinfo(fi);
		} else if (strcmp(argv[0], "ls") == 0) {
			if (argc >= 2) {
				FATFILE* ff = fat_openfile(fi, NULL, argv[1], FATFILE_WILL_READ);
				if (ff == NULL) {
					fprintf(stderr, "failed to open %s\n", argv[1]);
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
						printf("%s\n", argv[1]);
					}
					fat_closefile(ff);
				}
			}
		}
	} else {
		puts("no command specified.\n");
	}
}
