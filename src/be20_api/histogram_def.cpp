#include "histogram_def.h"

histogram_def::histogram_def(const std::string& name_,
                             const std::string& feature_, // which feature file to use
                             const std::string& pattern_, // which pattern to abstract
                             const std::string& require_, // text required on the line
                             const std::string& suffix_,  // which suffix to add to the feature file name for the histogram
                             const struct flags_t& flags_):
    name(name_), feature(feature_), pattern(pattern_), reg(pattern_), require(require_), suffix(suffix_), flags(flags_) {
}



bool histogram_def::match(std::u32string u32key, std::string* displayString, const std::string &context) const {
    if (flags.lowercase) {
        u32key = utf32_lowercase(u32key);
    }

    if (flags.numeric) {
        u32key = utf32_extract_numeric(u32key);
    }

    /* TODO: When we have the ability to do regular expressions in utf32, do that here.
     * We don't have that, so do the rest in utf8
     */

    /* Convert match string to u8key */
    std::string u8key = convert_utf32_to_utf8(u32key);

    if (require.size() > 0 ){

        /* If a string is required and it is not present, return */
        if (flags.require_feature && u8key.find(require)  == std::string::npos) {
            return false;
        }

        if (flags.require_context && context.find(require) == std::string::npos) {
            return false;
        }
    }

    /* Check for pattern */
    if (pattern.size() > 0) {
        std::smatch m{};
        std::regex_search(u8key, m, this->reg);
        if (m.empty() == true) { // match does not exist
            return false;        // regex not found
        }
        u8key = m.str();
    }

    if (displayString) { *displayString = u8key; }
    return true;
}

bool histogram_def::match(std::string u32key, std::string* displayString, const std::string &context) const {
    return match(convert_utf8_to_utf32(u32key), displayString, context);
}

std::ostream& operator<<(std::ostream& os, const histogram_def::flags_t& f) {
    os << "<histogram_def::flags(";
    if (f.lowercase) os << " lowercase";
    if (f.numeric) os << " numeric";
    if (f.require_feature) os << " require_feature";
    if (f.require_context) os << " require_context";
    os << "> ";
    return os;
}


std::ostream& operator<<(std::ostream& os, const histogram_def& hd) {
    os << "<histogram_def( name:" << hd.name << " feature:" << hd.feature << " pattern:" << hd.pattern
       << " require:" << hd.require << " suffix:" << hd.suffix << ")>";
    return os;
}
