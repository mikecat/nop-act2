#include "../fat.h"
#include "commands.h"

void process_info(FATINFO* fi, FATFILE* curdir, int argc, char* argv[]) {
	(void)curdir;
	(void)argc;
	(void)argv;
	fat_printinfo(fi);
}
