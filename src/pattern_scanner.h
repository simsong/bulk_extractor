#ifndef PATTERN_SCANNER_H
#define PATTERN_SCANNER_H

#include <vector>
#include <string>
#include <tr1/functional>

#include <lightgrep/api.h>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;

typedef function< void(const LG_SearchHit&, const scanner_params& sp, const recursion_control_block& rcb) > CallbackFnType;

/*********************************************************/

class PatternScanner;

struct Handler {
  Handler(PatternScanner& scanner, const string& re, const vector<string>& encs, const CallbackFnType& fn);

  string RE;

  vector<string> Encodings;

  CallbackFnType Callback;
};

/*********************************************************/

class PatternScanner {
public:
  PatternScanner(const string& name): Name(name), Handlers() {}
  virtual ~PatternScanner() {}

  virtual const vector<const Handler*>& handlers() const { return Handlers; }

  virtual void init(const scanner_params& sp) = 0;
  virtual void cleanup(const scanner_params& sp) = 0;

  // return bool indicates whether scanner addition should be continued
  // default is to print message to stderr and quit parsing scanner patterns
  virtual bool handleParseError(const Handler& h, LG_Error* err) const;

  virtual void addHandler(const Handler* h) {
    Handlers.push_back(h);
  }

protected:
  string                    Name;
  vector< const Handler* >  Handlers;
};

/*********************************************************/

class LightgrepController {
public:

  static LightgrepController& Get();

  void addScanner(const PatternScanner& scanner);

  void regcomp();
  void scan(const scanner_params& sp, const recursion_control_block& rcb);
  void processHit(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

private:
  LightgrepController();
  LightgrepController(const LightgrepController&);
  ~LightgrepController();

  LightgrepController& operator=(const LightgrepController&);

  LG_HPATTERN     ParsedPattern;
  LG_HFSM         Fsm;
  LG_HPATTERNMAP  PatternInfo;
  LG_HPROGRAM     Prog;
};

#endif
