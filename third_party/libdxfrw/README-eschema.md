# libdxfrw (vendored)

Unmodified copy of `src/` from the upstream LibreCAD project:

- Upstream: https://github.com/LibreCAD/libdxfrw
- Vendored commit: `92d7466ed9146badcd4fb44c82d1dd8302b3c7db` (branch `master`,
  2025-09-25)
- License: GPL-2.0-or-later (see `COPYING`) — compatible with eSchema's
  GPLv3; every source file keeps its original upstream copyright/license
  header, unmodified.

eSchema only uses the DXF (ASCII/binary) read/write path (`dxfRW`,
`DRW_Interface`) via `src/IO/DxfReader.cpp`/`DxfWriter.cpp`. The DWG
reader/writer code (`libdwgr.*`, `intern/dwgreader*.*`, `intern/dwgbuffer.*`,
`intern/rscodec.*`) is vendored too — deliberately not trimmed out, since
`intern/dwgutil.cpp` transitively depends on it and the full tree compiles
cleanly with no extra external dependencies — but eSchema never calls into
it (no DWG import/export feature).

To update: replace the contents of `src/` with a newer upstream checkout
(same file list) and update the commit hash above. Do not run
`scripts/add_license_headers.py` over this directory — it only touches
`src/*.cpp`/`src/*.h` at the repo root by default, so this vendored tree is
already outside its scope; keep it that way.
