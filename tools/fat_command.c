#include <stdio.h>
#include <string.h>
#include "fat_command.h"

static void process_info(FATINFO* fi, int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	fat_printinfo(fi);
}

static void process_ls(FATINFO* fi, int argc, char* argv[]) {
	if (argc >= 1) {
		FATFILE* ff = fat_openfile(fi, NULL, argv[0], FATFILE_WILL_READ);
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

void fat_process_command(FATINFO* fi, int argc, char* argv[]) {
	if (argc >= 1) {
		if (strcmp(argv[0], "info") == 0) {
			process_info(fi, argc - 1, argv + 1);
		} else if (strcmp(argv[0], "ls") == 0) {
			process_ls(fi, argc - 1, argv + 1);
		}
	} else {
		puts("no command specified.\n");
	}
}
