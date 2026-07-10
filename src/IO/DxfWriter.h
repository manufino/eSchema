/*
 * DxfWriter.h
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

#ifndef DXFWRITER_H
#define DXFWRITER_H

#include <QString>

class Sheet;

// Serializes a Sheet's primitives to ASCII DXF ($ACADVER AC1015 / AutoCAD
// 2000), mirroring FidoCadWriter's shape. Every PrimitiveMacro instance is
// flattened via its own convertToPrimitives() first (recursively, up to a
// depth guard) - DXF export never writes BLOCKS/INSERT, only loose entities.
// Primitive types with no DXF equivalent (PrimitiveImage) are silently
// skipped, matching FidoCadReader/Writer's own "not everything round-trips"
// posture.
namespace DxfWriter {

// Returns the full file contents.
QString write(const Sheet *sheet);

// Writes to disk. Returns false and sets *errorMessage on failure to open
// the file for writing.
bool writeFile(const Sheet *sheet, const QString &filePath, QString *errorMessage = nullptr);

} // namespace DxfWriter

#endif // DXFWRITER_H
