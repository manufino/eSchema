/*
 * LibraryManager.h
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

#ifndef LIBRARYMANAGER_H
#define LIBRARYMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QPixmap>

#include "MacroDescriptor.h"

class GraphicsPrimitive;
class Sheet;

// One loaded ".fcl" library file: its display name plus every macro it
// defines, grouped by category in file order (matching the reference
// FidoCadJ editor's own library browser).
struct MacroLibrary
{
    QString filename;    // the file-derived prefix ("", "pcb", "ihram", ...)
    QString displayName; // the "[FIDOLIB ...]" header text
    QStringList categoryOrder;
    QHash<QString, QList<MacroDescriptor>> macrosByCategory;
};

// Scans the "lib" directory next to the running executable for ".fcl" macro
// libraries (matching the reference FidoCadJ editor's own DIR_LIBS
// convention, LibUtils.getLibDir()) and indexes every macro they define.
// Also expands each macro's body into real primitives on first use, so
// PrimitiveMacro can paint the actual symbol instead of a placeholder box.
class LibraryManager : public QObject
{
    Q_OBJECT
public:
    static LibraryManager &getInstance();

    // (Re)scans the library directory, clearing and rebuilding everything -
    // safe to call more than once.
    void loadLibraries();

    const QList<MacroLibrary> &libraries() const { return m_libraries; }
    const MacroDescriptor *macro(const QString &key) const;

    // The macro's body expanded into primitives, still in the library's own
    // (100,100)-centered local coordinate space (never translated/rotated) -
    // built once per key and cached forever. Returns an empty list for an
    // unrecognized key. The returned primitives are owned by LibraryManager:
    // never delete them, and never add them to a Sheet - PrimitiveMacro reads
    // them read-only and places them itself via a QPainter transform.
    //
    // Returned BY VALUE, not by reference: a macro's body can itself contain
    // "MC" lines referencing other macros (e.g. IHRAM/elettrotecnica bodies
    // routinely delegate to bare FCDstdlib keys for shared sub-symbols like
    // ground/ports), so painting one macro's body recursively calls this
    // same function again for each nested one, inserting new entries into
    // m_expandedBodies *while a caller may still be iterating a previous
    // result*. QHash::insert() can rehash and invalidate every existing
    // reference into the table, so a caller holding a reference from an
    // earlier call would silently start iterating freed/moved memory -
    // returning a copy (cheap: it's just a QList of pointers) sidesteps that
    // entirely.
    QList<GraphicsPrimitive *> expandedBody(const QString &key);

    // A small preview icon for the macro's body, fitted and centered within
    // a size x size square. Cached forever per (key, size).
    QPixmap icon(const QString &key, int size);

signals:
    void librariesReloaded();

private:
    LibraryManager() = default;
    ~LibraryManager();
    LibraryManager(const LibraryManager &) = delete;
    LibraryManager &operator=(const LibraryManager &) = delete;

    void loadLibraryFile(const QString &filePath);
    Sheet *parseContext();

    QList<MacroLibrary> m_libraries;
    QHash<QString, MacroDescriptor> m_macrosByKey;
    QHash<QString, QList<GraphicsPrimitive *>> m_expandedBodies;
    QHash<QString, QPixmap> m_iconCache;
    // Supplies the default connection diameter FidoCadReader::parse() needs
    // for a bare "SA" line - never shown/added to a view, just a data holder.
    Sheet *m_parseContext = nullptr;
};

#endif // LIBRARYMANAGER_H
