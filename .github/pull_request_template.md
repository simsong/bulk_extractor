## Summary

Describe the change and why it is needed. Link the related issue or Discussion.

## Forensic impact

State the expected effect on extraction, feature output, offsets, recursion,
false positives/negatives, compatibility, or performance. Write `None` when it
does not affect those areas.

## Validation

List the commands run through the Makefile and their results. Include focused
fixtures, platform/compiler coverage, or benchmark comparisons when relevant.

```
make check
```

## Checklist

- [ ] I kept this pull request focused and updated documentation where needed.
- [ ] I added or updated a meaningful test for changed behavior, or explained why one is not appropriate.
- [ ] I used only redistributable, non-sensitive test data.
- [ ] I read `doc/scanner_api.md` and used `doc/scanner_template.cpp` if this changes or adds a scanner.
- [ ] I confirm I may contribute this work under the repository license.
