/**
 * class word_and_context_list reads from disk and maintains in memory
 * a data structure that is used for the stop list and alert list.
 */

#include "config.h"
#include <cinttypes>
#include <iostream>

#include "word_and_context_list.h"

void word_and_context_list::add_regex(const std::string& pat) { patterns.push_back(pat); }

/**
 * Insert a feature and context, but only if not already present.
 * Returns true if added.
 */
bool word_and_context_list::add_fc(const std::string& f, const std::string& c) {
    context ctx(f, c); // ctx includes feature, before and after

    if (c.size() > 0 && context_set.find(c) != context_set.end()) return false; // already present
    context_set.insert(c);                                                      // now we've seen it.
    fcmap.insert(std::pair<std::string, context>(f, ctx));
    return true;
}

/**
returns 0 if success, -1 if fail. */
int word_and_context_list::readfile(const std::filesystem::path path, std::ostream &os) {
    std::ifstream i( path );
    if (!i.is_open()) return -1;
    os << "Reading context stop list " << path << "\n";
    std::string line;
    uint64_t total_context = 0;
    uint64_t line_counter = 0;
    uint64_t features_read = 0;
    while (getline(i, line)) {
        line_counter++;
        if (line.size() == 0) continue;
        if (line[0] == '#') continue; // it's a comment
        if ((*line.end()) == '\r') { line.erase(line.end()); /* remove the last character if it is a \r */ }
        if (line.size() == 0) continue; // no line content
        ++features_read;

        // If there are two tabs, this is a line from a feature file
        size_t tab1 = line.find('\t');
        if (tab1 != std::string::npos) {
            size_t tab2 = line.find('\t', tab1 + 1);
            if (tab2 != std::string::npos) {
                size_t tab3 = line.find('\t', tab2 + 1);
                if (tab3 == std::string::npos) tab3 = line.size();
                std::string f = line.substr(tab1 + 1, (tab2 - 1) - tab1);
                std::string c = line.substr(tab2 + 1, (tab3 - 1) - tab2);
                if (add_fc(f, c)) { ++total_context; }
            } else {
                std::string f = line.substr(tab1 + 1);
                add_fc(f, ""); // Insert a feature with no context
            }
            continue;
        }

        // If there is no tab, then this must be a simple item to ignore.
        // If it is a regular expression, add it to the list of REs
        if (regex_vector::has_metachars(line)) {
            patterns.push_back(line);
        } else {
            // Otherwise, add it as a feature with no context
            fcmap.insert(std::pair<std::string, context>(line, context(line)));
        }
    }
    os << "Stop list read.\n";
    os << "  Total features read: " << features_read << " in " << line_counter << " lines.\n";
    os << "  List Size: " << fcmap.size() << "\n";
    os << "  Context Strings: " << total_context << "\n";
    os << "  Regular Expressions: " << patterns.size() << "\n";
    return 0;
}

/** check() is threadsafe. */
bool word_and_context_list::check(const std::string& probe, const std::string& before, const std::string& after) const {
    /* First check literals, because they are faster */
    for (stopmap_t::const_iterator it = fcmap.find(probe); it != fcmap.end(); it++) {
        if ((rstrcmp((*it).second.before, before) == 0) && (rstrcmp((*it).second.after, after) == 0) &&
            ((*it).second.feature == probe)) {
            return true;
        }
    }

    /* Now check the patterns; do this second because it is more expensive */
    return patterns.search_all(probe, nullptr);
};

bool word_and_context_list::check_feature_context(const std::string& probe, const std::string& context) const {
    std::string before;
    std::string after;
    context::extract_before_after(probe, context, before, after);
    return check(probe, before, after);
}

void word_and_context_list::dump(std::ostream &os) {
    os << "dump context list:\n";
    for (auto const& it : fcmap) { os << it.first << " = " << it.second << "\n"; }
    os << "dump RE list:\n";
    patterns.dump(os);
}

#ifdef STAND
int main(int argc, char** argv) {
    cout << "testing contxt_list\n";
    word_and_context_list cl;
    while (--argc) {
        argv++;
        if (cl.readfile(*argv)) { err(1, "Cannot read %s", *argv); }
    }
    cl.dump();
    exit(1);
}
#endif
