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
- A dockable library browser with searchable, previewed parts and a large preview panel for the selected macro
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
- A background tracing image drawn under the grid (Tools > Tracing image...), for redrawing schematics from photos or scans — persisted in the file, invisible to a plain FidoCadJ

### A real editing workflow
- Full undo/redo history for every operation (create, move, resize, mirror, rotate, delete, node edits)
- Boolean operations between closed shapes — union, subtraction, intersection — with results that stay fully FidoCadJ-compatible (optionally as smooth complex curves)
- A dedicated Modify toolbar grouping every shape-editing command: rotate, mirror, booleans, align/distribute, macro explode
- Cut / copy / paste / duplicate, multi-selection, select all — including pasting an image straight from the clipboard
- Find (Ctrl+F) across text, macro names, and name/value labels
- Nudge the selection by one grid step with Alt+arrow keys
- A dockable live FCD code panel: watch the drawing as FidoCadJ text while you edit, or edit the text and apply it back as one undo step
- Live status bar: cursor position (grid units and millimeters), zoom level, and running counts of primitives and macros in the drawing

### Export & printing
- One export dialog for PNG, JPG, SVG, PDF, EPS and DXF, sized either by resolution (pixels per unit) or by an exact pixel size, with optional antialiasing, black & white, and one-file-per-layer splitting
- Copy the current selection (or the whole drawing) straight to the clipboard as an image
- "Copy split" and "Save split as..." to copy/save with every macro expanded into raw primitives, leaving the drawing itself untouched
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
- Update checker: automatic on startup (can be turned off) or on demand from the Help menu

### A UI that adapts to you
- Dockable, floatable, tabbable panels (Libraries, Properties, FCD code) — arrange your workspace however you like, and it's remembered between sessions
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

English is the language the interface is natively written in, and Italian is maintained natively by the project too (this is originally an Italian-authored app). The other 10 translations are complete but were produced with the help of **Claude** (Anthropic's AI assistant) and have **not** been reviewed by native speakers — some strings may be awkward or plain wrong. If you spot an incorrect translation, please [open an issue](https://github.com/manufino/eSchema/issues) or a pull request against the corresponding `translations/eSchema_<lang>.ts` file; corrections from native speakers are very welcome.

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

## What's new

### 1.0.4
- **Boolean operations** between closed shapes (union, subtraction, intersection), applied as one undo step; curved results can optionally come out as smooth complex curves instead of flattened polygons
- New **Modify toolbar**, docked below the main one, grouping every shape-editing command (rotate, mirror, booleans, align/distribute, macro explode)
- **Dockable live FCD code panel** — see and edit the drawing as raw FidoCadJ text
- **Background tracing image** under the grid (Tools > Tracing image...), persisted in the `.fcd` file
- **Find** (Ctrl+F) across text, macro names, and name/value labels
- Large **macro preview panel** in the Libraries dock
- **Paste an image from the clipboard**; "Copy split" / "Save split as..." with macros expanded
- **Update checker**, automatic on startup or manual from the Help menu
- All 11 non-English **translations completed** (100%)
- Fixed docking to the left side; the saved window layout is now versioned

### 1.0.3
- Unified **export dialog** (PNG/JPG/SVG/PDF/EPS/DXF) shared with the new headless **command-line batch converter**; EPS export added
- **Autosave** with crash recovery
- **Print options**: mirror, black & white, landscape, margins, single layer
- Recent Files menu, **drag & drop** of `.fcd`/`.dxf`, unsaved-changes prompts
- **Node insertion/removal** on polygons and complex curves
- Zoom to selection, Alt+arrows nudge, "Copy as image"
- **Decorated text** (superscripts/subscripts) and working per-layer visibility/lock
- FidoCadJ-style grid rendering and default 5-unit pitch; grid/snap toggles on the main toolbar
- English became the source language of the interface

### 1.0.2
- **DXF import/export**, powered by libdxfrw
- **Light/dark/system themes** with custom stylesheet support

### 1.0.1
- First stable release (BETA label dropped), with automated release packaging for Windows, Linux and macOS
- **Multi-language interface** (11 translations)
- **Rulers** along the canvas edges and live primitive/macro counts in the status bar
- **Image primitive** with opacity, grayscale and aspect-ratio lock
- PDF/SVG/PNG export from the File menu and a reorganized Options dialog

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
