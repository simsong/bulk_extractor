#ifndef PATTERN_SCANNER_H
#define PATTERN_SCANNER_H

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <vector>
#include <string>
#include <utility>

#include <lightgrep/api.h>

#include "be20_api/scanner_params.h"

using namespace std;

class PatternScanner;

// // Inherit from this to create your own Lightgrep-based scanners
// // clone(), startup(), init(), and initScan() must be overridden
class PatternScanner {
public:
  PatternScanner(const string& n): Name(n) {}
  virtual ~PatternScanner() {}

  virtual PatternScanner* clone() const = 0;

  const string& name() const { return Name; }

  virtual void startup(const scanner_params& sp) = 0;

  virtual void init(const scanner_params& sp) = 0; // register handlers

  virtual void finishScan(const scanner_params& sp) {} // done searching a region

  virtual void shutdown(const scanner_params& sp); // perform any shutdown, if necessary

protected:
  PatternScanner(const PatternScanner& s):
    Name(s.Name) {}

  string                 Name;
};


class LightgrepController { // Centralized search facility amongst PatternScanners
public:

  LightgrepController();
  LightgrepController(const LightgrepController&);
  ~LightgrepController();

  bool addUserPatterns(PatternScanner& scanner, const vector<string>& cli_patterns, const vector<filesystem::path>& user_files);

  void regcomp();
  void scan(const scanner_params& sp);

  unsigned int numPatterns() const;

private:
   LightgrepController& operator=(const LightgrepController&);

  LG_HPATTERN     ParsedPattern;
  LG_HFSM         Fsm;
  LG_HPROGRAM     Prog;

  vector<PatternScanner*> Scanners;
};

#endif
#endif /* PATTERN_SCANNER_H */
