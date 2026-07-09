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

    // True for FidoCadJ's own 5 standard libraries (the unprefixed original
    // FidoCAD library, stored with filename=="", plus pcb/ihram/
    // elettrotecnica/ey_libraries) - these ship with eSchema and are never
    // written to. Also usable on a filename that isn't loaded yet, to reject
    // creating a *new* library under one of these reserved names.
    bool isStandardLibraryFilename(const QString &filename) const;
    // Display names of every loaded library that isn't standard - "Crea
    // macro dalla selezione"'s save-target picker only ever offers these.
    QStringList userLibraryDisplayNames() const;
    // The real filename (prefix) of the user library whose display name is
    // exactly `displayName`, or empty if none matches (including if
    // `displayName` names a standard library instead).
    QString userLibraryFilename(const QString &displayName) const;

    // Appends one new macro to a user library's ".fcl" file (creating the
    // file, with a "[FIDOLIB libraryDisplayName]" header, if
    // `libraryFilename` doesn't exist yet on disk) and registers it in
    // memory immediately - no reload needed for it to show up in the
    // library panel or resolve when placed. `body` is the macro's FCD lines
    // (LI/EV/...), already in the library's local (100,100)-anchored
    // coordinate space and newline-joined, matching MacroDescriptor::body.
    // Fails (returning false and, if given, an explanatory *errorMessage)
    // without writing anything if `libraryFilename` is a standard library,
    // `key` is empty/malformed, or the resulting key is already used -
    // matching the checks DialogCreateMacro itself already runs, kept here
    // too since this is a public entry point on its own.
    bool addUserMacro(const QString &libraryFilename, const QString &libraryDisplayName,
                       const QString &key, const QString &name, const QString &category,
                       const QString &body, QString *errorMessage = nullptr);

    // The following six all rewrite the affected user library's ".fcl" file
    // in full (from the mutated in-memory model) and then call
    // loadLibraries() to resync everything from disk - simpler and safer
    // than hand-patching individual lines in place, and guarantees the
    // in-memory model can never drift from what's actually on disk. Every
    // one fails (returning false and, if given, an explanatory
    // *errorMessage) without changing anything on a standard library.

    // Changes a user library's display name (the "[FIDOLIB ...]" header).
    bool renameLibrary(const QString &filename, const QString &newDisplayName,
                        QString *errorMessage = nullptr);
    // Deletes a user library's ".fcl" file entirely, along with every macro
    // it contains.
    bool deleteLibrary(const QString &filename, QString *errorMessage = nullptr);

    // Renames a category within a user library; every macro under it moves
    // to the new category name (merging into an existing one of that name,
    // if any).
    bool renameCategory(const QString &filename, const QString &oldCategory,
                         const QString &newCategory, QString *errorMessage = nullptr);
    // Deletes a category and every macro declared under it from a user
    // library.
    bool deleteCategory(const QString &filename, const QString &category,
                         QString *errorMessage = nullptr);

    // Changes one macro's display name (not its lookup key) within a user
    // library.
    bool renameMacro(const QString &key, const QString &newName, QString *errorMessage = nullptr);
    // Deletes one macro from a user library.
    bool deleteMacro(const QString &key, QString *errorMessage = nullptr);

signals:
    void librariesReloaded();

private:
    LibraryManager() = default;
    ~LibraryManager();
    LibraryManager(const LibraryManager &) = delete;
    LibraryManager &operator=(const LibraryManager &) = delete;

    void loadLibraryFile(const QString &filePath);
    Sheet *parseContext();
    // Non-const pointer into m_libraries for the given filename, or nullptr -
    // shared by every mutator above.
    MacroLibrary *findLibrary(const QString &filename);
    // Overwrites the library's ".fcl" file from scratch based on `library`'s
    // current categoryOrder/macrosByCategory - the exact inverse of
    // loadLibraryFile()'s own parsing.
    bool writeLibraryFile(const MacroLibrary &library, QString *errorMessage) const;

    QList<MacroLibrary> m_libraries;
    QHash<QString, MacroDescriptor> m_macrosByKey;
    QHash<QString, QList<GraphicsPrimitive *>> m_expandedBodies;
    QHash<QString, QPixmap> m_iconCache;
    // Supplies the default connection diameter FidoCadReader::parse() needs
    // for a bare "SA" line - never shown/added to a view, just a data holder.
    Sheet *m_parseContext = nullptr;
};

#endif // LIBRARYMANAGER_H
