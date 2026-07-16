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
- Arc (three clicks, live preview) and regular polygon (3–64 sides) tools, both emitting standard FidoCadJ primitives
- Linear dimension tool: two clicks to measure, a third to place the dimension line — arrows, extension lines, and the measured distance in millimeters, all built from standard primitives
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
- Object snap: clicks and drags capture endpoints, midpoints, centers, and intersections of what's already drawn, with a visual marker — each target kind individually configurable
- A measure tool (distance and angle between two clicks, live in the status bar)
- Photoshop/Illustrator-style rulers along the top and left edges of the canvas, synced live with the grid step and current zoom/pan — drag guide lines out of them: positions snap to the guides (and object snap captures their crossings), and they never reach print or export output
- Mouse-wheel zoom anchored to the point under the cursor, at any zoom level and anywhere on the sheet
- Align and distribute tools for tidying up a selection (left/right/top/bottom, center, even spacing), plus snapping a whole selection back onto the grid
- Zoom to fit the whole drawing or just the current selection
- A background tracing image drawn under the grid (Tools > Tracing image...), for redrawing schematics from photos or scans — persisted in the file, invisible to a plain FidoCadJ

### A real editing workflow
- Full undo/redo history for every operation (create, move, resize, mirror, rotate, delete, node edits)
- Boolean operations between closed shapes — union, subtraction, intersection — with results that stay fully FidoCadJ-compatible (optionally as smooth complex curves)
- Shape tools: convert rectangles/ellipses/curves to editable polygons (and back to smooth curves), simplify nodes, fillet or chamfer corners, and grow/shrink a shape by a parallel outline offset (sharp corners preserved, original optionally kept, rectangles and ellipses stay exact)
- AutoCAD-style precision edits with live previews: split a line/curve/track at a clicked point, trim away the stretch of a line between its intersections, extend an end until it meets the first shape on its way
- Whole-selection transforms: rotate by an arbitrary angle, scale by percentage, and replicate on a grid or circularly (clock-face style, with a mouse-picked center)
- A dedicated Modify toolbar grouping every shape-editing command: rotate, mirror, booleans, transforms, align/distribute, macro explode
- Cut / copy / paste / duplicate, multi-selection, select all, invert selection, select same type — including pasting an image straight from the clipboard, paste in place (Ctrl+Shift+V), mirror as copy, and Alt+drag to drag off a duplicate
- Editable geometry right in the Properties panel: a line's length, a rectangle's or ellipse's width and height
- Find (Ctrl+F) across text, macro names, and name/value labels
- Nudge the selection by one grid step with Alt+arrow keys
- A dockable live FCD code panel: watch the drawing as FidoCadJ text while you edit, or edit the text and apply it back as one undo step
- Live status bar: cursor position (grid units and/or millimeters), zoom level, and running counts of primitives and macros in the drawing

### Export & printing
- One export dialog for PNG, JPG, SVG, PDF, EPS and DXF, sized either by resolution (pixels per unit) or by an exact pixel size, with optional antialiasing, black & white, and one-file-per-layer splitting
- Copy the current selection (or the whole drawing) straight to the clipboard as an image
- "Copy split" and "Save split as..." to copy/save with every macro expanded into raw primitives, leaving the drawing itself untouched
- Print with mirroring, black & white, landscape orientation, custom margins, and an option to print a single layer only — fit to page or at true real scale (one unit = 0.127 mm, adjustable percentage) for dimensionally exact output, e.g. toner-transfer PCB etching

### Native FidoCadJ file I/O
- Opens and saves the standard `.fcd` drawing format and `.fcl` library format
- Faithful, idempotent round-tripping — including the `FCJ` extensions (dash styles, arrowheads) and per-document `FJC` configuration
- Also available as a headless command-line converter (`eSchema -n -c ...`) for scripted batch conversion to any of the supported export formats

### DXF import/export
- Import and export AutoCAD DXF drawings, powered by the [libdxfrw](https://github.com/LibreCAD/libdxfrw) library
- A best-effort mapping between FidoCadJ primitives and DXF entities (lines, circles/ellipses, arcs, polylines, splines, text, blocks) in both directions, with a post-import summary of anything that couldn't be mapped

### Safety & convenience
- Autosave with a configurable interval, and crash recovery on the next startup if eSchema didn't close cleanly
- Optional `.bak` backup copy of the previous version on every save
- Recent Files menu (configurable length), drag-and-drop of `.fcd`/`.dxf` files straight onto the main window, and an option to reopen the last file on startup
- Files with content outside the drawing area (e.g. negative coordinates) are shifted back onto the sheet on open, so nothing loads stranded out of sight
- Update checker: automatic on startup (can be turned off) or on demand from the Help menu

### A UI that adapts to you
- Dockable, floatable, tabbable panels (Libraries, Properties, FCD code) — arrange your workspace however you like; the layout, window size, and position are all remembered between sessions
- Customizable toolbars: add, remove, and reorder the commands on the main and Modify toolbars, like in professional CAD packages
- Customizable keyboard shortcuts: reassign or remove any command's shortcut from a searchable editor, with conflict handling and one-click restore of the defaults
- A command palette (Ctrl+Shift+P): search and run any command by name, VS Code-style
- A searchable Options dialog organized in pages, with a live grid preview and per-page defaults — from toolbar icon size and mouse-wheel behavior to snap targets and marker colors
- Seven built-in themes — Light, Dark, Nord, Midnight, Graphite, Sepia, Ocean — each with its own gradients and UI font, plus custom stylesheet support
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

| Layer manager | Circular array of copies |
|---|---|
| ![Layer manager](https://github.com/manufino/eSchema/blob/main/resources/layer_manager.png?raw=true) | ![Circular array of copies](https://github.com/manufino/eSchema/blob/main/resources/array_copies.png?raw=true) |

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

### 1.0.7
- Fixed a crash on the very first launch of a fresh install (no settings file yet), triggered by moving the mouse over the drawing area
- Fixed the Linux/macOS portable packages: the app failed to start with "Qt libraries not found" because the zip step turned a symlink into a stray duplicate of the binary outside its library folder
- **Parametric symbol wizard** (Tools > Symbol wizard): DIP/SIP/quad packages in through-hole, SMD, or schematic style, pin numbers and/or names, live preview, saved straight into any user library — including a brand-new one created from the same dialog
- **Angular and radial dimension tools** with AutoCAD-style picking: hovering highlights the two lines (rectangle edges work too) or the circle, and the center is found automatically
- **Title-block library**: ready-made A4/A3 drawing frames at real paper sizes
- Hold **Shift while drawing** to constrain lines to 45° steps and rectangles/ellipses to squares/circles
- **Offset rework**: the dialog asks only the distance, then the mouse picks the side with a live preview until you click (AutoCAD-style); offset polygons and curves keep exactly the original node count, and every base primitive can now be offset (lines, Béziers, rectangles, ellipses, polygons, complex curves)
- The **command palette search box** now sits centered in the menu bar
- **Nord is the default theme** on a fresh install
- Reliability fixes from a full code review: an undo/redo double free, dangling layer pointers after deleting a layer, verified atomic saves/exports, DXF import/export hardened against malformed files, and lifetime fixes around tool previews
- Performance: settings reads are cached and selection/settings/clipboard notifications are coalesced, removing multi-second freezes with large selections or when applying options

### 1.0.6
- **Customizable toolbars** (View > Customize toolbars...): add, remove, and reorder the commands on the main and Modify toolbars
- **Customizable keyboard shortcuts**: the shortcut list becomes a searchable editor — reassign or remove any command's binding, with conflict handling and one-click defaults
- **Command palette** (Ctrl+Shift+P): search and run any command by name
- **Guide lines** dragged out of the rulers, Illustrator-style: positions snap to them, object snap captures their crossings, and they stay out of print/export output
- New **Dimension tool** (D): arrows, extension lines, and the measured distance in millimeters, placed with a live preview and built from plain primitives
- **AutoCAD-style edits with live previews**: split at point, trim to intersection, extend to intersection
- **Paste in place** (Ctrl+Shift+V) and **Mirror as copy**
- **Offset outline** improvements: optionally keep the original shape, corners stay sharp, rectangles and ellipses produce exact rectangles/ellipses
- **Real-scale printing**: fit to page or a true physical scale (one unit = 0.127 mm, adjustable percentage)
- **Five new built-in themes** — Nord, Midnight, Graphite, Sepia, Ocean — with gradients and per-theme fonts, plus many dark-theme legibility fixes (dialog buttons, menu bar, layer and library icons)
- Mouse-wheel **zoom anchored to the cursor**; files with off-sheet content are pulled back into view on open
- Icons on every menu entry, red control-point markers on all drawing-tool icons, and an Options dialog that applies changes instantly instead of freezing

### 1.0.5
- **Object snap**: clicks and drags capture endpoints, midpoints, centers, and intersections of what's already drawn, with a visual marker and per-kind configuration
- New drawing tools: **Arc** (U) and **Regular polygon** (N), plus a **Measure** tool (W)
- **Shape tools**: convert to polygon/complex curve, simplify nodes, fillet/chamfer corners, parallel outline **offset**
- **Transforms**: rotate by arbitrary angle, scale by percentage, **array of copies** (grid or circular, with a mouse-picked center)
- Selection conveniences: **Alt+drag duplicate**, invert selection, select same type, snap selection to grid
- Editable **geometry in the Properties panel** (line length, rectangle/ellipse size)
- Completely **reworked Options dialog**: searchable sidebar pages, live grid preview, per-page defaults, and many new options (backup `.bak` on save, undo limit, reopen last file, toolbar icon size, antialiasing, coordinate units, wheel zoom, snap tuning, default fonts and colors...)
- Libraries/Properties/FCD panels tabbed on the right by default, with tab icons; window size and position remembered
- Fixes: stale version shown in the About box / false update notifications, main window no longer blocked from shrinking, left-side docking

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
