/*
 * LibraryManager.cpp
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

#include "LibraryManager.h"
#include "Sheet.h"
#include "FidoCadReader.h"
#include "GraphicsPrimitive.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QPainter>
#include <utility>

namespace {
// FidoCadJ's own 5 standard libraries (LibUtils.isStdLib()'s extensions
// list, plus the original unprefixed FidoCAD library itself) - reserved
// filenames a user library may never be created or written under.
bool isReservedStandardName(const QString &filename)
{
    static const QStringList reserved = {
        QStringLiteral("fcdstdlib"), QStringLiteral("pcb"), QStringLiteral("ihram"),
        QStringLiteral("elettrotecnica"), QStringLiteral("ey_libraries")
    };
    return reserved.contains(filename.trimmed().toLower());
}
}

LibraryManager &LibraryManager::getInstance()
{
    static LibraryManager instance;
    return instance;
}

LibraryManager::~LibraryManager()
{
    for (const QList<GraphicsPrimitive *> &body : std::as_const(m_expandedBodies))
        qDeleteAll(body);
    delete m_parseContext;
}

Sheet *LibraryManager::parseContext()
{
    if (!m_parseContext)
        m_parseContext = new Sheet(); // never shown/added to a view - just a data holder
    return m_parseContext;
}

void LibraryManager::loadLibraries()
{
    for (const QList<GraphicsPrimitive *> &body : std::as_const(m_expandedBodies))
        qDeleteAll(body);
    m_expandedBodies.clear();
    m_iconCache.clear();
    m_macrosByKey.clear();
    m_libraries.clear();

    const QDir libDir(QCoreApplication::applicationDirPath() + QStringLiteral("/lib"));
    const QStringList files = libDir.entryList(QStringList() << QStringLiteral("*.fcl"),
                                                QDir::Files, QDir::Name);
    for (const QString &fileName : files)
        loadLibraryFile(libDir.filePath(fileName));

    emit librariesReloaded();
}

void LibraryManager::loadLibraryFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // The library's prefix is its own file name (case-insensitively), except
    // for the original FidoCAD standard library, which is unprefixed - so its
    // macro keys have no dot (matches ParserActions.readLibraryFile()).
    QString prefix = QFileInfo(filePath).completeBaseName();
    if (prefix.compare(QStringLiteral("FCDstdlib"), Qt::CaseInsensitive) == 0)
        prefix.clear();

    MacroLibrary library;
    library.filename = prefix;

    QString categoryName;
    QString macroKey;   // lowercased "prefix.name" (or bare "name") - empty means no macro open
    QString macroName;  // display name, as declared on the "[KEY name]" line
    QStringList bodyLines;

    auto commitMacro = [&]() {
        if (macroKey.isEmpty())
            return;
        MacroDescriptor descriptor;
        descriptor.key = macroKey;
        descriptor.name = macroName.trimmed();
        descriptor.category = categoryName;
        descriptor.library = library.displayName;
        descriptor.filename = prefix;
        descriptor.body = bodyLines.join(QLatin1Char('\n'));

        if (!library.macrosByCategory.contains(categoryName))
            library.categoryOrder.append(categoryName);
        library.macrosByCategory[categoryName].append(descriptor);
        m_macrosByKey.insert(macroKey, descriptor);

        macroKey.clear();
        bodyLines.clear();
    };

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.length() <= 1)
            continue;

        if (line.at(0) == QLatin1Char('{')) {
            commitMacro();
            const int closeBrace = line.indexOf(QLatin1Char('}'));
            categoryName = (closeBrace < 0 ? line.mid(1) : line.mid(1, closeBrace - 1)).trimmed();
            continue;
        }

        if (line.at(0) == QLatin1Char('[')) {
            const int closeBracket = line.indexOf(QLatin1Char(']'));
            if (closeBracket < 0)
                continue; // malformed line - skip, matching the format's general robustness
            const QString inside = line.mid(1, closeBracket - 1);
            const int firstSpace = inside.indexOf(QLatin1Char(' '));
            const QString rawKey = (firstSpace < 0 ? inside : inside.left(firstSpace)).trimmed();
            const QString longName = (firstSpace < 0 ? QString() : inside.mid(firstSpace + 1)).trimmed();

            commitMacro();
            if (rawKey.compare(QStringLiteral("FIDOLIB"), Qt::CaseInsensitive) == 0) {
                library.displayName = longName;
                continue;
            }
            macroName = longName;
            macroKey = (prefix.isEmpty() ? rawKey : prefix + QLatin1Char('.') + rawKey).toLower();
            continue;
        }

        if (!macroKey.isEmpty())
            bodyLines.append(line);
    }
    commitMacro();

    m_libraries.append(library);
}

const MacroDescriptor *LibraryManager::macro(const QString &key) const
{
    auto it = m_macrosByKey.constFind(key.trimmed().toLower());
    return it == m_macrosByKey.constEnd() ? nullptr : &it.value();
}

QList<GraphicsPrimitive *> LibraryManager::expandedBody(const QString &key)
{
    const QString normalizedKey = key.trimmed().toLower();
    auto it = m_expandedBodies.constFind(normalizedKey);
    if (it != m_expandedBodies.constEnd())
        return it.value();

    const MacroDescriptor *descriptor = macro(normalizedKey);
    if (!descriptor)
        return {}; // unrecognized macro - PrimitiveMacro falls back to a placeholder box

    QList<GraphicsPrimitive *> parsed = FidoCadReader::parse(descriptor->body, parseContext());
    m_expandedBodies.insert(normalizedKey, parsed);
    return parsed;
}

QPixmap LibraryManager::icon(const QString &key, int size)
{
    const QString normalizedKey = key.trimmed().toLower();
    const QString cacheKey = normalizedKey + QLatin1Char('@') + QString::number(size);
    auto cached = m_iconCache.constFind(cacheKey);
    if (cached != m_iconCache.constEnd())
        return cached.value();

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    const QList<GraphicsPrimitive *> body = expandedBody(normalizedKey);
    if (!body.isEmpty()) {
        QRectF bounds;
        for (GraphicsPrimitive *primitive : body)
            bounds = bounds.united(primitive->boundingRect());

        if (bounds.isValid() && bounds.width() > 0 && bounds.height() > 0) {
            const qreal margin = size * 0.1;
            const qreal scale = qMin((size - 2 * margin) / bounds.width(),
                                     (size - 2 * margin) / bounds.height());
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.translate(size / 2.0, size / 2.0);
            painter.scale(scale, scale);
            painter.translate(-bounds.center());
            for (GraphicsPrimitive *primitive : body)
                primitive->paint(&painter, nullptr, nullptr);
        }
    }

    m_iconCache.insert(cacheKey, pixmap);
    return pixmap;
}

bool LibraryManager::isStandardLibraryFilename(const QString &filename) const
{
    // The original FidoCAD standard library is stored with an empty
    // filename/prefix once loaded (see loadLibraryFile()); a *candidate* new
    // library named e.g. "FCDstdlib" (not yet loaded, so not empty yet) is
    // caught by the reserved-names list instead.
    const QString trimmed = filename.trimmed();
    return trimmed.isEmpty() || isReservedStandardName(trimmed);
}

QStringList LibraryManager::userLibraryDisplayNames() const
{
    QStringList names;
    for (const MacroLibrary &library : m_libraries) {
        if (!isStandardLibraryFilename(library.filename))
            names.append(library.displayName.isEmpty() ? library.filename : library.displayName);
    }
    return names;
}

QString LibraryManager::userLibraryFilename(const QString &displayName) const
{
    for (const MacroLibrary &library : m_libraries) {
        if (isStandardLibraryFilename(library.filename))
            continue;
        const QString shown = library.displayName.isEmpty() ? library.filename : library.displayName;
        if (shown.compare(displayName.trimmed(), Qt::CaseSensitive) == 0)
            return library.filename;
    }
    return QString();
}

bool LibraryManager::addUserMacro(const QString &libraryFilename, const QString &libraryDisplayName,
                                   const QString &key, const QString &name, const QString &category,
                                   const QString &body, QString *errorMessage)
{
    auto fail = [errorMessage](const QString &message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };

    const QString trimmedFilename = libraryFilename.trimmed();
    if (trimmedFilename.isEmpty())
        return fail(QObject::tr("Nome libreria mancante."));
    if (isStandardLibraryFilename(trimmedFilename))
        return fail(QObject::tr("Non è possibile salvare in una libreria standard."));

    const QString trimmedKey = key.trimmed();
    if (trimmedKey.isEmpty() || trimmedKey.contains(QLatin1Char('[')) || trimmedKey.contains(QLatin1Char(']'))
            || trimmedKey.contains(QLatin1Char(' ')))
        return fail(QObject::tr("Chiave macro non valida."));

    const QString fullKey = (trimmedFilename + QLatin1Char('.') + trimmedKey).toLower();
    if (m_macrosByKey.contains(fullKey))
        return fail(QObject::tr("Esiste già una macro con questa chiave in questa libreria."));

    const QDir libDir(QCoreApplication::applicationDirPath() + QStringLiteral("/lib"));
    const QString filePath = libDir.filePath(trimmedFilename + QStringLiteral(".fcl"));
    const bool isNewFile = !QFile::exists(filePath);

    QFile file(filePath);
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return fail(QObject::tr("Impossibile scrivere il file %1").arg(filePath));

    const QString displayName = libraryDisplayName.trimmed().isEmpty() ? trimmedFilename
                                                                        : libraryDisplayName.trimmed();
    QTextStream out(&file);
    if (isNewFile)
        out << "[FIDOLIB " << displayName << "]\n\n";
    out << '{' << category << "}\n";
    out << '[' << trimmedKey << ' ' << name << "]\n";
    out << body;
    if (!body.endsWith(QLatin1Char('\n')))
        out << '\n';
    file.close();

    // Registers the new macro in memory immediately, rather than requiring
    // loadLibraries() to be called again - matches every other mutation in
    // this class (e.g. a freshly-placed macro resolves without a restart).
    MacroDescriptor descriptor;
    descriptor.key = fullKey;
    descriptor.name = name;
    descriptor.category = category;
    descriptor.filename = trimmedFilename;
    descriptor.body = body;

    MacroLibrary *library = nullptr;
    for (MacroLibrary &candidate : m_libraries) {
        if (candidate.filename.compare(trimmedFilename, Qt::CaseInsensitive) == 0) {
            library = &candidate;
            break;
        }
    }
    if (!library) {
        m_libraries.append(MacroLibrary());
        library = &m_libraries.last();
        library->filename = trimmedFilename;
        library->displayName = displayName;
    }
    descriptor.library = library->displayName;
    if (!library->macrosByCategory.contains(category))
        library->categoryOrder.append(category);
    library->macrosByCategory[category].append(descriptor);
    m_macrosByKey.insert(fullKey, descriptor);

    emit librariesReloaded();
    return true;
}
