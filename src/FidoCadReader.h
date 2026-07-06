/*
 * FidoCadReader.h
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

#ifndef FIDOCADREADER_H
#define FIDOCADREADER_H

#include <QString>
#include <QList>

class Sheet;
class GraphicsPrimitive;

// Parses FidoCadJ (.fcd) text into a Sheet (FIDOSPECS.md 4-7). Per the format's
// robustness contract, malformed or unrecognized lines are skipped rather than
// aborting the parse - reading never "fails" except for an unreadable file.
namespace FidoCadReader {

// Clears `sheet` and populates it from `text`.
void read(const QString &text, Sheet *sheet);

// Reads from disk. Returns false and sets *errorMessage if the file can't be
// opened; parsing itself (via read()) never fails.
bool readFile(const QString &filePath, Sheet *sheet, QString *errorMessage = nullptr);

// Parses `text` into standalone primitives without touching any sheet - used
// by Paste, which must add the result to the existing document (via an undo
// command) rather than replacing it the way read() does. `contextSheet` is
// read-only here, supplying only the connection diameter a bare "SA" line
// should default to; an FJC line in `text` (which a Copy never produces) is
// parsed but has nowhere to be stored and is simply discarded.
QList<GraphicsPrimitive *> parse(const QString &text, Sheet *contextSheet);

} // namespace FidoCadReader

#endif // FIDOCADREADER_H
