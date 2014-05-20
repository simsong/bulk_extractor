Writing New Pattern Scanners
=============================

## Overview

This guide is for C++ developers who wish to develop new bulk_extractor scanners that use one or more regular expression patterns to search for potentially relevant data and then invoke custom procedures to handle the identified data.

bulk_extractor links with the [Lightgrep search library](https://github.com/LightboxTech/liblightgrep) to provide for multi-pattern, multi-encoding searches, and it powers the [Accts](../src/scan_accts_lg.cpp), [Base16](../src/scan_base16_lg.cpp), [Email](../src/scan_email_lg.cpp), and [GPS](../src/scan_gps_lg.cpp) scanners in addition to searching for user-specified patterns. Lightgrep aggregates all of the search patterns and conducts the search over a single pass of the data, and then dispatches to registered handlers when there are matches on any of the patterns.

There are three classes that work together to provide this functionality: LightgrepController, PatternScanner, and Handler. All three are declared in [pattern_scanner.h](../src/pattern_scanner.h).

### LightgrepController

LightgrepController is a [Singleton](http://en.wikipedia.org/wiki/Singleton_pattern) class that interacts with liblightgrep. It collects patterns from different scanners, searches the blocks, and dispatches back to the appropriate scanner when there is a pattern match.

LightgrepController is given data by the [Lightgrep scanner](../src/scan_lightgrep.cpp), which searches for user-specified patterns, too. Unless you're making changes to scan_lightgrep, you will not need to interact with LightgrepController to create your scanner.

### PatternScanner

To create your own pattern-based scanner, you need to create a class which inherits from class PatternScanner. PatternScanner has a number of virtual functions that need to be overridden. The basic lifetime model is to create a static prototype instance. Bulk_extractor interacts with the prototype, for getting basic information and to register the patterns with the LightgrepController instance.

When it is time to scan a block, the PatternScanner prototype is cloned and `initScan()` is called on the clone. If any pattern hits are encountered in the block, Lightgrep will invoke the given callback handler on the PatternScanner. The handler is given the full sbuf along with the location of the pattern, to allow it to perform any contextual analysis. The handler is also responsible for optionally recording the hit to any given feature recorders. When the block has been completely searched, the PatternScanner is notified via `finishScan()`, allowing it to perform cleanup and write any additional information to associated feature recorders.

By operating on the clone of the PatternScanner prototype, PatternScanner objects are thread-safe by default. Developers need only lock any immutable data that is shared between their PatternScanner instances.

### Handlers

Handlers are simple structs that associate a PatternScanner with a given pattern, a set of encodings, some options related to parsing the pattern, and a callback method pointer for when a hit on the pattern is encountered. The PatternScanner base class maintains a collection of Handler objects for the scanner, and then provides it to the LightgrepController instance. The LightgrepController instance initializes Lightgrep with the patterns and then can dispatch back to the callback method when the pattern is encountered. `PatternScanner::init()` should create the initial Handler objects.

## Example

Below is a simple example of a custom PatternScanner:

    #include "pattern_scanner.h"

    class MyScanner: public PatternScanner {
    public:
      MyScanner(): PatternScanner("MyScannerName"), Recorder(0) {} // constructor
      virtual ~MyScanner() {} // virtual destructor

      virtual MyScanner* clone() const { return new MyScanner(*this); }

      virtual void startup(const scanner_params& sp) {
        // Set the info fields
        sp.info->name             = "MyScanner";
        sp.info->author           = "Jane Doe";
        sp.info->description      = "An Example Pattern Scanner";
        sp.info->scanner_version  = "0.1";

        // Add a feature_recorder
        sp.info->feature_names.insert("MyScanner Features");
      }

      virtual void init(const scanner_params& sp) {
        // Create the Handlers in an idiomatic fashion
        // the PatternScanner base class will handle destruction
        new Handler(*this, "foo", PatternScanner::DefaultEncodings, PatternScanner::DefaultOptions, &MyScanner::fooHit);
        new Handler(*this, "bar", PatternScanner::DefaultEncodings, PatternScanner::DefaultOptions, &MyScanner::barHit);
      }

      virtual void initScan(const scanner_params&) {
        // retrieve the feature_recorder at the beginning of a scan
        Recorder = sp.fs.get_name("MyScanner Features");
      }

      void fooHit(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
        // record the hit on foo
        Recorder->write_buf(sp.sbuf, hit.Start, hit.End - hit.Start);
      }

      void barHit(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
        // record the hit on bar
        Recorder->write_buf(sp.sbuf, hit.Start, hit.End - hit.Start);
      }

    private:
      MyScanner(const MyScanner& x): PatternScanner(x), Recorder(x.Recorder) {} // copy constructor
      MyScanner& operator=(const MyScanner&); // private to avoid accidental copying

      feature_recorder* Recorder;
    };

    static MyScanner MyScannerPrototype; // the prototype instance, as a static

    // the scanner function, using the scan_lg() utility function
    extern "C"
    void scan_myscanner(const class scanner_params &sp, const recursion_control_block &rcb) {
      scan_lg(MyScannerPrototype, sp, rcb);
    }

The `startup()` method is used simply to set the various info fields on the scanner\_params object, for potential display to the user, and to initialize any feature\_recorders and/or feature\_histograms. The `init()` is called to enable initialization of the Handlers, which are then retrieved automatically by LightgrepController via the PatternScanner base class. Here you can see that we create two handlers for the patterns "foo" and "bar", each with their own callback method. In this case, the implementation of the callback methods is identical and therefore a single callback method could be used by both Handlers. Finally, the `initScan()` method is called at the beginning of a block scan and is used here to retrieve feature_recorder for use by the handler callbacks.

Normal Bulk\_Extractor scanners are simple functions. To bridge the divide between the PatternScanner class and Bulk\_extractor's function interface, the `scan_lg()` utility function is provided. You create a stub function for Bulk\_Extractor and then have it call `scan_lg()` with the prototype instance of your customer PatternScanner class. It will handle the various scan phases automatically.

## Registration

Once you've created your custom PatternScanner as above, you must register the scanner function in the central bulk_extractor scanner list. Add a declaration of the scanner function to [bulk_extractor.h](../src/bulk_extractor.h):

    ...
    #ifdef HAVE_LIBLIGHTGREP
    extern "C" scanner_t scan_accts_lg;
    extern "C" scanner_t scan_base16_lg;
    extern "C" scanner_t scan_email_lg;
    extern "C" scanner_t scan_gps_lg;

    extern "C" scanner_t scan_myscanner; // declare scan_myscanner here

    extern "C" scanner_t scan_lightgrep;
    #endif
    ...

And then add a pointer to your scan function to the `scanners_builtin` array in [bulk_extractor.cpp](../src/bulk_extractor.cpp):

    ...
    scanner_t *scanners_builtin[] = {
    #ifdef HAVE_LIBLIGHTGREP
        scan_accts_lg,
        scan_base16_lg,
        scan_email_lg,
        scan_gps_lg,

        scan_myscanner, // adds function pointer to the array

        scan_lightgrep,
    #endif

Once you've done this, your scanner will be part of Bulk\_Extractor!
