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

For every Codex-authored pull request, follow the **Copilot-to-Ready
lifecycle** in order. The user may invoke this procedure by name:

1. Push the branch and open the pull request as a draft.
2. Request the Copilot bot with the exact GitHub API reviewer value
   `copilot-pull-request-reviewer[bot]`; do not use the incomplete value
   `copilot-pull-request-reviewer`. GitHub's UI "Request" control for Copilot
   is an equivalent fallback when the API response is ambiguous. Verify a
   Copilot pending-review entry or a `REVIEW_REQUESTED_EVENT`, not merely an
   HTTP success response.
3. Keep the pull request a draft while monitoring until a Copilot review or
   review thread actually appears. Do not report a request or response based
   only on a CLI command or an `@copilot` comment.
4. Address every actionable Copilot finding. For each pushed fix, reply on the
   exact Copilot thread with the commit and validation evidence, then request
   and monitor Copilot's re-review of that new commit. Do not manually resolve
   the thread; if GitHub resolves it automatically, report that fact.
5. After Copilot is feedback-free and all required CI checks are green, mark
   the pull request ready for review and assign it to `@simsong`.

The lifecycle is not complete at a pushed fix, a thread reply, or a successful
Copilot request. It completes only after the current head's Copilot review and
required CI are clear, the pull request has been marked ready for review, and
`@simsong` has been assigned. If validation or feedback remains, retain draft
status and report the exact blocker.

### Periodic Copilot-to-Ready timer

When the Copilot-to-Ready timer is active, wake every 20 minutes and reconcile
live GitHub state. For the current release milestone, scan its open issues
assigned to `@simsong-codex`, then resume the highest-priority unblocked work
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
