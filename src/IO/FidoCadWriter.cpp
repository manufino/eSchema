/*
 * FidoCadWriter.cpp
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

#include "FidoCadWriter.h"
#include "FidoCadTokenUtils.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include "LayerList.h"
#include "Layer.h"
#include <QFile>
#include <QTextStream>

namespace FidoCadWriter {

namespace {

QString buildLabelLine(const GraphicsPrimitive *primitive, const QString &text, int labelIndex)
{
    using namespace FidoCadTokenUtils;
    // The label's actual (possibly user-dragged) position, not a recomputed
    // fixed offset - matches whatever FidoCadReader will read back on the
    // next open, so a moved label round-trips instead of snapping back.
    const QPointF pos = labelIndex == 0 ? primitive->nameLabelPos() : primitive->valueLabelPos();
    QStringList tokens {
        QStringLiteral("TY"),
        roundIntelligently(pos.x()), roundIntelligently(pos.y()),
        QStringLiteral("4"), QStringLiteral("3"), QStringLiteral("0"), QStringLiteral("0"),
        QString::number(primitive->layerIndex()),
        QStringLiteral("*"),
        text
    };
    return tokens.join(QLatin1Char(' '));
}

void writePrimitive(QStringList &lines, const GraphicsPrimitive *primitive)
{
    if (primitive->isDegenerate())
        return;

    lines << primitive->toTokens().join(QLatin1Char(' '));

    const bool hasText = !primitive->name().isEmpty() || !primitive->value().isEmpty();

    if (primitive->supportsFCJ()) {
        const int dash = FidoCadTokenUtils::penStyleToDashStyle(primitive->lineStyle());
        const bool hasArrows = primitive->supportsArrows()
                && (primitive->arrowAtStart() || primitive->arrowAtEnd());
        // An FCJ line is only emitted when something about it is non-default
        // (FIDOSPECS.md 9.4) - legacy-compatible files omit it entirely.
        if (dash != 0 || hasArrows || hasText) {
            QStringList fcj { QStringLiteral("FCJ") };
            if (primitive->supportsArrows()) {
                const int arrows = (primitive->arrowAtStart() ? 1 : 0) | (primitive->arrowAtEnd() ? 2 : 0);
                const int arrowStyle = (primitive->arrowStyleLimiter() ? 1 : 0)
                        | (primitive->arrowStyleEmpty() ? 2 : 0);
                fcj << QString::number(arrows) << QString::number(arrowStyle)
                    << FidoCadTokenUtils::roundIntelligently(primitive->arrowLength())
                    << FidoCadTokenUtils::roundIntelligently(primitive->arrowHalfWidth());
            }
            fcj << QString::number(dash) << (hasText ? QStringLiteral("1") : QStringLiteral("0"));
            lines << fcj.join(QLatin1Char(' '));
        }
    }

    if (hasText) {
        if (!primitive->name().isEmpty())
            lines << buildLabelLine(primitive, primitive->name(), 0);
        if (!primitive->value().isEmpty())
            lines << buildLabelLine(primitive, primitive->value(), 1);
    }
}

// Emits "FJC C/A/B" only for values that differ from the spec's compiled-in
// defaults (FIDOSPECS.md 9.2), in the same order the spec lists them in - so a
// file that never touched these settings gets no FJC config lines at all.
void writeDocumentConfig(QStringList &lines, const Sheet *sheet)
{
    using namespace FidoCadTokenUtils;
    if (!qFuzzyCompare(sheet->connectionDiameter(), 2.0))
        lines << QStringLiteral("FJC C ") + roundIntelligently(sheet->connectionDiameter());
    if (!qFuzzyCompare(sheet->lineWidth(), 0.5))
        lines << QStringLiteral("FJC A ") + roundIntelligently(sheet->lineWidth());
    if (!qFuzzyCompare(sheet->lineWidthCircles(), 0.35))
        lines << QStringLiteral("FJC B ") + roundIntelligently(sheet->lineWidthCircles());

    // Layer lock state: matches the reference FidoCadJ editor's own
    // persistence (ParserActions.checkAndRegisterLayers()'s "FJC K i true"),
    // only emitted for a layer actually locked - false is simply the
    // default, absent state, mirroring FidoCadJ never writing "FJC K i
    // false".
    const QList<Layer *> *layers = LayerList::getInstance().getList();
    for (int i = 0; layers && i < layers->size(); ++i) {
        if (layers->at(i)->isLocked())
            lines << QStringLiteral("FJC K %1 true").arg(i);
    }

    // Background tracing image - an eSchema-only extension (see
    // FidoCadReader.cpp's own comment on the "IMG" sub-code for why this is
    // safe to write even though the reference FidoCadJ editor never emits
    // or reads it). Embedded as base64, matching PrimitiveImage's own "IM"
    // line convention, rather than a file path, so the reference stays
    // valid even if the original image file is later moved or deleted.
    if (sheet->hasBackgroundImage()) {
        lines << QStringLiteral("FJC IMG %1 %2 %3 %4 %5")
                     .arg(sheet->backgroundImageMimeSubtype())
                     .arg(roundIntelligently(sheet->backgroundImageResolution()))
                     .arg(roundIntelligently(sheet->backgroundImageCorner().x()))
                     .arg(roundIntelligently(sheet->backgroundImageCorner().y()))
                     .arg(sheet->backgroundImageBase64());
    }
}

} // namespace

QString write(const Sheet *sheet)
{
    return writeExpanded(sheet, sheet->primitives());
}

QString writeExpanded(const Sheet *sheet, const QList<GraphicsPrimitive *> &primitives)
{
    QStringList lines;
    lines << QStringLiteral("[FIDOCAD]");
    writeDocumentConfig(lines, sheet);

    for (GraphicsPrimitive *primitive : primitives)
        writePrimitive(lines, primitive);

    return lines.join(QLatin1Char('\n'));
}

QString writeSelection(const QList<GraphicsPrimitive *> &primitives)
{
    QStringList lines;
    for (const GraphicsPrimitive *primitive : primitives)
        writePrimitive(lines, primitive);
    return lines.join(QLatin1Char('\n'));
}

bool writeFile(const Sheet *sheet, const QString &filePath, QString *errorMessage)
{
    return writeExpandedFile(sheet, sheet->primitives(), filePath, errorMessage);
}

bool writeExpandedFile(const Sheet *sheet, const QList<GraphicsPrimitive *> &primitives,
                        const QString &filePath, QString *errorMessage)
{
    // No QIODevice::Text: that flag makes Qt translate '\n' to the platform
    // newline (CRLF on Windows) on write. FIDOSPECS.md 2.1 accepts either
    // ending, but writing plain LF avoids rewriting every line ending of an
    // LF-authored file on its very first save under this app.
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream << writeExpanded(sheet, primitives);
    return true;
}

} // namespace FidoCadWriter
