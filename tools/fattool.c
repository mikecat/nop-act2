#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

int main(int argc, char* argv[]) {
	char** command = NULL;
	int command_num = -1;

	int cmd_error = 0, print_help = 0;
	char* disk_name = NULL;
	int partition = 0;
	int partition_list = 0;

	int i;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0) {
				print_help = 1;
			} else if (strcmp(argv[i], "-disk") == 0) {
				if (++i >= argc) cmd_error = 1; else disk_name = argv[i];
			} else if (strcmp(argv[i], "-partition") == 0) {
				if (++i >= argc) cmd_error = 1; else {
					if (strcmp(argv[i], "list") == 0) {
						partition_list = 1;
					} else {
						partition = atoi(argv[i]);
					}
				}
			} else {
				fprintf(stderr, "unknown command line: %s\n", argv[i]);
				cmd_error = 1;
			}
		} else {
			command = &argv[i];
			command_num = argc - i;
			break;
		}
	}
	if (disk_name == NULL && !print_help) {
		fprintf(stderr, "disk file name must be specified.\n");
		cmd_error = 1;
	}

	if (cmd_error || print_help) {
		fprintf(stderr, "Usage: %s options [command]\n", argc > 0 ? argv[0] : "fattool");
		fprintf(stderr,
			"\n"
			"options:\n"
			"  -help / --help : print this help\n"
			"  -disk <file>   : specify disk file\n"
			"  -partition\n"
			"    -partition <number> : set partition number (1-4) to use\n"
			"    -partition 0        : use whole disk as one partition (default)\n"
			"    -partition list     : print partition list\n"
			"\n"
			"commands:\n"
			"  not implemented\n"
		);
		return cmd_error ? 1 : 0;
	}

	return 0;
}
