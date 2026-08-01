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
| `2` | Write every object passed to that recorder's carver, subject to validation, deduplication, and its minimum and maximum carve sizes. |

Mode `1` is the default for JPEG and RAR recorders (`rar` and
`unrar_carved`). It focuses on objects that ordinary file carvers are less
likely to recover while avoiding a second copy of bare objects from the
original image. `zip_carved` and `sqlite_carved` default to mode `2`.

The carving feature file records files actually carved (or a cached duplicate),
not every object that a different mode could have carved. Some scanners also
emit separate metadata feature records regardless of carve mode; for example,
`zip.txt` describes recognized ZIP components. Use mode `2` when an inventory
of every validated object for a carving recorder is required.

There is currently no one-option global carve-mode override. Supply one `-S`
setting for each carving recorder whose default you want to change.
