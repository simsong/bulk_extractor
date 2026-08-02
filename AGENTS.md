# AI contributor instructions

GitHub activity authored by Codex must use `@${USER}-codex` when that account exists. For this workspace, use `@simsong-codex`; do not use the personal `@simsong` identity for GitHub writes, pushes, issues, pull requests, reviews, or comments.

Before creating or amending a Codex-authored commit, configure and verify this
repository's author and committer as `Codex AI Assistant <simsong+codex@acm.org>`
and verify the configured signing key belongs to that identity. SSH remote
authentication (including `github-codex`) controls push access only; it does
not set commit metadata. Use `git log --format='%G? %GS %an <%ae> %cn <%ce>'`
to verify the resulting commit before pushing. When correcting existing
commits, use `git commit --amend --reset-author -S` rather than only amending
the signature.

For every Codex-authored pull request, request a Copilot review after publishing it, then monitor the pull request until Copilot has responded and all required CI checks have completed. Address actionable feedback before declaring the pull request ready. When pushing a fix in response to Copilot, reply to its thread with the fix and validation evidence, but leave the thread unresolved for `@simsong` to review and resolve. Once it is green and feedback-free, mark it ready for review and assign it to `@simsong`; do not approve or merge it unless explicitly asked.

Delete a local branch once it has merged into `main`. First verify that it is an ancestor of the current `main`; preserve unmerged branches and local files in linked worktrees.

Do not close GitHub issues. You may validate an issue, record evidence, and recommend
closure in a comment, but change an issue's open/closed state only when the repository
owner explicitly instructs you to do so.

Before changing a scanner or writing a scanner plug-in, read
[doc/scanner_api.md](doc/scanner_api.md). Start new loadable scanners from
[doc/scanner_template.cpp](doc/scanner_template.cpp), and preserve its phase
and concurrency rules unless the API documentation is updated in the same
change.
