#include "config.h"

#include "be20_api/scanner_params.h"

namespace {
void scan_test_plugin(scanner_params &sp)
{
    if (sp.phase != scanner_params::PHASE_INIT) return;
    sp.check_version();
    sp.info->set_name("test_plugin");
    sp.info->description = "Loadable-scanner integration test.";
    sp.info->scanner_version = "1";
}
}

extern "C" scanner_t *bulk_extractor_scanner_v1()
{
    return scan_test_plugin;
}
