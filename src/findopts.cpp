#include "findopts.h"

FindOpts::FindOpts(): Files(), Patterns() {}

FindOpts::FindOpts(const FindOpts& x):
  Files(x.Files), Patterns(x.Patterns) {}

bool FindOpts::empty() const {
  return Files.empty() && Patterns.empty();
}

FindOpts& FindOpts::get() {
  static FindOpts singleton;
  return singleton;
}
