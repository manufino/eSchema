/*
 * FidoCadWriter.h
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

#ifndef FIDOCADWRITER_H
#define FIDOCADWRITER_H

#include <QString>
#include <QList>

class Sheet;
class GraphicsPrimitive;

// Serializes a Sheet's primitives to FidoCadJ (.fcd) text (FIDOSPECS.md 9).
// Writing is the mirror image of FidoCadReader: each primitive's own
// toTokens() plus, where applicable, an FCJ line (arrow/dash/text-flag) and
// TY name/value lines.
namespace FidoCadWriter {

// Returns the full file contents (including the "[FIDOCAD]" header line).
QString write(const Sheet *sheet);

// Like write(), but serializes `primitives` instead of sheet->primitives() -
// `sheet` is only consulted for its document-wide FJC config (connection
// diameter/line widths) and the global LayerList's lock state, never
// iterated itself. Used by "Copy/Save split" to substitute a macro-expanded
// primitive list without needing those primitives to actually belong to a
// Sheet (GraphicsPrimitive::convertToPrimitives()'s result never is one).
QString writeExpanded(const Sheet *sheet, const QList<GraphicsPrimitive *> &primitives);

// Writes to disk. Returns false and sets *errorMessage on failure to open
// the file for writing.
bool writeFile(const Sheet *sheet, const QString &filePath, QString *errorMessage = nullptr);

// writeExpanded()'s equivalent of writeFile().
bool writeExpandedFile(const Sheet *sheet, const QList<GraphicsPrimitive *> &primitives,
                        const QString &filePath, QString *errorMessage = nullptr);

// Serializes just the given primitives (in the given order), with no
// "[FIDOCAD]" header and no document-wide FJC config line - used by Copy/Cut
// to put a FidoCadJ-text fragment on the system clipboard, mirroring the
// reference FidoCadJ editor's CopyPasteActions.copySelected(), which copies
// only the selected elements' own lines rather than a whole document.
QString writeSelection(const QList<GraphicsPrimitive *> &primitives);

} // namespace FidoCadWriter

#endif // FIDOCADWRITER_H
