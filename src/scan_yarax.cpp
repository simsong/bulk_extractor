/**
 * Plugin: scan_yarax
 * Purpose: Run yara-x rules against raw pages
 * Reference: https://virustotal.github.io/yara-x/
 **/

#include <iostream>

#include "config.h"
#include "be20_api/scanner_params.h"

#ifdef HAVE_YARAX
extern "C" void scan_yarax(scanner_params &sp);

void scan_yarax(scanner_params &sp) {
  sp.check_version();
  if (sp.phase == scanner_params::PHASE_INIT){
    sp.info->set_name("yarax");
    sp.info->author          = "Jon Stewart";
    sp.info->description     = "Scans for yara-x rule matches in raw pages";
    sp.info->scanner_version = "1.0";
    return;
  }
}
#endif

