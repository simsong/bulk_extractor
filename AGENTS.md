# AI contributor instructions

GitHub activity authored by Codex must use `@${USER}-codex` when that account exists. For this workspace, use `@simsong-codex`; do not use the personal `@simsong` identity for GitHub writes, pushes, issues, pull requests, reviews, or comments.

For every Codex-authored pull request, request a Copilot review after publishing it, then monitor the pull request until Copilot has responded and all required CI checks have completed. Address actionable feedback before declaring the pull request ready. Once it is green and feedback-free, mark it ready for review and assign it to `@simsong`; do not approve or merge it unless explicitly asked.

Do not close GitHub issues. You may validate an issue, record evidence, and recommend closure in a comment, but change an issue's open/closed state only when the repository owner explicitly instructs you to do so.

Before changing a scanner or writing a scanner plug-in, read
[doc/scanner_api.md](doc/scanner_api.md). Start new loadable scanners from
[doc/scanner_template.cpp](doc/scanner_template.cpp), and preserve its phase
and concurrency rules unless the API documentation is updated in the same
change.

Update [doc/RELEASE_NOTES.md](doc/RELEASE_NOTES.md) in the same pull request
for every user-visible behavior, build or platform-support, packaging, or
documentation change. Do not add release-note entries for test-only or purely
internal refactors.
