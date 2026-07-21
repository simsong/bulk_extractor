/*
 * Minimal loadable bulk_extractor scanner.
 *
 * Copy this file, rename scan_empty and "empty", then add feature recorders
 * and scan logic only in the phases documented in scanner_api.md.
 */
#include "config.h"
#include "scanner_params.h"

namespace {
void scan_empty(scanner_params &sp)
{
    switch (sp.phase) {
    case scanner_params::PHASE_INIT:
        sp.check_version();
        sp.info->set_name("empty");
        sp.info->author = "Your Name";
        sp.info->description = "Minimal loadable scanner template.";
        sp.info->scanner_version = "1.0";
        return;

    case scanner_params::PHASE_INIT2:
    case scanner_params::PHASE_SCAN:
    case scanner_params::PHASE_SHUTDOWN:
    case scanner_params::PHASE_CLEANUP:
        return;

    default:
        return;
    }
}
}

extern "C" scanner_t *bulk_extractor_scanner_v1()
{
    return scan_empty;
}
