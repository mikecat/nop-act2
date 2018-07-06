#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "number_io.h"

int main(int argc, char* argv[]) {
	char** command = NULL;
	int command_num = -1;

	int cmd_error = 0, print_help = 0;
	char* disk_name = NULL;
	int partition = 0;
	int partition_list = 0;

	DISK* disk;
	size_t sector_start, sector_num;

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

	disk = open_disk(disk_name);
	if (disk == NULL) {
		fprintf(stderr, "failed to open disk %s\n", disk_name);
		return 1;
	}

	if (partition_list) {
		char data[512];
		int i, found = 0;
		size_t all_sector_num = get_disk_sector_num(disk);
		if (!disk_read(disk, 0, 512, data)) {
			fprintf(stderr, "MBR read failed\n");
			if (!close_disk(disk)) fprintf(stderr, "failed to close disk!\n");
			return 1;
		}
		for (i = 0; i < 4; i++) {
			unsigned char type = (unsigned char)data[446 + 16 * i + 4];
			if (type != 0x00) {
				unsigned int first, num;
				const char* unit;
				double divisor;
				first = read_number(data + 446 + 16 * i + 8, 4);
				num = read_number(data + 446 + 16 * i + 12, 4);
				if (!found) {
					puts("#  type  first sector  number of sector  size");
					found = 1;
				}
				printf("%u  0x%02X  0x%08X    0x%08X        ", i + 1, type, first, num);
				if (num < 2 * 1024) { unit = "KiB"; divisor = 2.0; }
				else if (num < 2 * 1024 * 1024) { unit = "MiB"; divisor = 2.0 * 1024; }
				else { unit = "GiB"; divisor = 2.0 * 1024 * 1024; }
				printf("%6.1f %s", num / divisor, unit);
				if (all_sector_num < first || all_sector_num - first < num) {
					printf(" (out of the disk)");
				}
				printf("\n");
			}
		}
		if (!found) {
			printf("no non-empty partition information found.\n");
		}
	} else {
		if (partition == 0) {
			sector_start = 0;
			sector_num = get_disk_sector_num(disk);
		} else if (1 <= partition && partition <= 4) {
			char data[512];
			if (!disk_read(disk, 0, 512, data)) {
				fprintf(stderr, "MBR read failed\n");
				if (!close_disk(disk)) fprintf(stderr, "failed to close disk!\n");
				return 1;
			}
			sector_start = read_number(data + 446 + 16 * (partition - 1) + 8, 4);
			sector_num = read_number(data + 446 + 16 * (partition - 1) + 12, 4);
		} else {
			fprintf(stderr, "invalid partition number %d\n", partition);
			if (!close_disk(disk)) fprintf(stderr, "failed to close disk!\n");
			return 1;
		}
		/* todo: initialize FAT info, ... */
		printf("sector_start = %u\nsector_num   = %u\n",
			(unsigned int)sector_start, (unsigned int)sector_num);
	}

	if (!close_disk(disk)) {
		fprintf(stderr, "failed to close disk!\n");
		return 1;
	}
	return 0;
}
