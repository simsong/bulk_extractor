#ifndef FINDOPTS_H
#define FINDOPTS_H

#include <vector>
#include <string>

/*
 * Small class for accumulating command line options.
 *
 * Lists of patterns and pattern files
 */
class FindOpts {
public:
    std::vector<std::string> Files {};     // accumulates pattern files
    std::vector<std::string> Patterns {};  // accumulates cmdline patterns

    bool empty() const {
        return Files.empty() && Patterns.empty();
    }

    static FindOpts& get() {
        static FindOpts singleton;
        return singleton;
    }

private:
    FindOpts(){}
    FindOpts(const FindOpts& x) = delete;
};

#endif
