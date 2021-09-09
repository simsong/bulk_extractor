/*
 *
 */

#ifndef _BULK_EXTRACTOR_H_
#define _BULK_EXTRACTOR_H_

#include "be13_api/scanner_set.h"

void debug_help();
void usage(const char *progname, scanner_set &ss);
void validate_path(const std::filesystem::path fn);
int bulk_extractor_main(int argc,char **argv);

#endif
