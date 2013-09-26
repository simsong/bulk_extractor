#ifndef FINDOPTS_H
#define FINDOPTS_H

#include <vector>
#include <string>

// Lists of patterns and pattern files
struct FindOptsStruct {
  std::vector<std::string> Files;     // accumulates pattern files
  std::vector<std::string> Patterns;  // accumulates cmdline patterns
};

#endif