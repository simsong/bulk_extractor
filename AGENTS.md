# AI contributor instructions

Agent GitHub writes use `@simsong-agent`, the shared account for Codex and Claude Code; never use the personal `@simsong` account. Codex keeps `Codex AI Assistant <simsong+codex@acm.org>` and its existing signing key. Claude uses its own commit email and must not use the Codex signing key.

Before creating or amending a Codex-authored commit, configure and verify this
repository's author and committer as `Codex AI Assistant <simsong+codex@acm.org>`
and verify the configured signing key belongs to that identity. SSH remote
authentication (including `github-codex`) controls push access only; it does
not set commit metadata. Use `git log --format='%G? %GS %an <%ae> %cn <%ce>'`
to verify the resulting commit before pushing. When correcting existing
commits, use `git commit --amend --reset-author -S` rather than only amending
the signature.

For every Codex-authored pull request, follow the shared **pr-to-ready**
workflow from the installed Codex skill.

### Periodic pr-to-ready timer

When the pr-to-ready timer is active, wake every 20 minutes and reconcile
live GitHub state. For the current release milestone, scan its open issues
assigned to `@simsong-agent`, then resume the highest-priority unblocked work
or its pending pull-request review/CI follow-through. Verify milestone,
assignment, PR head, Copilot state, and checks live; do not rely on an earlier
heartbeat. Do not change issue assignments or milestones, approve, merge, or
close anything without explicit user instruction. If there is no in-scope work
or nothing has changed, return a quiet heartbeat; otherwise report the exact
next action or blocker.

Do not approve or merge a pull request unless explicitly asked.

Delete a local branch once it has merged into `main`. First verify that it is an ancestor of the current `main`; preserve unmerged branches and local files in linked worktrees.

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

Every pull request that resolves a Coverity finding must update both
[doc/RELEASE_NOTES.md](doc/RELEASE_NOTES.md) and [ChangeLog](ChangeLog) in the
same pull request. Cite each resolved CID and describe the correction, even
when the change is an internal refactor or performance fix.
