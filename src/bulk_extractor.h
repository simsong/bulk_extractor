/*
 *
 */

#ifndef _BULK_EXTRACTOR_H_
#define _BULK_EXTRACTOR_H_

#include "be13_api/scanner_set.h"
#include "be13_api/aftimer.h"
#include "notify_thread.h"

#include <ostream>

[[noreturn]] void debug_help();
void validate_path(const std::filesystem::path fn);
int bulk_extractor_main(std::ostream &cout, std::ostream &cerr, int argc,char * const *argv);
#endif
