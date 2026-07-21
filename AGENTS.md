# AI contributor instructions

Before changing a scanner or writing a scanner plug-in, read
[doc/scanner_api.md](doc/scanner_api.md). Start new loadable scanners from
[doc/scanner_template.cpp](doc/scanner_template.cpp), and preserve its phase
and concurrency rules unless the API documentation is updated in the same
change.
