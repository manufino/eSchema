# eSchema

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![Platforms](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)](#getting-started)
[![Release](https://img.shields.io/github/v/release/manufino/eSchema?include_prereleases)](https://github.com/manufino/eSchema/releases)

![eSchema main window](https://github.com/manufino/eSchema/blob/main/resources/main_window.png?raw=true)

**eSchema** is a free, open-source 2D CAD editor for electrical/electronic schematics and PCB sketches, built with C++ and Qt 6. It uses [FidoCadJ](https://github.com/FidoCadJ/FidoCadJ) as its native drawing format, combining a lightweight, human-readable file format with a modern, dockable desktop UI.

Licensed under the **GNU GPLv3**.

---

## Features

### Drawing primitives
A complete FidoCadJ primitive set, each with on-canvas resize handles, snap-to-grid placement, and mirror/rotate support:

- Line, Bézier curve, closed polygon, complex (spline) curve — with node add/remove editing directly on the canvas
- Rectangle and ellipse (filled or outlined)
- Connection dot and PCB track
- PCB pad
- Text labels, with superscript/subscript markup
- Raster images
- Macros (parts pulled from a component library)

### Component libraries
- Bundled standard FidoCadJ libraries (electrotechnics, PCB footprints, and more)
- A dockable library browser with searchable, previewed parts
- Build your own reusable macros directly from a selection on the sheet, and manage personal libraries/categories from the same panel

### Layers
- Up to 16 independent, colorable layers, matching the FidoCadJ layer model
- A dedicated layer manager to rename, recolor, and toggle per-layer visibility and lock

### Precision drawing tools
- Configurable grid (dots or lines) with adjustable step and marked-line intervals
- Snap-to-grid for every placement, move, and resize operation, with one-click toggles for both right on the toolbar
- Photoshop/Illustrator-style rulers along the top and left edges of the canvas, synced live with the grid step and current zoom/pan
- Align and distribute tools for tidying up a selection (left/right/top/bottom, center, even spacing)
- Zoom to fit the whole drawing or just the current selection

### A real editing workflow
- Full undo/redo history for every operation (create, move, resize, mirror, rotate, delete, node edits)
- Cut / copy / paste / duplicate, multi-selection, select all
- Nudge the selection by one grid step with Alt+arrow keys
- Live status bar: cursor position (grid units and millimeters), zoom level, and running counts of primitives and macros in the drawing

### Export & printing
- One export dialog for PNG, JPG, SVG, PDF, EPS and DXF, sized either by resolution (pixels per unit) or by an exact pixel size, with optional antialiasing, black & white, and one-file-per-layer splitting
- Copy the current selection (or the whole drawing) straight to the clipboard as an image
- Print with mirroring, black & white, landscape orientation, custom margins, and an option to print a single layer only

### Native FidoCadJ file I/O
- Opens and saves the standard `.fcd` drawing format and `.fcl` library format
- Faithful, idempotent round-tripping — including the `FCJ` extensions (dash styles, arrowheads) and per-document `FJC` configuration
- Also available as a headless command-line converter (`eSchema -n -c ...`) for scripted batch conversion to any of the supported export formats

### DXF import/export
- Import and export AutoCAD DXF drawings, powered by the [libdxfrw](https://github.com/LibreCAD/libdxfrw) library
- A best-effort mapping between FidoCadJ primitives and DXF entities (lines, circles/ellipses, arcs, polylines, splines, text, blocks) in both directions, with a post-import summary of anything that couldn't be mapped

### Safety & convenience
- Autosave with a configurable interval, and crash recovery on the next startup if eSchema didn't close cleanly
- Recent Files menu, and drag-and-drop of `.fcd`/`.dxf` files straight onto the main window

### A UI that adapts to you
- Dockable, floatable, tabbable panels (Libraries, Properties) — arrange your workspace however you like, and it's remembered between sessions
- Light and dark themes, with custom stylesheet support
- A fully translated, multi-language interface (see below)

---

## Supported languages

eSchema's interface is available in:

| | | | |
|---|---|---|---|
| 🇬🇧 English | 🇮🇹 Italiano | 🇩🇪 Deutsch | 🇫🇷 Français |
| 🇪🇸 Español | 🇷🇺 Русский | 🇨🇳 中文 | 🇵🇱 Polski |
| 🇯🇵 日本語 | 🇵🇹 Português | 🇸🇦 العربية | 🇮🇳 हिन्दी |

English is the language the interface is natively written in, and Italian is maintained natively by the project too (this is originally an Italian-authored app). The other 10 translations were produced with the help of **Claude** (Anthropic's AI assistant) and have **not** all been reviewed by native speakers — some strings may be missing, awkward, or plain wrong. If you spot an incorrect translation, please [open an issue](https://github.com/manufino/eSchema/issues) or a pull request against the corresponding `translations/eSchema_<lang>.ts` file; corrections from native speakers are very welcome.

---

## Screenshots

| Layer manager | Component libraries |
|---|---|
| ![Layer manager](https://github.com/manufino/eSchema/blob/main/resources/layer_manager.png?raw=true) | ![Library panel](https://github.com/manufino/eSchema/blob/main/resources/library_panel.png?raw=true) |

---

## Getting started

### Requirements
- Qt 6.x (Widgets, Gui, Core, PrintSupport, Svg)
- A C++17-capable compiler (MSVC, MinGW, or GCC/Clang on Linux/macOS)

### Build

```bash
qmake eSchema.pro
# MinGW / Linux / macOS
make            # or: mingw32-make

# MSVC
nmake
```

The resulting binary picks up the bundled component libraries from the `lib/` folder next to it at runtime.

### Open Qt Creator instead

Just open `eSchema.pro` in Qt Creator, pick a Qt 6 kit, and build/run — no extra configuration needed.

---

## Project status

eSchema is under active development. The core drawing engine, the full FidoCadJ primitive set, layers, libraries, undo/redo, file I/O, export/printing, and autosave are all in place and usable day to day. Expect rough edges and missing niceties in less-traveled corners of the UI — bug reports and feature requests are welcome.

## Contributing

Issues and pull requests are welcome — for code changes, translation fixes, or anything in between. If you're planning a larger change, consider opening an issue first to discuss the approach.

## License

eSchema is released under the [GNU General Public License v3.0](LICENSE).

## Acknowledgements

- [FidoCadJ](https://github.com/FidoCadJ/FidoCadJ) — the native file format and component libraries eSchema builds on
- [libdxfrw](https://github.com/LibreCAD/libdxfrw) — DXF import/export (vendored under `third_party/libdxfrw/`, GPL-2.0-or-later)
- [Remixicon](https://remixicon.com/) — toolbar and UI icons
- [dafont.com](https://www.dafont.com/) — bundled fonts
