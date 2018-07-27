#ifndef COMMANDS_H_GUARD_A64A198C_8C76_40FC_8709_A8166E1361BC
#define COMMANDS_H_GUARD_A64A198C_8C76_40FC_8709_A8166E1361BC

#include "../fat.h"

void process_info(FATINFO* fi, FATFILE* curdir, int argc, char* argv[]);
void process_ls(FATINFO* fi, FATFILE* curdir, int argc, char* argv[]);

#endif
