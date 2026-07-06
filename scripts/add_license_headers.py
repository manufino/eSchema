#!/usr/bin/env python3
"""Prepends the project's GPLv3 license header to .cpp/.h files under src/
that don't already have one. Safe to re-run: files already containing the
license text are left untouched.

Usage: python scripts/add_license_headers.py [file ...]
With no arguments, processes every src/*.cpp and src/*.h file.
"""

import pathlib
import sys

AUTHOR = "Manuel Finessi"
YEARS = "2023-2026"

HEADER_LINES = [
    "/*",
    " * {filename}",
    " *",
    " * Copyright (C) {years} {author}",
    " *",
    " * This program is free software: you can redistribute it and/or modify",
    " * it under the terms of the GNU General Public License as published by",
    " * the Free Software Foundation, either version 3 of the License, or",
    " * (at your option) any later version.",
    " *",
    " * This program is distributed in the hope that it will be useful,",
    " * but WITHOUT ANY WARRANTY; without even the implied warranty of",
    " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the",
    " * GNU General Public License for more details.",
    " *",
    " * You should have received a copy of the GNU General Public License",
    " * along with this program. If not, see <https://www.gnu.org/licenses/>.",
    " */",
    "",
    "",
]

MARKER = "GNU General Public License"


def make_header(filename: str, eol: str) -> bytes:
    text = eol.join(line.format(filename=filename, years=YEARS, author=AUTHOR)
                     for line in HEADER_LINES)
    return text.encode("utf-8")


def add_header(path: pathlib.Path) -> bool:
    raw = path.read_bytes()
    if MARKER.encode("utf-8") in raw[:2000]:
        return False  # already has a header - idempotent, skip
    eol = "\r\n" if b"\r\n" in raw else "\n"
    path.write_bytes(make_header(path.name, eol) + raw)
    return True


def main() -> None:
    if len(sys.argv) > 1:
        files = [pathlib.Path(p) for p in sys.argv[1:]]
    else:
        repo_root = pathlib.Path(__file__).resolve().parent.parent
        src = repo_root / "src"
        files = sorted(list(src.glob("*.cpp")) + list(src.glob("*.h")))

    changed = 0
    for f in files:
        if add_header(f):
            changed += 1
            print(f"added header: {f}")
    print(f"done: {changed}/{len(files)} file(s) updated")


if __name__ == "__main__":
    main()
