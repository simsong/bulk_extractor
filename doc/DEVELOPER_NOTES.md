Notes on writing modules:

## Working with PDF files

Approaches for decompressing data streams for debugging:

* https://stackoverflow.com/questions/15058207/pdftk-will-not-decompress-data-streams
* https://superuser.com/questions/264695/how-can-i-deflate-compressed-streams-inside-a-pdf

One of these should work:
```
pdftk test.pdf output test-d.pdf uncompress

qpdf --stream-data=uncompress input.pdf output.pdf
```

