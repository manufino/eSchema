/*
 * GraphicExporter.cpp
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

#include "GraphicExporter.h"
#include "EpsGenerator.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include "Layer.h"
#include "LayerList.h"
#include "DxfWriter.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSet>
#include <QSvgGenerator>
#include <QtMath>

#include <algorithm>

namespace {

// Fits `source` inside `target` preserving the aspect ratio and centering it,
// same logic as MainWindow::renderForPrint() - stretching a schematic
// unevenly would distort right angles and text.
QRectF fitCentered(const QRectF &source, const QRectF &target)
{
    const qreal scale = qMin(target.width() / source.width(),
                              target.height() / source.height());
    const QSizeF scaledSize = source.size() * scale;
    return QRectF(target.left() + (target.width() - scaledSize.width()) / 2.0,
                  target.top() + (target.height() - scaledSize.height()) / 2.0,
                  scaledSize.width(), scaledSize.height());
}

// Exports the sheet's currently-visible primitives to one file. Split-layer
// export calls this once per layer with the others hidden.
bool exportOneFile(Sheet *sheet, const QString &path,
                   const GraphicExporter::Options &options, QString *error)
{
    const QString &format = options.format;

    if (format == QLatin1String("dxf"))
        return DxfWriter::writeFile(sheet, path, error);

    const QRectF source = sheet->itemsBoundingRect();

    QSizeF targetSize;
    if (options.resolutionBased)
        targetSize = source.size() * options.resolution;
    else
        targetSize = QSizeF(options.totX, options.totY);
    targetSize = targetSize.expandedTo(QSizeF(1, 1));

    // Hard ceiling on the output size: the CLI accepts any positive
    // resolution, and resolution x a large drawing can demand a
    // multi-gigabyte QImage - qCeil() on a huge double is UB and even a
    // "successful" allocation just thrashes the machine. 32000 px per side
    // matches the GUI's own size-mode limit.
    constexpr qreal MaxSidePx = 32000.0;
    if (targetSize.width() > MaxSidePx || targetSize.height() > MaxSidePx) {
        if (error)
            *error = QStringLiteral("export size %1x%2 px exceeds the %3 px per side limit")
                    .arg(qRound(targetSize.width())).arg(qRound(targetSize.height()))
                    .arg(qRound(MaxSidePx));
        return false;
    }

    const QRectF target(QPointF(0, 0), targetSize);
    const QRectF drawTarget = fitCentered(source, target);

    if (format == QLatin1String("png") || format == QLatin1String("jpg")
            || format == QLatin1String("jpeg")) {
        // White background (not transparent): the scene itself paints none.
        QImage image(qCeil(targetSize.width()), qCeil(targetSize.height()),
                     QImage::Format_ARGB32);
        // QImage doesn't throw on allocation failure - it returns a null
        // image, and painting/saving into one would "succeed" confusingly.
        if (image.isNull()) {
            if (error)
                *error = QStringLiteral("not enough memory for a %1x%2 px image")
                        .arg(qCeil(targetSize.width())).arg(qCeil(targetSize.height()));
            return false;
        }
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, options.antialias);
        sheet->render(&painter, drawTarget, source);
        painter.end();
        // Explicit format: the extension may not match it (CLI -f).
        const bool saved = image.save(path,
                format == QLatin1String("png") ? "PNG" : "JPG");
        if (!saved && error)
            *error = QStringLiteral("unable to save the image to \"%1\"").arg(path);
        return saved;
    }

    if (format == QLatin1String("svg")) {
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setSize(targetSize.toSize());
        generator.setViewBox(target);
        generator.setTitle(QStringLiteral("eSchema drawing"));
        QPainter painter;
        if (!painter.begin(&generator)) {
            if (error)
                *error = QStringLiteral("unable to write \"%1\"").arg(path);
            return false;
        }
        sheet->render(&painter, drawTarget, source);
        // end() reports whether the paint engine could flush everything to
        // the file - ignoring it turned a full disk into a silently
        // truncated export.
        if (!painter.end()) {
            if (error)
                *error = QStringLiteral("unable to write \"%1\"").arg(path);
            return false;
        }
        return true;
    }

    if (format == QLatin1String("eps")) {
        // Same 1-pt-per-pixel convention as the PDF branch below.
        EpsGenerator generator;
        generator.setFileName(path);
        generator.setSize(targetSize.toSize());
        generator.setTitle(QStringLiteral("eSchema drawing"));
        QPainter painter;
        if (!painter.begin(&generator)) {
            if (error)
                *error = QStringLiteral("unable to write \"%1\"").arg(path);
            return false;
        }
        sheet->render(&painter, drawTarget, source);
        if (!painter.end()) { // see the SVG branch
            if (error)
                *error = QStringLiteral("unable to write \"%1\"").arg(path);
            return false;
        }
        return true;
    }

    if (format == QLatin1String("pdf")) {
        // A single page cut exactly to the requested size (1 pt per pixel at
        // 72 dpi), so both sizing modes are honored - unlike the fit-to-page
        // print path.
        QPdfWriter writer(path);
        writer.setResolution(72);
        writer.setPageSize(QPageSize(targetSize, QPageSize::Point, QString(),
                                     QPageSize::ExactMatch));
        writer.setPageMargins(QMarginsF(0, 0, 0, 0));
        QPainter painter;
        if (!painter.begin(&writer)) {
            if (error)
                *error = QStringLiteral("unable to write \"%1\"").arg(path);
            return false;
        }
        painter.setRenderHint(QPainter::Antialiasing);
        sheet->render(&painter, drawTarget, source);
        if (!painter.end()) { // see the SVG branch
            if (error)
                *error = QStringLiteral("unable to write \"%1\"").arg(path);
            return false;
        }
        return true;
    }

    if (error)
        *error = QStringLiteral("unsupported format \"%1\"").arg(format);
    return false;
}

} // namespace

namespace GraphicExporter {

QString splitLayerFileName(const QString &fileName, int layer)
{
    const QFileInfo info(fileName);
    const QString suffix = info.suffix();
    QString name = info.completeBaseName() + QLatin1Char('_') + QString::number(layer);
    if (!suffix.isEmpty())
        name += QLatin1Char('.') + suffix;
    return info.dir().filePath(name);
}

bool exportSheet(Sheet *sheet, const QString &path, const Options &options,
                 QString *error)
{
    // Black & white: temporarily paint every layer black. Primitives read
    // their layer's color live at paint time (GraphicsPrimitive::drawColor(),
    // and DxfWriter's own layer-color lookup), so no per-primitive work is
    // needed - just restore the palette afterwards. Layer is a plain class
    // (no signals), so nothing else in the app notices the round trip.
    QList<Layer *> *layers = LayerList::getInstance().getList();
    QHash<Layer *, QColor> savedColors;
    if (options.blackWhite) {
        for (Layer *layer : *layers) {
            savedColors.insert(layer, layer->color());
            layer->setColor(QColor(Qt::black));
        }
    }

    bool ok = true;
    if (options.splitLayers && options.format != QLatin1String("dxf")) {
        const QList<GraphicsPrimitive *> &primitives = sheet->primitives();
        QSet<int> usedSet;
        for (GraphicsPrimitive *primitive : primitives)
            usedSet.insert(primitive->layerIndex());
        QList<int> usedLayers(usedSet.begin(), usedSet.end());
        std::sort(usedLayers.begin(), usedLayers.end());

        for (int layer : std::as_const(usedLayers)) {
            // The primitive-owned visibility flag (paint() early-returns on
            // it), not the QGraphicsItem one that layer hiding drives.
            for (GraphicsPrimitive *primitive : primitives)
                primitive->setVisible(primitive->layerIndex() == layer);
            if (!exportOneFile(sheet, splitLayerFileName(path, layer),
                               options, error)) {
                ok = false;
                break;
            }
        }
        for (GraphicsPrimitive *primitive : primitives)
            primitive->setVisible(true);
    } else {
        ok = exportOneFile(sheet, path, options, error);
    }

    if (options.blackWhite) {
        for (auto it = savedColors.constBegin(); it != savedColors.constEnd(); ++it)
            it.key()->setColor(it.value());
    }
    return ok;
}

} // namespace GraphicExporter
