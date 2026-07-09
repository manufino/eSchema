# eSchema

![eSchema main window](https://github.com/manufino/eSchema/blob/main/resources/win.JPG?raw=true)

**eSchema** is a free, open-source 2D CAD editor for electrical/electronic schematics and PCB sketches, built with C++ and Qt 6. It uses [FidoCadJ](https://darwinne.github.io/FidoCadJ/) as its native drawing format, combining a lightweight, human-readable file format with a modern, dockable desktop UI.

Licensed under the **GNU GPLv3**.

---

## Features

### Drawing primitives
A complete FidoCadJ primitive set, each with on-canvas resize handles, snap-to-grid placement, and mirror/rotate support:

- Line, Bézier curve, closed polygon, complex (spline) curve
- Rectangle and ellipse (filled or outlined)
- Connection dot and PCB track
- PCB pad
- Text labels
- Raster images
- Macros (parts pulled from a component library)

### Component libraries
- Bundled standard FidoCadJ libraries (electrotechnics, PCB footprints, and more)
- A dockable library browser with searchable, previewed parts
- Build your own reusable macros directly from a selection on the sheet, and manage personal libraries/categories from the same panel

### Layers
- Up to 16 independent, colorable layers, matching the FidoCadJ layer model
- A dedicated layer manager to rename, recolor, and toggle layer visibility

### Precision drawing tools
- Configurable grid (dots or lines) with adjustable step and marked-line intervals
- Snap-to-grid for every placement, move, and resize operation
- Photoshop/Illustrator-style rulers along the top and left edges of the canvas, synced live with the grid step and current zoom/pan
- Align and distribute tools for tidying up a selection (left/right/top/bottom, center, even spacing)

### A real editing workflow
- Full undo/redo history for every operation (create, move, resize, mirror, rotate, delete)
- Cut / copy / paste / duplicate, multi-selection, select all
- Live status bar: cursor position (grid units and millimeters), zoom level, and running counts of primitives and macros in the drawing

### Native FidoCadJ file I/O
- Opens and saves the standard `.fcd` drawing format and `.fcl` library format
- Faithful, idempotent round-tripping — including the `FCJ` extensions (dash styles, arrowheads) and per-document `FJC` configuration
- Print and print-preview support

### A UI that adapts to you
- Dockable, floatable, tabbable panels (Libraries, Properties) — arrange your workspace however you like, and it's remembered between sessions
- Light and dark themes, with custom stylesheet support
- Multi-language interface (Italian, English, Spanish)

---

## Screenshots

| Layers | Options |
|---|---|
| ![Layer manager](https://github.com/manufino/eSchema/blob/main/resources/layer_win.JPG?raw=true) | ![Options dialog](https://github.com/manufino/eSchema/blob/main/resources/option_win.JPG?raw=true) |

---

## Getting started

### Requirements
- Qt 6.x (Widgets, Gui, Core, PrintSupport)
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

eSchema is under active development. The core drawing engine, the full FidoCadJ primitive set, layers, libraries, undo/redo, and file I/O are all in place and usable day to day. Expect rough edges and missing niceties in less-traveled corners of the UI — bug reports and feature requests are welcome.

## Contributing

Issues and pull requests are welcome. If you're planning a larger change, consider opening an issue first to discuss the approach.

## License

eSchema is released under the [GNU General Public License v3.0](LICENSE).

## Acknowledgements

- [FidoCadJ](https://darwinne.github.io/FidoCadJ/) — the native file format and component libraries eSchema builds on
- [Remixicon](https://remixicon.com/) — toolbar and UI icons
- [dafont.com](https://www.dafont.com/) — bundled fonts
