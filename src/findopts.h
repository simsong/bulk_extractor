#ifndef FINDOPTS_H
#define FINDOPTS_H

#include <vector>
#include <string>

// Lists of patterns and pattern files
class FindOpts {
public:
  std::vector<std::string> Files;     // accumulates pattern files
  std::vector<std::string> Patterns;  // accumulates cmdline patterns

  bool empty() const;

  static FindOpts& get();

private:
  FindOpts();
  FindOpts(const FindOpts& x);
};

#endif