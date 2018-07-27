#include <stdio.h>
#include <string.h>
#include "fat_command.h"
#include "commands/commands.h"

void fat_process_command(FATINFO* fi, FATFILE* curdir, int argc, char* argv[]) {
	if (argc >= 1) {
		if (strcmp(argv[0], "info") == 0) {
			process_info(fi, curdir, argc - 1, argv + 1);
		} else if (strcmp(argv[0], "ls") == 0) {
			process_ls(fi, curdir, argc - 1, argv + 1);
		}
	} else {
		puts("no command specified.\n");
	}
}
