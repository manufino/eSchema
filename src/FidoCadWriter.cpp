#include "FidoCadWriter.h"
#include "FidoCadTokenUtils.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include <QFile>
#include <QTextStream>

namespace FidoCadWriter {

namespace {

QString buildLabelLine(const GraphicsPrimitive *primitive, const QString &text, int labelIndex)
{
    using namespace FidoCadTokenUtils;
    const QPointF pos = primitive->controlPoint(0) + primitive->labelOffset(labelIndex);
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
}

} // namespace

QString write(const Sheet *sheet)
{
    QStringList lines;
    lines << QStringLiteral("[FIDOCAD]");
    writeDocumentConfig(lines, sheet);

    for (GraphicsPrimitive *primitive : sheet->primitives())
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
    stream << write(sheet);
    return true;
}

} // namespace FidoCadWriter
