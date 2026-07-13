/*
 * DxfReader.h
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DXFREADER_H
#define DXFREADER_H

#include <QString>
#include <QStringList>

class Sheet;

// Parses ASCII DXF (R12 through AC1015-era, tolerant of both LWPOLYLINE and
// legacy POLYLINE/VERTEX/SEQEND) into a Sheet, mirroring FidoCadReader's
// shape. Per-entity malformed/degenerate/unsupported data is skipped rather
// than aborting the parse (matching FidoCadReader's robustness contract);
// reading only hard-fails for an unreadable file or a totally missing
// ENTITIES section.
namespace DxfReader {

// Clears `sheet` and populates it from `text`. Distinct DXF layers
// (up to 16) actually used by a supported entity rename/recolor the
// existing global LayerList slots to match. `warnings`, if given, collects
// one human-readable summary line per kind of skipped/unmapped content
// (e.g. "12 unsupported HATCH entities").
void read(const QString &text, Sheet *sheet, QStringList *warnings = nullptr);

// Reads from disk. Returns false and sets *errorMessage if the file can't be
// opened or has no ENTITIES section; per-entity issues never fail the read,
// they only appear in *warnings.
bool readFile(const QString &filePath, Sheet *sheet, QString *errorMessage = nullptr,
              QStringList *warnings = nullptr);

} // namespace DxfReader

#endif // DXFREADER_H
