# AI contributor instructions

GitHub activity authored by Codex must use `@${USER}-codex` when that account exists. For this workspace, use `@simsong-codex`; do not use the personal `@simsong` identity for GitHub writes, pushes, issues, pull requests, reviews, or comments.

Before changing a scanner or writing a scanner plug-in, read
[doc/scanner_api.md](doc/scanner_api.md). Start new loadable scanners from
[doc/scanner_template.cpp](doc/scanner_template.cpp), and preserve its phase
and concurrency rules unless the API documentation is updated in the same
change.
