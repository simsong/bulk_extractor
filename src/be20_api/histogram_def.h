#ifndef HISTOGRAM_DEF_H
#define HISTOGRAM_DEF_H

#include <cstdio>
#include <iostream>
#include <regex>
#include <string>

#include "unicode_escape.h"

/**
 * histogram_def defines the histograms that will be made by a feature recorder.
 * If the mhistogram is set, the histogram is generated when features are recorded
 * and kept in memory. If mhistogram is not set, the histogram is generated when the feature recorder is closed.
 */

struct histogram_def {
    struct flags_t {
        flags_t(const flags_t& a) {
            this->lowercase = a.lowercase;
            this->numeric = a.numeric;
            this->require_feature = a.require_feature;
            this->require_context = a.require_context;
        };

        flags_t& operator=(const flags_t& a) {
            this->lowercase = a.lowercase;
            this->numeric = a.numeric;
            this->require_feature = a.require_feature;
            this->require_context = a.require_context;
            return *this;
        };

        bool operator<(const flags_t& a) const {
            if (this->lowercase < a.lowercase) return true;
            if (this->lowercase > a.lowercase) return false;
            if (this->numeric < a.numeric) return true;
            if (this->numeric > a.numeric) return false;

            if (this->require_feature < a.require_feature) return true;
            if (this->require_feature > a.require_feature) return false;
            if (this->require_context < a.require_context) return true;
            if (this->require_context > a.require_context) return false;
            return false;
        }

        bool operator==(const flags_t& a) const {
            return (this->lowercase == a.lowercase) && (this->numeric == a.numeric) && (this->require_feature==a.require_feature) && (this->require_context==a.require_context);
        }

        flags_t(){};
        flags_t(bool lowercase_, bool numeric_) : lowercase(lowercase_), numeric(numeric_) {}
        bool lowercase       {false}; // make all flags lowercase
        bool numeric         {false};   // extract digits only
        bool require_feature {true};  // require text is applied to feature
        bool require_context {false};  // require text is applied to context
    };

    /**
     * @param feature - the feature file to histogram (no .txt)
     * @param pattern - the regular expression to extract.
     * @param suffix - the suffix to add to the histogram file after feature name before .txt
     * @param flags  - any flags (see above)
     * @param require- require this string on the line (usually in context)
     */

    histogram_def(const std::string& name_,
                  const std::string& feature_, // which feature file to use
                  const std::string& pattern_, // which pattern to abstract
                  const std::string& require_, // text required on the line
                  const std::string& suffix_,  // which suffix to add to the feature file name for the histogram
                  const struct flags_t& flags_);
    std::string name{};    // name of the hsitogram
    std::string feature{}; // feature file to extract
    std::string
        pattern{}; // regular expression used to extract feature substring from feature. "" means use the entire feature
    mutable std::regex reg{}; // the compiled regular expression.
    std::string require{};    // text required somewhere on the feature line. Sort of like grep. used for IP histograms
    std::string suffix{};     // suffix to append to histogram report name

    /* flags */
    struct flags_t flags {};

    /* default copy construction and assignment */
    histogram_def(const histogram_def& a) {
        this->name = a.name;
        this->feature = a.feature;
        this->pattern = a.pattern;
        this->reg = a.reg;
        this->require = a.require;
        this->suffix = a.suffix;
        this->flags = a.flags;
    };

    /* assignment operator */
    histogram_def& operator=(const histogram_def& a) {
        this->name = a.name;
        this->feature = a.feature;
        this->pattern = a.pattern;
        this->reg = a.reg;
        this->require = a.require;
        this->suffix = a.suffix;
        this->flags = a.flags;
        return *this;
    }

    bool operator==(const histogram_def& a) const {
        return (this->name == a.name) && (this->feature == a.feature) && (this->pattern == a.pattern) &&
               (this->require == a.require) && (this->suffix == a.suffix) && (this->flags == a.flags);
    }

    bool operator!=(const histogram_def& a) const { return !(*this == a); }

    /* comparator, so we can have a functioning map and set classes.'
     * ignores reg.
     */
    bool operator<(const histogram_def& a) const {
        if (this->name < a.name) return true;
        if (this->name > a.name) return false;
        if (this->feature < a.feature) return true;
        if (this->feature > a.feature) return false;
        if (this->pattern < a.pattern) return true;
        if (this->pattern > a.pattern) return false;
        if (this->require < a.require) return true;
        if (this->require > a.require) return false;
        if (this->suffix < a.suffix) return true;
        if (this->suffix > a.suffix) return false;
        if (this->flags < a.flags) return true;
        return false;
    }

    /* Match and extract:
     * If the string matches this histogram, return true and optionally
     * set match to Extract and match: Does this string match
     */

    bool match(std::u32string u32key, std::string* displayString, const std::string &context) const;
    bool match(std::string u32key,    std::string* displayString, const std::string &context) const;
};

std::ostream& operator<<(std::ostream& os, const histogram_def::flags_t& f);
std::ostream& operator<<(std::ostream& os, const histogram_def& hd);

#endif
