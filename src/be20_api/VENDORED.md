# Integrated source provenance

`be20_api` is an internal bulk_extractor component. It was imported as ordinary tracked source on 2026-07-19 because it had no remaining external consumers and the Git submodule boundary did not provide a build or API boundary.

Imported revision:

- be20_api: `625f5b3ddd9d6baf4aaa7c10a6cefb5e2bfb1a8b` from `https://github.com/simsong/be20_api.git`

The upstream license and attribution files remain beside the imported be20_api
sources. dfxml_cpp and utfcpp were not vendored; the parent `.gitmodules` and
Git links retain their independent repositories and exact revisions. Future
be20_api changes are made and tested in bulk_extractor, and this directory is
no longer independently bootstrapped or released.
