#!/usr/bin/env python3
"""Counts total lines of code under src/ (.cpp/.h), excluding the GPLv3
license header block every file starts with (see add_license_headers.py) -
so the total reflects code actually written, not repeated boilerplate.

Usage: python scripts/count_lines.py [file ...]
With no arguments, processes every src/*.cpp and src/*.h file.
"""

import pathlib
import sys

# Matches add_license_headers.py's HEADER_LINES exactly (17 comment lines +
# 2 trailing blank lines) - kept in sync manually since duplicating a single
# constant isn't worth importing that script as a module for.
HEADER_LINE_COUNT = 19
MARKER = "GNU General Public License"


def count_file(path: pathlib.Path) -> tuple[int, int]:
    """Returns (total_lines, code_lines) for one file."""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    total = len(lines)
    has_header = MARKER in "\n".join(lines[:HEADER_LINE_COUNT])
    code = total - HEADER_LINE_COUNT if has_header else total
    return total, code


def main() -> None:
    if len(sys.argv) > 1:
        files = [pathlib.Path(p) for p in sys.argv[1:]]
    else:
        repo_root = pathlib.Path(__file__).resolve().parent.parent
        src = repo_root / "src"
        files = sorted(list(src.glob("*.cpp")) + list(src.glob("*.h")))

    total_lines = 0
    total_code = 0
    for f in files:
        lines, code = count_file(f)
        total_lines += lines
        total_code += code
        print(f"{code:6d}  {f}")

    print("-" * 60)
    print(f"{len(files)} file(s)")
    print(f"total lines (incl. headers): {total_lines}")
    print(f"license header lines excluded: {total_lines - total_code}")
    print(f"code lines: {total_code}")


if __name__ == "__main__":
    main()
