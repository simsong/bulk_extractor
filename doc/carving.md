# Carving

Carving writes a recognized object to a file below the report directory. The
corresponding carving feature file records its forensic path, carved filename,
size, and hash.

Carving is configured per feature recorder, not globally. Run
`bulk_extractor -h` to list the available recorder names, then pass:

```
-S <recorder>_carve_mode=<mode>
```

For example, `-S jpeg_carve_mode=2` writes every JPEG accepted by the JPEG
validator. Carving recorders include `jpeg`, `zip_carved`, `rar`,
`unrar_carved`, and `sqlite_carved` when their scanners are enabled in the
build.

| Mode | Effect |
| --- | --- |
| `0` | Do not write carved files or entries in the carving feature file. |
| `1` | Write only objects reached through an encoded or derived input path, such as decompressed or Base64-decoded data. A bare object found directly in the input is not carved. |
| `2` | Write every object passed to that recorder's carver, subject to validation and its minimum and maximum carve sizes. |

Mode `1` is the default for JPEG and RAR recorders (`rar` and
`unrar_carved`). It focuses on objects that ordinary file carvers are less
likely to recover while avoiding a second copy of bare objects from the
original image. `zip_carved` and `sqlite_carved` default to mode `2`.

## Duplicate-processing modes

`--dedupe-mode=N` controls duplicate processing across recursive ZIP work and
carving. It is independent of each recorder's `_carve_mode`: the latter decides
which candidates a recorder may carve, while dedupe mode decides whether equal
content reached by more than one forensic path is processed or written again.

| Mode | Recursive ZIP work | Carved-object cache | Tradeoff |
| --- | --- | --- | --- |
| `0` | Process every path. | Disabled: every accepted copy is written. | Reproducible paths and output, but duplicate-rich nested archives can consume very large amounts of time and storage. |
| `1` | Skip repeated ZIP content. | Disabled: accepted copies are still carved. | Reduces repeated decompression while retaining carved copies, but the first worker to reach equal nested content determines its surviving attribution. |
| `2` (legacy, default) | Skip repeated content. | Enabled: later equal objects are reported as `<CACHED>`. | Minimizes repeated work and carved storage, but preserves the legacy scheduling-dependent path attribution and omits duplicate carved files. |

Mode 0 is the reproducible choice: it preserves every recursive and carving
path, and it does not depend on worker count. Its cost is real. A shallow,
wide archive can repeatedly expand equal members without exceeding the ZIP
depth limit, producing substantial output amplification. Choose mode 1 or 2
when processing untrusted or duplicate-heavy archives makes that cost
unacceptable, but do not treat either mode as a reproducibility guarantee:
deduplication changes which equal path wins a concurrent race. The modes change
redundancy and attribution, not the content hashes of the recovered evidence.

Mode 0 and mode 1 choose carving directories deterministically from the object
hash rather than from a processing counter. That makes the tree reproducible,
but it also scatters files across buckets; scripts that assumed
`zip_carved/000/*` contains the first carved files must instead traverse the
recorder's directories.

Some scanners emit separate metadata feature records regardless of carve mode;
for example, `zip.txt` describes recognized ZIP components. Every scan also
creates `duplicates.txt`, whose sorted rows document equal content reached by
multiple paths.

There is currently no one-option global carve-mode override. Supply one `-S`
setting for each carving recorder whose default you want to change. ZIP recursion
is independently limited to four nested ZIP levels by default; use
`-S max_zip_depth=N` to change that limit. This cap limits nesting depth, not
the number of members, total decompressed bytes, or archive fanout. A component
not opened because of the cap is recorded in `zip_carved.txt` with the
`max-zip-depth` disposition, so the limitation is auditable. Raise the cap only
when deeper nesting is required and the resulting resource cost is acceptable.
