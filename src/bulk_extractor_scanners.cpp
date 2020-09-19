/*
 * bulk_extractor.cpp:
 * Feature Extraction Framework...
 *
 */

#include <dirent.h>
#include <fcntl.h>
#include <setjmp.h>
#include <unistd.h>

#include "config.h"

#include "bulk_extractor_i.h"
#include "bulk_extractor.h"
#include "image_process.h"
#include "histogram.h"
#include "findopts.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"

#include "phase1.h"

/************************
 *** SCANNER PLUG-INS ***
 ************************/

/* scanner_def is the class that is used internally to track all of the plug-in scanners.
 * Some scanners are compiled-in; others can be loaded at run-time.
 * Scanners can have multiple feature recorders. Multiple scanners can record to the
 * same feature recorder.
 */

/* An array of the built-in scanners */
#define SCANNER(scanner) extern "C" scanner_set::scanner_t scan_ ## scanner;
#include "bulk_extractor_scanners.h"
#undef SCANNER

#define SCANNER(scanner) scan_ ## scanner ,
scanner_t *scanners_builtin[] = {
#include "bulk_extractor_scanners.h"
    0};
#undef SCANNER
