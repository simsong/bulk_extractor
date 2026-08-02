# AI contributor instructions

GitHub activity authored by Codex must use `@${USER}-codex` when that account exists. For this workspace, use `@simsong-codex`; do not use the personal `@simsong` identity for GitHub writes, pushes, issues, pull requests, reviews, or comments.

For every Codex-authored pull request, request a Copilot review after publishing it by requesting `copilot-pull-request-reviewer` (not `copilot`). Verify the request through GitHub's review state, then monitor until a Copilot review or review thread actually appears and all required CI checks have completed. Do not report a request as made, or Copilot as having responded, based only on a CLI command or an `@copilot` comment. Address actionable feedback before declaring the pull request ready. When pushing a fix in response to Copilot, reply to its thread with the fix and validation evidence, but leave the thread unresolved for `@simsong` to review and resolve. Once it is green and feedback-free, mark it ready for review and assign it to `@simsong`; do not approve or merge it unless explicitly asked.

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
