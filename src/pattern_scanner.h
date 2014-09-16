#ifndef PATTERN_SCANNER_H
#define PATTERN_SCANNER_H

#include <vector>
#include <string>
#include <utility>

#include <lightgrep/api.h>

#include "findopts.h"

using namespace std;

class PatternScanner;

// the typedef for a handler callback
typedef void (PatternScanner::*CallbackFnType)(const LG_SearchHit&, const scanner_params& sp, const recursion_control_block& rcb);

/*********************************************************/

struct Handler;

// Inherit from this to create your own Lightgrep-based scanners
// clone(), startup(), init(), and initScan() must be overridden
class PatternScanner {
public:
  PatternScanner(const string& n): Name(n), Handlers(), PatternRange(0, 0) {}
  virtual ~PatternScanner() {}

  virtual PatternScanner* clone() const = 0;

  const string& name() const { return Name; }

  virtual void startup(const scanner_params& sp) = 0;

  virtual void init(const scanner_params& sp) = 0; // register handlers

  virtual void initScan(const scanner_params& sp) = 0; // get feature_recorders
  virtual void finishScan(const scanner_params& sp) {} // done searching a region

  virtual void shutdown(const scanner_params& sp); // perform any shutdown, if necessary

  // return bool indicates whether scanner addition should be continued
  // default is to print message to stderr and quit parsing scanner patterns
  virtual bool handleParseError(const Handler& h, LG_Error* err) const;

  virtual void addHandler(const Handler* h) {
    Handlers.push_back(h);
  }

  virtual const vector<const Handler*>& handlers() const { return Handlers; }

  pair<unsigned int, unsigned int>& patternRange() { return PatternRange; }
  const pair<unsigned int, unsigned int>& patternRange() const { return PatternRange; }

protected:
  PatternScanner(const PatternScanner& s):
    Name(s.Name), Handlers(s.Handlers), PatternRange(s.PatternRange) {}

  string                 Name;
  vector<const Handler*> Handlers;

  pair<unsigned int, unsigned int> PatternRange; // knows the label range of its associated patterns
};

/*********************************************************/

struct Handler {
  // Agglomeration of the scanner, pattern, encodings, parse options, and callback
  template <typename Fn>
  Handler(
    PatternScanner& scanner,
    const string& re,
    const vector<string>& encs,
    const LG_KeyOptions& opts,
    Fn fn
  ):
    RE(re),
    Encodings(encs),
    Options(opts),
    Callback(static_cast<CallbackFnType>(fn))
  {
    scanner.addHandler(this);
  }

  string RE;

  vector<string> Encodings;

  LG_KeyOptions Options;

  CallbackFnType Callback;
};

/*********************************************************/

class LightgrepController { // Centralized search facility amongst PatternScanners
public:

  static LightgrepController& Get(); // singleton instance

  bool addScanner(PatternScanner& scanner);
  bool addUserPatterns(PatternScanner& scanner, CallbackFnType* callbackPtr, const FindOpts& userPatterns);

  void regcomp();
  void scan(const scanner_params& sp, const recursion_control_block& rcb);
  void processHit(const vector<PatternScanner*>& sTbl, const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

  unsigned int numPatterns() const;

private:
  LightgrepController();
  LightgrepController(const LightgrepController&);
  ~LightgrepController();

  LightgrepController& operator=(const LightgrepController&);

  LG_HPATTERN     ParsedPattern;
  LG_HFSM         Fsm;
  LG_HPATTERNMAP  PatternInfo;
  LG_HPROGRAM     Prog;

  vector<PatternScanner*> Scanners;
};

/*********************************************************/

// Utility function. Makes your scan function a one-liner, given a PatternScanner instance
void scan_lg(PatternScanner& scanner, const class scanner_params &sp, const recursion_control_block &rcb);

#endif /* PATTERN_SCANNER_H */
