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

vector<ssize_t> used_offsets;
const ssize_t window = 4096;

bool value_used(ssize_t value)
{
    for (unsigned int i = 0; i < used_offsets.size(); i++)        {
            if (used_offsets[i] - window / 2 < value &&
                used_offsets[i] + window / 2 > value)                {
                    return true;
                }
    }
    used_offsets.push_back(value);
    return false;
}

extern "C"
void scan_facebook(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP)        {
            assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
            sp.info->name = "facebook";
            sp.info->author = "Bryant Nichols";
            sp.info->description = "Searches for facebook html and json tags";
            sp.info->scanner_version = "2.0";
            sp.info->feature_names.insert("facebook");
            sp.info->flags = scanner_info::SCANNER_RECURSE | scanner_info::SCANNER_RECURSE_EXPAND;
            return;
        }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if(sp.phase==scanner_params::PHASE_SCAN) {
            feature_recorder *facebook_recorder = sp.fs.get_name("facebook");
            used_offsets.clear();
                
            for (int j = 0; j < 15; j++)
                {
                    const char *text_search;
                    switch (j)
                        {
                        case 1: //css tag before a wall post comment
                            text_search = "hovercard/page";
                            break;
                        case 2: //html for likes something
                            text_search = "profile_owner";
                            break;
                        case 3:
                            text_search = "actorDescription actorNames";
                            break;
                        case 4: //css tag for the facebook user name
                            text_search = "navAccountName";
                            break;
                        case 5: // this should find fb email messages
                            text_search = "renderedAuthorList";
                            break;
                        case 6: //pokes css div class
                            text_search = "pokesText";
                            break;
                        case 7: //in the header of a facbook page
                            text_search = "id=\"facebook.com\"";
                            break;
                        case 8: //friends list
                            text_search = "OrderedFriendsListInitialData";
                            break;
                        case 9: //mobile friends list
                            text_search = "mobileFriends";
                            break;
                        case 10: //in json, profile characteristics
                            text_search = "ShortProfiles";
                            break;
                        case 11: //facebook search terms
                            text_search = "bigPipe.onPageletArrive";
                            break;
                        case 12: //facebook timeline page
                            text_search = "TimelineContentLoader";
                            break;
                        case 13: //facebook meta data header
                            text_search = "Facebook is a social utility that connects";
                            break;
                        case 14: //facebook profile in html
                            text_search = "facebook.com/profile.php";
                            break;
                        default:
                            text_search = "timelineUnitContainer";
                            break;
                        }
                    for (size_t i = 0;  i < sp.sbuf.bufsize - 50; i++)                        {
                            ssize_t location = sp.sbuf.find(text_search, i);
                            if (location < 1)
                                break;
                            if (value_used(location))
                                {
                                    i = location + window;
                                    continue;
                                }
                                
                            ssize_t begin = location - window / 2;
                            if (begin < 0)
                                begin = 0;
                            ssize_t end = begin + window;
                            if (end > sp.sbuf.bufsize - 10L)
                                end = sp.sbuf.bufsize - 10L;
                            ssize_t length = end - begin;
                            facebook_recorder->write_buf(sp.sbuf, begin, length);
                            i = location + window;
                        }
                }
        }
}
