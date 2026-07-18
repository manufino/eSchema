/*
 * DrawingThumbnails.cpp
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

#include "DrawingThumbnails.h"
#include "FidoCadReader.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QPainter>
#include <QTextStream>

namespace {

struct CacheEntry {
    QDateTime lastModified;
    QPixmap pixmap;
};

// Keyed by "path|WxH" so the same file can be cached at several sizes
// (menu preview vs welcome-card row).
QHash<QString, CacheEntry> &cache()
{
    static QHash<QString, CacheEntry> instance;
    return instance;
}

// One hidden Sheet reused for every rendering: primitives are parsed into
// it, rendered, and removed again right away (removePrimitive(), NOT
// clearPrimitives() - the latter also unlocks every global layer, a side
// effect a preview must not have).
Sheet *renderSheet()
{
    static Sheet *instance = new Sheet();
    return instance;
}

} // namespace

namespace DrawingThumbnails {

QPixmap thumbnail(const QString &path, const QSize &size)
{
    const QFileInfo info(path);
    if (!info.exists() || !size.isValid())
        return QPixmap();

    const QString key = path + QLatin1Char('|')
            + QString::number(size.width()) + QLatin1Char('x') + QString::number(size.height());
    const auto cached = cache().constFind(key);
    if (cached != cache().constEnd() && cached->lastModified == info.lastModified())
        return cached->pixmap;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QPixmap();
    QTextStream stream(&file);
    const QString text = stream.readAll();

    Sheet *sheet = renderSheet();
    const QList<GraphicsPrimitive *> primitives = FidoCadReader::parse(text, sheet);
    for (GraphicsPrimitive *primitive : primitives)
        sheet->addPrimitive(primitive);

    QPixmap pixmap(size);
    pixmap.fill(Qt::white);
    QRectF bounds = sheet->itemsBoundingRect();
    if (!bounds.isEmpty()) {
        bounds.adjust(-2, -2, 2, 2); // a hair of margin around the drawing
        // Fit-and-center by hand: QGraphicsScene::render()'s own
        // KeepAspectRatio anchors the scaled source at the target's
        // top-left rather than centering it.
        const qreal scale = qMin(size.width() / bounds.width(),
                                 size.height() / bounds.height());
        const QSizeF fitted = bounds.size() * scale;
        const QRectF target((size.width() - fitted.width()) / 2.0,
                            (size.height() - fitted.height()) / 2.0,
                            fitted.width(), fitted.height());
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        sheet->render(&painter, target, bounds, Qt::IgnoreAspectRatio);
    }

    // Tear the sheet back down for the next call (see renderSheet()).
    while (!sheet->primitives().isEmpty())
        sheet->removePrimitive(sheet->primitives().first());

    cache().insert(key, { info.lastModified(), pixmap });
    return pixmap;
}

} // namespace DrawingThumbnails
