This directory contains:

1 - plugin_test.cpp --- a stand-alone tester for bulk_extractor plugins.

2 - scan_demo.cpp --- a demonstration plugin

3 - scan_flexdemo.flex --- a demonstration plugin written in GNU Flex


This directory should not be built as part of the standard
bulk_extractor release.


bulk_extractor scanner plugins are implemented as shared libraries
that begin with the name "scan_". For example, the demo plug-in that
counts the number of blank sectors and prints a report of the
percentage of the disk that is blank is called scan_blank.so on
Linux/Mac and scan_blank.DLL on Windows.

When bulk_extractor starts up it examines the plugins directory for
all of the shared libraries whose name begins "scan_". Each one is
loaded into the address space. The plugin interface is a single C++
function with the same name as the shared library. The plugin must be
compiled with C linkage, rather than C++ linkage, to avoid name
mangling issues. For example:


   extern "C" 
   void scan_bulk(const class scanner_params &sp,
                  const class recursion_control_block &rcb);


The plugin takes two arguments, both of which are references to C++
objects. The first class provides the scanner parameters. This
includes a "phase" instance variable that denotes whether the scanner
is called to start-up, examine a block of data, or shut down. The
second is the recursion control block, which provides information for
use by recursive scanners (e.g. scanners that perform some kind of
transformation on the data and then request re-analysis).

The file bulk_extractor_i.h contains the bulk_extractor plug-in
interface. It is the only file that needs to be included for
plugins. By design this file contains the minimum necessary for a
functional plug-in. This approach minimizes possible interactions
between the bulk_extractor system and the plugin system.

For complete information, please refer to The bulk_extractor plugin
developer's manual, which you can download from:

       http://digitalcorpora.org/downloads/bulk_extractor/BEProgrammersManual.pdf

