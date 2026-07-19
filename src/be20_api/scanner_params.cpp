#include "config.h"
#include "scanner_params.h"
#include "feature_recorder.h"
#include "feature_recorder_set.h"
#include "scanner_set.h"
#include "path_printer.h"

scanner_params::scanner_params(struct scanner_config &sc_, class scanner_set  *ss_,
                               const path_printer *pp_, phase_t phase_, const sbuf_t* sbuf_)
    : sc(sc_), ss(ss_), pp(pp_), phase(phase_), sbuf(sbuf_)
{
}

scanner_params::scanner_params(const scanner_params& sp_existing, const sbuf_t* sbuf_, std::string pp_path_)
    : sc(sp_existing.sc), ss(sp_existing.ss), pp(sp_existing.pp), phase(sp_existing.phase), sbuf(sbuf_),
      pp_path(pp_path_), pp_po(sp_existing.pp_po)
{
}


/* This interface creates if we are in init phase, doesn't if we are in scan phase */
feature_recorder& scanner_params::named_feature_recorder(const std::string feature_recorder_name) const
{
    assert(ss!=nullptr);
    return ss->named_feature_recorder(feature_recorder_name);
}

/*
 * Allow call by scanners using the sp. Currently used in scan_zip
 */
bool scanner_params::check_previously_processed(const sbuf_t &s) const
{
    assert(ss!=nullptr);
    return ss->previously_processed_count(s)==0;
}

void scanner_params::recurse(const sbuf_t* new_sbuf) const {
    if (pp!=nullptr) {                  // we have a path printer; call that instead
        scanner_params sp_new(*this, new_sbuf, this->pp_path);
        try {
            pp->process_sp( sp_new );           // where do we keep the path being processed? In scanner_params...
        }
        catch (path_printer::path_printer_finished &e) {
            delete new_sbuf;                    // make sure it gets deleted
            throw;                              // and re-throw
        }
        delete new_sbuf;                    // and now we are done with it.
        return;
    }

    assert(ss!=nullptr);                // make sure there is a scanner set if we are descending
    // In normal operations we recurse. However, in unit testing recursion is sometimes intentionally disabled.
    // In such a situation, the sbuf is just deleted.
    if (ss->allow_recurse()) {
        ss->schedule_sbuf(new_sbuf);    /* sbuf will be deleted after it is processed */
    } else {
        delete new_sbuf;                // just delete it
    }
}
