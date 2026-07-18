<!--
  Filename:    FIDOSPECS.md
  Author:      Dante Loi
  Date:        2026-05-26
  Description: Precise specification of the FidoCadJ (.fcd) drawing format and
               the related (.fcl) macro-library format, as implemented by
               FidoCadJS.
  Copyright:   Copyright 2026 Dante Loi - GPL v3
  Details:     Documents the full lexical, syntactic, and semantic rules of the
               FCD text format: tokens, primitives, the FCJ/FJC extensions,
               macros, libraries, coordinate system, and worked examples. The
               grammar reflects the behaviour of the FidoCadJS parser
               (ParserActions) and the primitive serializers.
-->

# FidoCadJ Drawing Format (FCD) — Specification

This document specifies the **FCD** text format used by FidoCadJ / FidoCadJS to
describe schematics and PCB drawings, together with the **FCL** macro-library
format. It is intended to be complete enough to implement a conforming reader
and writer.

> **Provenance and credits.** This specification was written by Dante Loi for
> the [FidoCadJS](https://github.com/DanteCpp/FidoCadJS) project (GPL v3) and
> is included here as the reference the eSchema reader/writer is documented
> against — the `FIDOSPECS.md` section numbers cited throughout eSchema's
> source comments refer to this file.
>
> **Notes for eSchema** (verified against the reference FidoCadJ sources and
> eSchema's own implementation; the rest of the document applies as written):
>
> - **`FJC K <layer> <true|false>` is missing from §7.** The reference
>   FidoCadJ editor itself writes and reads it (`ParserActions.java`,
>   `LayerDesc.isLocked()`) to persist a layer's lock state, and eSchema does
>   the same.
> - **`FJC IMG` carries a different payload here.** §7's
>   `FJC IMG <x> <y> <scale> <alpha>` is FidoCadJS's placement-only form (the
>   bitmap lives in browser local storage). eSchema instead writes
>   `FJC IMG <mime> <resolution> <x> <y> <base64>`, embedding the tracing
>   image in the file itself. The two payloads are not interchangeable, but
>   each implementation skips the other's line harmlessly under the format's
>   robustness contract (stock FidoCadJ ignores the sub-code entirely).
> - **Negative coordinates (§3) are a FidoCadJS-only restriction.** Files
>   written by the reference FidoCadJ editor can contain negative coordinates;
>   eSchema accepts them on read and shifts the whole drawing back onto the
>   sheet as one rigid translation instead of clamping each value to 0.
> - **§6.3's "for primitives without an `FCJ` (such as `SA`, `PL`, `PA`,
>   `MC`), the name and value `TY` lines simply follow the primitive line
>   directly" is an erratum.** In the reference implementation
>   (`ParserActions`), a name/value `TY` pair is attached to the previous
>   primitive ONLY when announced by an `FCJ` line - a bare `FCJ` for these
>   types (the reference editor's own files show exactly this pattern), the
>   text flag for the types with FCJ attributes. A `TY` directly following
>   `MC`/`SA`/`PL`/`PA` is a **standalone text primitive**. eSchema follows
>   the reference behavior on both read and write.

---

## 1. Overview

An FCD drawing is a **plain-text, line-oriented** document. Each non-empty line
is a *command*: a whitespace-separated list of tokens whose first token is an
*instruction code* identifying the command. Commands either:

- declare a **graphic primitive** (line, rectangle, oval, text, macro, …),
- attach **extended attributes** to the previous primitive (`FCJ`, `TY`), or
- set a **global configuration** value (`FJC`).

The format is deliberately simple and forgiving: unknown commands are ignored,
malformed lines are skipped, and a file may be parsed incrementally (appended to
an existing drawing).

```
[FIDOCAD]
LI 10 20 30 40 0
RV 25 20 95 75 0
TY 85 25 5 3 0 0 0 * Hello
```

### 1.1 Encoding and line endings

- Text is **UTF-8**. Text fields (in `TY`/`TE` and macro/library names) may
  contain any Unicode characters, including characters outside the BMP.
- Lines are separated by `LF` (`\n`) or `CR LF` (`\r\n`); both are accepted.
  Lone `CR` is tolerated.
- There is no required maximum file length. A single command line may contain at
  most **10000 tokens** (`MAX_TOKENS`); longer lines are truncated with a
  warning.

### 1.2 The header line

The first line is conventionally:

```
[FIDOCAD]
```

This is a **magic marker** used to recognize the file type. The parser itself
does not require it: `[FIDOCAD]` is not a recognized instruction code, so it is
simply ignored like any unknown command. Conforming **writers SHOULD emit it**
as the first line; readers SHOULD treat its presence as confirmation of the
format but MUST NOT fail if it is absent.

---

## 2. Lexical structure

### 2.1 Tokenization

Each line is split into tokens on **single space** characters (`U+0020`).

- One or more interior spaces delimit tokens; empty tokens are not produced for
  runs of spaces in primitive argument lists (leading/trailing handled by the
  reader).
- Tabs and other whitespace are **not** treated as separators — only the space
  character.
- A line consisting only of spaces (no tokens) is an **empty line** and is
  ignored.

### 2.2 Tokens

A token is one of:

- an **integer** (e.g. `10`, `-5`),
- a **floating-point number** (e.g. `1.5`, `0.35`) — used by a few fields,
- an **instruction code** (a fixed two-or-three letter mnemonic), or
- a **free-text fragment** (only in the trailing text field of `TY`/`TE` and in
  macro names, where remaining tokens are re-joined with single spaces).

### 2.3 Whitespace inside strings

Because space is the token separator, strings that must contain spaces use an
escape convention in *font-name* fields:

- The literal sequence `++` represents a single space inside a **font name**.
  On read, `++` → `' '`; on write, `' '` → `++`.
- The single character `*` in a font-name field is a shorthand for the **default
  font** (`Courier New`).

The **text content** of a `TY`/`TE` command is the special last field: every
remaining token is rejoined with single spaces, so ordinary embedded spaces are
preserved verbatim without escaping.

### 2.4 Comments

The format has **no comment syntax**. Any line whose first token is not a
recognized instruction code is silently ignored, which can be used informally,
but this behaviour is not guaranteed to be preserved on round-trip (unknown
lines are dropped on save).

---

## 3. Coordinate system and units

- Coordinates live in the **FidoCadJ logical grid**.
- One grid unit corresponds to **5 mils (0.127 mm)** in the FidoCadJ
  convention; this matters for export scaling, not for the FCD syntax itself.
- The origin `(0, 0)` is at the **top-left**; **x grows right, y grows down**.
- Coordinates are **non-negative** in every mode. Parsed coordinates are clamped
  to the range `[0, +1 000 000]` (`MIN_COORD`/`MAX_COORD`); non-numeric
  coordinates are rejected for the variable-length primitives (`PP`/`PV`,
  `CP`/`CV`).
- Negative coordinates are **not supported**: any negative value in the source
  is clamped to `0` by the lexer, the editor refuses to move primitives to
  negative coordinates, and the viewport cannot pan north-west of the origin.
- **Floating-point coordinates.** FidoCadJS accepts and writes **floating-point**
  coordinates with up to **3 decimals** (trailing zeros and the decimal point
  trimmed, e.g. `12.5`, `12.125`, `13`). Whole numbers are therefore emitted as
  plain integers. For **FidoCadJ compatibility**, simply draw with
  **snap-to-grid enabled**: every point then lands on an integer grid multiple,
  so the file contains only integers. Files that contain fractional coordinates
  are a **FidoCadJS extension** and are *not* guaranteed to be readable by stock
  FidoCadJ. The bounds and non-negativity rule above apply to fractional
  coordinates as well.

### 3.1 Layers

- A drawing has **16 layers**, numbered `0`–`15` (`MAX_LAYERS = 16`).
- Every drawable primitive carries a layer index. An out-of-range or
  non-numeric layer index is coerced to `0`.
- Default layer semantics / colors:

  | # | Name | Default color (R,G,B) | Alpha |
  |---|------|----------------------|-------|
  | 0 | Circuit | 0,0,0 (black) | 1.00 |
  | 1 | Bottom copper | 0,0,128 (navy) | 1.00 |
  | 2 | Top copper | 255,0,0 (red) | 1.00 |
  | 3 | Silkscreen | 0,128,128 (teal) | 1.00 |
  | 4 | Other 1 | 255,200,0 | 1.00 |
  | 5 | Other 2 | 127,255,0 | 1.00 |
  | 6 | Other 3 | 0,255,255 | 1.00 |
  | 7 | Other 4 | 0,128,0 | 1.00 |
  | 8 | Other 5 | 154,205,50 | 1.00 |
  | 9 | Other 6 | 255,20,147 | 1.00 |
  | 10 | Other 7 | 181,155,12 | 1.00 |
  | 11 | Other 8 | 1,128,255 | 1.00 |
  | 12 | Other 9 | 225,225,225 | 0.95 |
  | 13 | Other 10 | 162,162,162 | 0.90 |
  | 14 | Other 11 | 95,95,95 | 0.90 |
  | 15 | Other 12 | 0,0,0 | 1.00 |

Layer color/alpha/name can be overridden per file using `FJC L` / `FJC N`
(§7).

### 3.2 Dash (line) styles

Several primitives accept a *dash style* index (0–4):

| Index | Pattern (dash, gap, …) | Meaning |
|-------|------------------------|---------|
| 0 | `[10, 0]` | solid |
| 1 | `[5, 5]` | dashed |
| 2 | `[2, 2]` | fine dotted |
| 3 | `[2, 5]` | dotted |
| 4 | `[2, 5, 5, 5]` | dash-dot |

Out-of-range indices are clamped into `[0, 4]`.

---

## 4. Document structure and parsing model

A drawing is an **ordered list of primitives**. The parser walks the file line
by line, maintaining a small amount of state so that *follow-up* lines can
augment the primitive declared on a previous line.

Three follow-up mechanisms exist:

1. **`FCJ` line** — immediately *after* a primitive line, supplies FidoCadJ
   extended attributes (arrows, dash style, fill flag, and a “has text” marker).
2. **`TY` lines as name/value** — after a primitive that has an `FCJ` marker set
   to “has text” (`1`), up to two `TY` lines provide the primitive's **name**
   and **value** labels.
3. **Standalone `TY`/`TE`** — when not attached, these declare a free
   **text primitive**.

The reader therefore *defers* committing a primitive until it has seen the line
that follows it, because that line might be an `FCJ` extension. At end of file
any pending primitive is flushed.

> **Robustness contract.** A conforming reader MUST NOT crash on malformed
> input. Unknown instruction codes, truncated lines, too-few/too-many tokens on
> `FCJ`, references to undefined macros, and out-of-range values are all handled
> gracefully (the offending construct is skipped or clamped). Errors on one line
> never abort parsing of the rest of the file.

---

## 5. Graphic primitives

Notation: `<x>` integer coordinate, `<l>` layer index `0–15`, `[...]`
optional, `{a|b}` alternatives. Unless noted, the basic primitive line is a
complete command; the optional `FCJ`/`TY` follow-up lines are described in §6.

### 5.1 `LI` — Line

```
LI <x1> <y1> <x2> <y2> <l>
```

A straight segment from `(x1,y1)` to `(x2,y2)` on layer `l`.

- Minimum tokens: 5 (layer optional but normally present).
- A degenerate line (`x1==x2 && y1==y2`) with no name/value is **not
  serialized** (dropped on save).
- May be followed by an `FCJ` line carrying arrow + dash data (§6.1).

```
LI 10 20 30 40 0
```

### 5.2 `BE` — Bézier curve

```
BE <x1> <y1> <x2> <y2> <x3> <y3> <x4> <y4> <l>
```

A cubic Bézier with endpoints `(x1,y1)`,`(x4,y4)` and control points
`(x2,y2)`,`(x3,y3)`.

- Minimum tokens: 9.
- May be followed by an `FCJ` line with arrow + dash data (§6.1).

```
BE 50 5 20 60 70 35 50 70 0
```

### 5.3 `RV` / `RP` — Rectangle

```
RV <x1> <y1> <x2> <y2> <l>      ; outline (not filled)
RP <x1> <y1> <x2> <y2> <l>      ; filled
```

Axis-aligned rectangle with opposite corners `(x1,y1)` and `(x2,y2)`.
`RV` = void (outline), `RP` = painted (filled).

- Minimum tokens: 5.
- May be followed by an `FCJ` line carrying only `dashStyle` and the text flag
  (§6.2).

```
RV 25 20 95 75 0
RP 10 10 50 40 0
```

### 5.4 `EV` / `EP` — Ellipse / Oval

```
EV <x1> <y1> <x2> <y2> <l>      ; outline
EP <x1> <y1> <x2> <y2> <l>      ; filled
```

Ellipse inscribed in the bounding box `(x1,y1)`–`(x2,y2)`. `EV` = void,
`EP` = painted. Same `FCJ` follow-up form as the rectangle (§6.2).

```
EV 45 15 95 65 0
EP 10 10 40 40 0
```

### 5.5 `PV` / `PP` — Polygon

```
PV <x1> <y1> <x2> <y2> ... <xn> <yn> <l>     ; outline
PP <x1> <y1> <x2> <y2> ... <xn> <yn> <l>     ; filled
```

A polygon with an arbitrary number of vertices. `PV` = void, `PP` = painted.

- Minimum tokens: 6 (at least two vertices + layer).
- Vertex count is capped at `MAX_VERTICES = 10000`; extra vertices are dropped.
- The layer index is the **last** numeric token before an optional `FCJ`.
- May be followed by an `FCJ` line carrying `dashStyle` and the text flag
  (§6.2).
- On serialization the vertex list is emitted with a trailing space before the
  layer index.

```
PV 25 50 65 25 50 85 100 35 100 75 0
PP 10 20 30 40 50 60 0
```

### 5.6 `CV` / `CP` — Complex curve (spline through points)

```
CV <closed> <x1> <y1> ... <xn> <yn> <l>      ; outline
CP <closed> <x1> <y1> ... <xn> <yn> <l>      ; filled
```

A smooth curve interpolating the given control points. `CV` = void,
`CP` = painted.

- The first argument `<closed>` is `1` for a closed curve, `0` for open.
- Minimum tokens: 6.
- Vertex count capped at `MAX_VERTICES`.
- May be followed by an `FCJ` line carrying **arrow + dash** data, like a line
  (§6.1). Arrows are only meaningful for open curves.

```
CP 1 45 45 70 30 45 60 80 50 75 65 95 30 20 30 65 75 0
CV 0 240 5 260 25 240 30 0
```

### 5.7 `SA` — Connection (solder dot)

```
SA <x> <y> <l>
```

A connection point (filled dot) at `(x,y)`. Its drawn diameter is the global
`diameterConnection` (default `2.0`, see §7).

- Minimum tokens: 3.
- Has no `FCJ`. It may still carry name/value `TY` labels (§6.3).

```
SA 70 60 0
```

### 5.8 `PL` — PCB track (line with width)

```
PL <x1> <y1> <x2> <y2> <width> <l>
```

A printed-circuit-board track from `(x1,y1)` to `(x2,y2)` with the given
`<width>` (floating-point, in grid units).

- Minimum tokens: 6.
- `width` is serialized "intelligently": integral values print without a decimal
  point (e.g. `5`), otherwise the full float is emitted.
- No `FCJ`; may carry name/value `TY` labels.

```
PL 10 110 90 110 5 0
```

### 5.9 `PA` — PCB pad

```
PA <x> <y> <rx> <ry> <ri> <style> <l>
```

A PCB pad centered at `(x,y)`.

- `rx`, `ry` — outer width/height (grid units).
- `ri` — internal hole diameter.
- `style` — pad shape:
  - `0` = oval/round,
  - `1` = rectangular,
  - `2` = rounded rectangle.
- Minimum tokens: 7.
- No `FCJ`; may carry name/value `TY` labels.

```
PA 50 40 10 10 5 0 0     ; round pad, hole 5
PA 30 135 10 10 2 1 0    ; rectangular
PA 45 135 10 10 2 2 0    ; rounded rectangle
```

### 5.10 `MC` — Macro instance

```
MC <x> <y> <orientation> <mirror> <macroName...>
```

Places an instance of a library macro (a reusable sub-drawing) with its
reference point at `(x,y)`.

- `orientation` — integer `0–3`, multiplied by 90° (`0`=0°, `1`=90°, `2`=180°,
  `3`=270°).
- `mirror` — `1` if horizontally mirrored, `0` otherwise.
- `macroName` — the macro **key**; all remaining tokens are joined with single
  spaces and **lowercased**. Keys are usually of the form
  `library.macroname` (a prefix, a dot, and the local name).
- Minimum tokens: 6.
- The referenced macro must exist in the loaded library set; an unknown macro is
  skipped without error.
- May carry name/value `TY` labels which become the macro's name and value
  annotations (e.g. a reference designator and a part value).

```
MC 100 100 0 0 ihram.resistor
MC 50 50 2 1 ttl.7400
```

Macro expansion is recursive (a macro body may instantiate other macros), with
recursion limited to depth `MAX_MACRO_DEPTH = 16`.

### 5.11 `TY` / `TE` — Text

`TY` (advanced text) is the canonical text command:

```
TY <x> <y> <sizeY> <sizeX> <orientation> <style> <l> <font> <text...>
```

- `(x,y)` — anchor (top-left of the text box).
- `sizeY`, `sizeX` — vertical and horizontal size (note the **Y-before-X**
  order). Typical aspect is `sizeY/sizeX = 10/7`.
- `orientation` — rotation in **degrees** (integer).
- `style` — bit flags:
  - bit 0 (`1`) = **bold**,
  - bit 1 (`2`) = **italic**,
  - bit 2 (`4`) = **mirrored**.
  (e.g. `0`=plain, `1`=bold, `3`=bold+italic.)
- `font` — font name; `*` means the default font (`Courier New`), and `++`
  encodes spaces inside the name.
- `text` — the displayed string; all remaining tokens, rejoined with single
  spaces, preserving embedded spaces. May contain `$...$` LaTeX math.
- Minimum tokens: 9.

```
TY 85 25 5 3 0 0 0 * Hello
TY 35 35 4 3 0 0 0 Helvetica This is a test
TY 225 40 8 3 20 0 0 * Rotated 20deg
TY 220 10 4 3 0 4 0 * Mirrored
```

`TE` is the **legacy/basic** text command:

```
TE <x> <y> <text...>
```

- Minimum tokens: 4.
- Equivalent to a `TY` with fixed defaults: `sizeY=4`, `sizeX=3`,
  `orientation=0`, `style=0`, `font=*`, `layer=0`.
- On save it is **always emitted as `TY`** — i.e. a reader/writer round-trip
  upgrades `TE` to its `TY` equivalent.

```
TE 10 20 Hello      →  TY 10 20 4 3 0 0 0 * Hello
```

> **Dual role of `TY`.** A `TY` line is a standalone text primitive *unless* it
> immediately follows a primitive whose preceding `FCJ` set the “has text”
> marker. In that position the first `TY` provides the primitive's **name** and
> the second its **value** (§6.3).

### 5.12 `IM` — Embedded image (FidoCadJS extension)

```
IM <x1> <y1> <x2> <y2> <opacity> <hue> <l> <mime> <data>
```

A raster image occupying the bounding box `(x1,y1)`–`(x2,y2)`.

- `(x1,y1)`, `(x2,y2)` — opposite corners of the image's placement rectangle,
  same role as `RV`/`EV`. Dragging either corner handle resizes the image;
  dragging the body moves it.
- `opacity` — float `0–1` (3 decimals, trailing zeros trimmed).
- `hue` — integer hue-rotation in degrees, `0–359`.
- `l` — layer index `0–15`, same clamping as every other primitive.
- `mime` — the image's MIME subtype with no slash (`png`, `jpeg`, `gif`,
  `webp`, `bmp`, `svg+xml`, …); reconstructed as
  `data:image/<mime>;base64,<data>`.
- `data` — **base64 of the original, unmodified file bytes** (no re-encoding,
  so no quality loss beyond the source file itself), as the final token.
  Base64's alphabet contains no spaces, so this is always a single token; a
  reader should still rejoin any accidentally-split trailing tokens (the same
  defensive rule `TY`'s text field uses).
- Minimum tokens: 9 (plus the `IM` code itself = 10).
- A degenerate (zero-area) or data-less image is **not serialized** (dropped
  on save), matching the rule already applied to `LI`.
- May carry name/value `TY` labels exactly like `SA`/`PL`/`PA`/`MC` — a bare
  `FCJ` line (no arguments) signals that one or two `TY` lines follow (§6.3).

```
IM 20 20 90 60 1 0 0 png iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=
```

Unlike the `FJC IMG` background-image placement (§7), which stores only
geometry and keeps the bitmap in the browser's local storage, `IM` embeds the
full image so the file round-trips on its own: reopening it, undoing/redoing,
and copy/paste all reconstruct the exact same pixels, position, opacity, and
hue with no external state.

> **Export note.** SVG export embeds the image natively (`<image>`), and PDF
> export embeds a real raster XObject. PGF and TikZ export draw a dashed
> placeholder box labeled `[image]` instead, since embedding a raster inline
> in those text-based LaTeX formats is not supported by this exporter (see
> README "Known Export Limitations").

---

## 6. The `FCJ` extension and primitive name/value labels

`FCJ` (“FidoCadJ”) lines extend the immediately preceding primitive with
attributes that were not part of the original FidoCAD format. An `FCJ` line is
written **only when needed** (i.e. when the primitive actually has an arrow, a
non-solid dash, or a name/value label), so legacy-compatible files omit it.

The last numeric token of an `FCJ` line is the **text flag**:

- `1` — the primitive has a name and/or value, supplied by the following one or
  two `TY` lines.
- `0` — no attached text.

### 6.1 `FCJ` for arrowed primitives (`LI`, `BE`, `CV`/`CP`)

```
FCJ <arrows> <arrowStyle> <arrowLength> <arrowHalfWidth> <dashStyle> <textFlag>
```

- `arrows` — bitmask: bit 0 (`1`) = arrow at **start**, bit 1 (`2`) = arrow at
  **end** (`3` = both, `0` = none).
- `arrowStyle` — bitmask: bit 0 (`1`) = **limiter/bar**, bit 1 (`2`) =
  **empty/open** head (otherwise filled).
- `arrowLength`, `arrowHalfWidth` — arrow geometry (float; integral values
  printed without decimals).
- `dashStyle` — dash index `0–4` (§3.2).
- `textFlag` — `0`/`1` as above.

```
LI 5 50 90 50 0
FCJ 2 0 3 1 0 0          ; arrow at end, filled head, length 3, halfwidth 1
```

### 6.2 `FCJ` for `RV`/`RP`, `EV`/`EP`, `PV`/`PP`

These shapes have no arrows; their `FCJ` carries only:

```
FCJ <dashStyle> <textFlag>
```

```
RV 50 20 60 30 0
FCJ 2 0                  ; fine-dotted outline, no text
```

### 6.3 Name and value via `TY`

When a primitive's `FCJ` text flag is `1`, the parser reads up to two following
`TY` lines and assigns them, in order, as the primitive's **name** and
**value**:

```
<primitive line>
FCJ ... 1
TY <x> <y> <sizeY> <sizeX> <o> <style> <l> <font> <name text...>
TY <x> <y> <sizeY> <sizeX> <o> <style> <l> <font> <value text...>
```

For primitives without an `FCJ` (such as `SA`, `PL`, `PA`, `MC`), the name and
value `TY` lines simply follow the primitive line directly; the writer emits an
`FCJ` marker first only for those primitive types that use one.

Example: a connection with a label.

```
SA 70 60 0
TY 75 65 4 3 0 0 0 * N1
TY 75 70 4 3 0 0 0 * net1
```

Example: a macro with reference designator and value.

```
MC 100 100 0 0 ihram.resistor
TY 110 105 4 3 0 0 0 * R1
TY 110 110 4 3 0 0 0 * 10k
```

> **Note on `saveText`.** The name/value `TY` lines write the **value's**
> font/size from the primitive; both labels share the primitive's `macroFont`
> and `macroFontSize`. The serializer emits `sizeY = floor(fontSize*4/3)` and
> `sizeX = fontSize`.

---

## 7. Global configuration: `FJC`

`FJC` lines set drawing-wide configuration. They are written only by FidoCadJS
("extensions" mode) and only when a value differs from its default, so they are
typically near the top of the file. Unknown sub-codes are ignored.

```
FJC C <diameter>            ; connection-dot diameter (default 2.0)
FJC A <lineWidth>           ; default line width (default 0.5)
FJC B <lineWidthCircles>    ; circle/oval line width (default 0.35)
FJC L <layer> <rgb> <alpha> ; override a layer's color and alpha
FJC N <layer> <name...>     ; override a layer's display name
FJC IMG <x> <y> <scale> <alpha>  ; attached background image placement
```

Details:

- `FJC C` — sets `diameterConnection` if the value is `> 0`.
- `FJC A` — sets the global `lineWidth` if `> 0`.
- `FJC B` — sets `lineWidthCircles` if `> 0`.
- `FJC L <layer> <rgb> <alpha>` — `rgb` is a packed 24-bit integer
  (`R<<16 | G<<8 | B`); `alpha` is a float `0..1`. Marks the layer as modified.
- `FJC N <layer> <name...>` — remaining tokens (space-joined) become the layer
  description.
- `FJC IMG <x> <y> <scale> <alpha>` — placement of an attached background
  reference image. The image **pixel data is not stored in the FCD file**; only
  its placement metadata is. (FidoCadJS keeps the bitmap in browser local
  storage and reconnects it on load.)

```
[FIDOCAD]
FJC C 1.5
FJC A 0.35
FJC B 0.25
FJC L 2 16711680 1
FJC N 2 Top Copper
LI 25 45 110 25 0
```

A writer in **non-extension mode** (`extensions = false`) emits no `FJC` lines
at all and no `FCJ` lines, producing a drawing that is maximally compatible with
the original FidoCAD reader.

---

## 8. Macro libraries (`.fcl`)

A **library file** (`.fcl`) defines reusable macros that `MC` commands
reference. Libraries are line-oriented but use a different block syntax from
drawings.

### 8.1 Library lexical rules

- Lines are trimmed; lines of length ≤ 1 are skipped.
- A line beginning with `{` opens a **category**: the text up to the closing `}`
  is the category name; it applies to subsequent macros.
- A line beginning with `[` opens a **macro definition** header:

  ```
  [<key> <long descriptive name...>
  ```

  - `<key>` is the first whitespace/`]`-delimited token — the macro's local
    name.
  - The remaining text up to `]` is the macro's human-readable long name.
  - The special key `FIDOLIB` is not a macro: its long name sets the **library
    name** for all following macros.
- All subsequent lines (until the next `[`) form the macro's **body** — an
  embedded FCD drawing (the same primitive commands described above).

### 8.2 Macro keys and prefixes

When a library is loaded with a *prefix* `P`, each macro is registered under the
key `P.key` (or just `key` when the prefix is empty), **lowercased**. `MC`
commands must reference this fully-qualified, lowercased key.

The **standard library** (`FCDstdlib`) is loaded with an *empty* prefix, so its
macros are registered and referenced by their bare `key` alone, **without any
prefix** (e.g. `MC 100 100 0 0 080`). Only non-standard libraries use the
`library.macroname` form.

### 8.3 Example library

```
[FIDOCAD]
{Electronics}
[RLED RLED - Red LED
LI 0 0 10 0 0
LI 10 0 20 0 0
EV 5 -5 15 5 0
]
[RESISTOR Resistor
LI 0 0 5 0 0
RV 5 -3 25 3 0
LI 25 0 30 0 0
]
```

Loaded with prefix `electronics`, these become keys `electronics.rled` and
`electronics.resistor`, usable as:

```
MC 100 100 0 0 electronics.rled
```

---

## 9. Serialization rules (writer behaviour)

A conforming writer SHOULD:

1. Emit `[FIDOCAD]` as the first line.
2. In extension mode, emit any non-default `FJC` configuration (connection
   diameter, line widths, modified layers, attached-image placement) before the
   primitives.
3. Emit each primitive in document order via its canonical command.
4. Emit an `FCJ` line **only** when the primitive has a non-default attribute
   (arrow, dash ≠ 0, or a name/value label).
5. Emit name/value as `TY` lines after the primitive (preceded by `FCJ` for the
   primitive types that use it).
6. Normalize numeric output:
   - integral floats print without a decimal point (`roundIntelligently`);
   - the default font prints as `*`; spaces in font names print as `++`.
7. Drop primitives that carry no information (e.g. a zero-length `LI` with no
   labels; a single-point complex curve with no labels).
8. Always upgrade `TE` to its `TY` equivalent.

Round-trip property: parsing a written document and re-writing it MUST produce
byte-identical output (idempotent serialization).

---

## 10. Instruction-code reference

| Code | Meaning | Core argument shape |
|------|---------|---------------------|
| `LI` | Line | `x1 y1 x2 y2 l` |
| `BE` | Bézier curve | `x1 y1 x2 y2 x3 y3 x4 y4 l` |
| `RV` / `RP` | Rectangle (void / filled) | `x1 y1 x2 y2 l` |
| `EV` / `EP` | Ellipse (void / filled) | `x1 y1 x2 y2 l` |
| `PV` / `PP` | Polygon (void / filled) | `x1 y1 … xn yn l` |
| `CV` / `CP` | Complex curve (void / filled) | `closed x1 y1 … xn yn l` |
| `SA` | Connection dot | `x y l` |
| `PL` | PCB track | `x1 y1 x2 y2 width l` |
| `PA` | PCB pad | `x y rx ry ri style l` |
| `MC` | Macro instance | `x y orientation mirror name…` |
| `TY` | Advanced text / name-value label | `x y sizeY sizeX o style l font text…` |
| `TE` | Basic text (legacy) | `x y text…` |
| `IM` | Embedded image (FidoCadJS extension) | `x1 y1 x2 y2 opacity hue l mime data` |
| `FCJ` | Extended attributes for previous primitive | varies (§6) |
| `FJC` | Global configuration | `sub-code args…` (§7) |

---

## 11. Complete worked example

A small schematic fragment combining several features:

```
[FIDOCAD]
FJC C 1.5
FJC B 0.25
LI 25 45 110 25 0
LI 5 50 90 50 0
FCJ 2 0 3 1 0 0
RV 25 20 95 75 0
FCJ 2 1
TY 30 25 4 3 0 0 0 * BOX
TY 30 30 4 3 0 0 0 * v1
EP 10 10 40 40 2
PL 10 110 90 110 5 0
PA 50 40 10 10 5 0 0
SA 70 60 0
CV 0 240 5 260 25 240 30 0
TY 220 10 4 3 0 4 0 * Title
MC 100 100 0 0 electronics.rled
TY 110 105 4 3 0 0 0 * D1
TY 110 110 4 3 0 0 0 * red
```

This document:

- raises the connection-dot diameter to `1.5` and the circle line width to
  `0.25`;
- draws two lines, the second with a filled arrow at its end;
- draws an outlined, fine-dotted rectangle named `BOX` with value `v1`;
- draws a filled ellipse on the top-copper layer;
- draws a PCB track and a round PCB pad;
- places a connection dot;
- draws an open complex curve;
- writes a mirrored title text; and
- instantiates the `electronics.rled` macro labelled `D1` / `red`.

---

## 12. Conformance summary

A reader is conforming if it:

- tokenizes on single spaces, accepts `LF`/`CRLF`/lone-`CR`;
- ignores unknown instruction codes and the `[FIDOCAD]` header;
- parses all primitives in §5 and the `FCJ`/`TY`/`FJC` extensions in §6–§7;
- clamps coordinates and layer indices, caps vertex counts, and never aborts the
  whole parse on a single malformed line;
- treats `TY` after a text-flagged primitive as name/value, otherwise as a text
  primitive.

A writer is conforming if it follows §9 and produces idempotent output.
