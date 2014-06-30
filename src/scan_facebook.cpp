#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sstream>

using namespace std;

struct used_offsets_t {
    used_offsets_t():offsets(){};
    virtual ~used_offsets_t(){};
    vector<ssize_t> offsets;
    static const ssize_t window = 4096;

    bool value_used(ssize_t value) {
        for (unsigned int i = 0; i < offsets.size(); i++) {
            if (offsets[i] - window / 2 < value &&
                offsets[i] + window / 2 > value) {
                return true;
            }
        }
        offsets.push_back(value);
        return false;
    }
};

static const char *facebook_searches[] = {"hovercard/page",
                                          "profile_owner",
                                          "actorDescription actorNames",
                                          "navAccountName",
                                          "renderedAuthorList",
                                          "pokesText",
                                          "id=\"facebook.com\"",
                                          "OrderedFriendsListInitialData",
                                          "mobileFriends",
                                          "ShortProfiles",
                                          "bigPipe.onPageletArrive",
                                          "TimelineContentLoader",
                                          "Facebook is a social utility that connects",
                                          "facebook.com/profile.php",
                                          "timelineUnitContainer",
                                          0};

extern "C"
void scan_facebook(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP)        {
            assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
            sp.info->name = "facebook";
            sp.info->author = "";
            sp.info->description = "Searches for facebook html and json tags";
            sp.info->scanner_version = "2.0";
            sp.info->feature_names.insert("facebook");
            sp.info->flags = scanner_info::SCANNER_DISABLED;
            return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN) {
        feature_recorder *facebook_recorder = sp.fs.get_name("facebook");
        used_offsets_t used_offsets;
        
        for (int j = 0; facebook_searches[j]; j++) {
            const char *text_search = facebook_searches[j];
            for (size_t i = 0;  i+50 < sp.sbuf.bufsize; i++) {
                ssize_t location = sp.sbuf.find(text_search, i);
                if (location < 1) break;
                if (used_offsets.value_used(location)) {
                    i = location + used_offsets_t::window;
                    continue;
                }
                
                ssize_t begin = (location > used_offsets_t::window / 2) ? (location-used_offsets_t::window/2) : 0;
                size_t end = begin + used_offsets_t::window;
                if (end + 10 > sp.sbuf.bufsize) end = sp.sbuf.bufsize - 10;
                ssize_t length = end - begin;
                facebook_recorder->write_buf(sp.sbuf, begin, length);
                i = location + used_offsets_t::window;
            }
        }
    }
}
