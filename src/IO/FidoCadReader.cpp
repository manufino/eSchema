/*
 * FidoCadReader.cpp
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

#include "FidoCadReader.h"
#include "FidoCadTokenUtils.h"
#include "Sheet.h"
#include "LayerList.h"
#include "Layer.h"
#include "PrimitiveLine.h"
#include "PrimitiveBezier.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
#include "PrimitivePolygon.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitiveConnection.h"
#include "PrimitivePcbTrack.h"
#include "PrimitivePad.h"
#include "PrimitiveMacro.h"
#include "PrimitiveText.h"
#include "PrimitiveImage.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>

namespace FidoCadReader {

namespace {

Layer *layerFromIndex(int idx)
{
    QList<Layer *> *layers = LayerList::getInstance().getList();
    idx = FidoCadTokenUtils::clampLayer(idx);
    if (layers && idx >= 0 && idx < layers->size())
        return layers->at(idx);
    return LayerList::getInstance().getMaster();
}

QStringList tokenize(const QString &line)
{
    return line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

// Builds the primitive for a geometry line (LI/BE/RV/RP/EV/EP/PV/PP/CV/CP/
// SA/PL/PA/MC/IM). Returns nullptr for an unrecognized or too-short line - the
// caller skips it, per FIDOSPECS.md 4's robustness contract.
//
// LI/BE/RV/RP/EV/EP/PV/PP/CV/CP never store their own line width - they
// stroke at whatever GraphicsPrimitive::effectiveLineWidth() resolves to at
// paint time (the owning Sheet's single document-wide default), matching the
// reference FidoCadJ editor, where line width is one shared value
// (Globals.lineWidth) read at draw time rather than a per-primitive
// property - so nothing needs setting here (unlike PL/PA, which carry an
// explicit width token of their own and are unaffected).
//
// The trailing layer token is OPTIONAL on every one of these (confirmed
// against the reference editor's own parseTokens() for each primitive, e.g.
// PrimitiveLine.java only requires nn>=5 for "LI x1 y1 x2 y2" and reads
// tokens[5] as the layer only "if(nn>5)") - macro library bodies routinely
// omit it, since a symbol's internal geometry is conventionally on layer 0
// regardless of which layer the macro instance itself ends up on. layerAt()
// already tolerates a missing token via QList::value() (empty string ->
// toInt()==0 -> layer 0), so the fix is simply not rejecting the line for
// being one token short.
GraphicsPrimitive *buildPrimitive(const QString &code, const QStringList &tokens)
{
    auto num = [&](int i) { return tokens.value(i).toDouble(); };
    auto layerAt = [&](int i) { return layerFromIndex(tokens.value(i).toInt()); };

    if (code == QStringLiteral("LI")) {
        if (tokens.size() < 5) return nullptr;
        auto *p = new PrimitiveLine();
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setControlPoint(1, QPointF(num(3), num(4)));
        p->setLayer(layerAt(5));
        return p;
    }
    if (code == QStringLiteral("BE")) {
        if (tokens.size() < 9) return nullptr;
        auto *p = new PrimitiveBezier();
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setControlPoint(1, QPointF(num(3), num(4)));
        p->setControlPoint(2, QPointF(num(5), num(6)));
        p->setControlPoint(3, QPointF(num(7), num(8)));
        p->setLayer(layerAt(9));
        return p;
    }
    if (code == QStringLiteral("RV") || code == QStringLiteral("RP")) {
        if (tokens.size() < 5) return nullptr;
        auto *p = new PrimitiveRectangle();
        p->setIsFilled(code == QStringLiteral("RP"));
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setControlPoint(1, QPointF(num(3), num(4)));
        p->setLayer(layerAt(5));
        return p;
    }
    if (code == QStringLiteral("EV") || code == QStringLiteral("EP")) {
        if (tokens.size() < 5) return nullptr;
        auto *p = new PrimitiveEllipse();
        p->setIsFilled(code == QStringLiteral("EP"));
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setControlPoint(1, QPointF(num(3), num(4)));
        p->setLayer(layerAt(5));
        return p;
    }
    if (code == QStringLiteral("PV") || code == QStringLiteral("PP")) {
        if (tokens.size() < 5) return nullptr;
        auto *p = new PrimitivePolygon();
        p->setIsFilled(code == QStringLiteral("PP"));
        // Vertices always come in (x,y) pairs, so the token count after the
        // code is even with no layer, odd with one - matching
        // PrimitivePolygon.java's own self-adjusting "while(j<nn-1)" parity.
        const bool hasLayer = (tokens.size() - 1) % 2 != 0;
        const int coordTokens = tokens.size() - 1 - (hasLayer ? 1 : 0);
        const int vertexCount = qMin(coordTokens / 2, int(PrimitivePolygon::MaxVertices));
        for (int i = 0; i < vertexCount; ++i)
            p->appendVertex(QPointF(num(1 + i * 2), num(2 + i * 2)));
        p->setLayer(hasLayer ? layerAt(1 + vertexCount * 2) : layerFromIndex(0));
        return p;
    }
    if (code == QStringLiteral("CV") || code == QStringLiteral("CP")) {
        if (tokens.size() < 6) return nullptr;
        auto *p = new PrimitiveComplexCurve();
        p->setIsFilled(code == QStringLiteral("CP"));
        p->setClosed(tokens.value(1).toInt() != 0);
        // Same parity logic as PV/PP, offset by one extra token for the
        // leading "closed" flag.
        const bool hasLayer = (tokens.size() - 2) % 2 != 0;
        const int coordTokens = tokens.size() - 2 - (hasLayer ? 1 : 0);
        const int vertexCount = qMin(coordTokens / 2, int(PrimitiveComplexCurve::MaxVertices));
        for (int i = 0; i < vertexCount; ++i)
            p->appendVertex(QPointF(num(2 + i * 2), num(3 + i * 2)));
        p->setLayer(hasLayer ? layerAt(2 + vertexCount * 2) : layerFromIndex(0));
        return p;
    }
    if (code == QStringLiteral("SA")) {
        if (tokens.size() < 3) return nullptr;
        auto *p = new PrimitiveConnection();
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setLayer(layerAt(3));
        return p;
    }
    if (code == QStringLiteral("PL")) {
        if (tokens.size() < 6) return nullptr;
        auto *p = new PrimitivePcbTrack();
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setControlPoint(1, QPointF(num(3), num(4)));
        p->setWidth(num(5));
        p->setLayer(layerAt(6));
        return p;
    }
    if (code == QStringLiteral("PA")) {
        if (tokens.size() < 7) return nullptr;
        auto *p = new PrimitivePad();
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setOuterSize(num(3), num(4));
        p->setHoleDiameter(num(5));
        p->setStyle(static_cast<PrimitivePad::Style>(qBound(0, int(num(6)), 2)));
        p->setLayer(layerAt(7));
        return p;
    }
    if (code == QStringLiteral("MC")) {
        if (tokens.size() < 6) return nullptr;
        auto *p = new PrimitiveMacro();
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setOrientation(int(num(3)));
        p->setMirrored(tokens.value(4).toInt() != 0);
        p->setMacroName(tokens.mid(5).join(QLatin1Char(' ')).toLower());
        return p;
    }
    if (code == QStringLiteral("IM")) {
        if (tokens.size() < 10) return nullptr;
        auto *p = new PrimitiveImage();
        p->setControlPoint(0, QPointF(num(1), num(2)));
        p->setControlPoint(1, QPointF(num(3), num(4)));
        p->setOpacity(num(5));
        p->setHue(int(num(6)));
        p->setLayer(layerAt(7));
        // Base64 has no spaces, but defensively rejoin in case it was ever
        // accidentally split on whitespace (FIDOSPECS.md 5.12).
        p->setImageData(tokens.at(8), tokens.mid(9).join(QString()));
        return p;
    }
    return nullptr;
}

// Applies an "FCJ ..." line to the primitive it follows (FIDOSPECS.md 6.1/6.2).
// Sets macroCounter so the main loop knows whether TY name/value lines follow.
void applyFCJ(GraphicsPrimitive *primitive, const QStringList &tokens, int &macroCounter)
{
    using namespace FidoCadTokenUtils;
    int i = 1;
    if (primitive->supportsArrows()) {
        if (tokens.size() < i + 4)
            return;
        const int arrows = tokens.at(i++).toInt();
        const int arrowStyle = tokens.at(i++).toInt();
        const qreal arrowLength = tokens.at(i++).toDouble();
        const qreal arrowHalfWidth = tokens.at(i++).toDouble();
        primitive->setArrowAtStart(arrows & 1);
        primitive->setArrowAtEnd(arrows & 2);
        primitive->setArrowStyleLimiter(arrowStyle & 1);
        primitive->setArrowStyleEmpty(arrowStyle & 2);
        primitive->setArrowLength(arrowLength);
        primitive->setArrowHalfWidth(arrowHalfWidth);
    }
    if (tokens.size() < i + 2)
        return;
    const int dash = tokens.at(i++).toInt();
    const int hasText = tokens.at(i++).toInt();
    primitive->penStyleIsChanged(dashStyleToPenStyle(dash));
    macroCounter = (hasText == 1) ? 2 : 0;
}

QString labelText(const QStringList &tokens, int firstTextTokenIndex)
{
    return tokens.size() <= firstTextTokenIndex ? QString()
                                                 : tokens.mid(firstTextTokenIndex).join(QLatin1Char(' '));
}

// Shared by read() (which replaces the whole document) and parse() (which
// hands the caller a standalone list to merge into an existing document, e.g.
// for Paste) - both walk the same line-by-line state machine, differing only
// in whether an FJC line's document-wide settings (connection diameter/line
// widths) get applied to `sheet`.
QList<GraphicsPrimitive *> parseLines(const QString &text, Sheet *sheet, bool applyDocumentConfig)
{
    QList<GraphicsPrimitive *> result;
    GraphicsPrimitive *pending = nullptr;
    int macroCounter = 0; // 2 = next TY is the name, 1 = next TY is the value

    auto commitPending = [&]() {
        if (!pending)
            return;
        if (!pending->isDegenerate())
            result.append(pending);
        else
            delete pending;
        pending = nullptr;
        macroCounter = 0;
    };

    QString normalized = text;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    for (const QString &line : normalized.split(QLatin1Char('\n'))) {
        const QStringList tokens = tokenize(line);
        if (tokens.isEmpty())
            continue;
        const QString code = tokens.at(0);

        if (code == QStringLiteral("[FIDOCAD]"))
            continue;

        if (code == QStringLiteral("FJC")) {
            // Minimal FJC support: "C"/"A"/"B" (connection diameter, line
            // width, circle line width) are stored on the Sheet so
            // FidoCadWriter can re-emit them - see Sheet::connectionDiameter()
            // et al. "K" (layer lock state) is applied straight to the
            // global LayerList, matching FidoCadWriter's own emission. "IMG"
            // (background tracing image) is an eSchema-only extension - the
            // reference FidoCadJ editor's own equivalent never persists to
            // file at all, so this is simply never present in a file it
            // wrote, and a real FidoCadJ opening one eSchema wrote just skips
            // it as an unrecognized sub-code, per the "L"/"N" (per-layer
            // color/name overrides) precedent already noted below.
            // Other sub-codes (L/N - per-layer color/name overrides) are
            // recognized-but-ignored for this pass, matching the spec's
            // "unknown sub-codes are ignored" rule. applyDocumentConfig is
            // only true for a genuine whole-document read() - never for
            // parse()/paste or macro-body expansion, so this can never
            // clobber layer/image state mid-paste.
            if (applyDocumentConfig && tokens.size() >= 3) {
                if (tokens.at(1) == QStringLiteral("K") && tokens.size() >= 4) {
                    const int layerNum = tokens.at(2).toInt();
                    layerFromIndex(layerNum)->setLocked(
                        tokens.at(3).compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
                } else if (tokens.at(1) == QStringLiteral("IMG") && tokens.size() >= 7) {
                    const QString mimeSubtype = tokens.at(2);
                    const qreal resolution = tokens.at(3).toDouble();
                    const qreal x = tokens.at(4).toDouble();
                    const qreal y = tokens.at(5).toDouble();
                    sheet->setBackgroundImage(mimeSubtype, tokens.at(6), resolution, QPointF(x, y));
                } else {
                    const qreal value = tokens.at(2).toDouble();
                    if (value > 0) {
                        if (tokens.at(1) == QStringLiteral("C"))
                            sheet->setConnectionDiameter(value);
                        else if (tokens.at(1) == QStringLiteral("A"))
                            sheet->setLineWidth(value);
                        else if (tokens.at(1) == QStringLiteral("B"))
                            sheet->setLineWidthCircles(value);
                    }
                }
            }
            continue;
        }

        if (code == QStringLiteral("FCJ")) {
            if (pending && pending->supportsFCJ())
                applyFCJ(pending, tokens, macroCounter);
            continue;
        }

        if (code == QStringLiteral("TY") || code == QStringLiteral("TE")) {
            // The attached label's own x/y (tokens 1/2) are its real,
            // possibly user-dragged position (FidoCadWriter now writes
            // exactly this, not a recomputed offset) - read it back instead
            // of always re-deriving controlPoint(0) + labelOffset(), or a
            // previously-moved label would snap back to the default on
            // every reopen.
            if (code == QStringLiteral("TY") && macroCounter == 2) {
                pending->setName(labelText(tokens, 9));
                if (tokens.size() > 2)
                    pending->setNameLabelPos(QPointF(tokens.at(1).toDouble(), tokens.at(2).toDouble()));
                macroCounter = 1;
                continue;
            }
            if (code == QStringLiteral("TY") && macroCounter == 1) {
                pending->setValue(labelText(tokens, 9));
                if (tokens.size() > 2)
                    pending->setValueLabelPos(QPointF(tokens.at(1).toDouble(), tokens.at(2).toDouble()));
                macroCounter = 0;
                commitPending();
                continue;
            }

            // Standalone text primitive - closes out whatever was pending
            // (it turned out to have no attached labels).
            commitPending();
            auto *textPrimitive = new PrimitiveText();
            if (code == QStringLiteral("TE")) {
                if (tokens.size() < 4) { delete textPrimitive; continue; }
                textPrimitive->setControlPoint(0, QPointF(tokens.value(1).toDouble(), tokens.value(2).toDouble()));
                textPrimitive->setLayer(layerFromIndex(0));
                textPrimitive->setText(labelText(tokens, 3));
            } else {
                if (tokens.size() < 9) { delete textPrimitive; continue; }
                textPrimitive->setControlPoint(0, QPointF(tokens.at(1).toDouble(), tokens.at(2).toDouble()));
                textPrimitive->setSize(tokens.at(3).toInt(), tokens.at(4).toInt());
                textPrimitive->setOrientationDeg(tokens.at(5).toInt());
                textPrimitive->setStyleFlags(tokens.at(6).toInt());
                textPrimitive->setLayer(layerFromIndex(tokens.at(7).toInt()));
                textPrimitive->setFontName(FidoCadTokenUtils::decodeFontName(tokens.at(8)));
                textPrimitive->setText(labelText(tokens, 9));
            }
            if (textPrimitive->isDegenerate())
                delete textPrimitive;
            else
                result.append(textPrimitive);
            continue;
        }

        // A new primitive line: the previous one's FCJ/TY window is closed.
        commitPending();
        GraphicsPrimitive *primitive = buildPrimitive(code, tokens);
        if (!primitive)
            continue; // unrecognized/malformed line - skip (robustness contract)

        pending = primitive;
        // SA/PL/PA/MC/IM have no FCJ line - their name/value TY lines (if any)
        // simply follow directly (FIDOSPECS.md 6.3), so optimistically expect
        // them; if the next line isn't a TY, commitPending() above discards
        // this state harmlessly when the next real line is processed.
        macroCounter = pending->supportsFCJ() ? 0 : 2;
    }

    commitPending();
    return result;
}

} // namespace

void read(const QString &text, Sheet *sheet)
{
    sheet->clearPrimitives();
    for (GraphicsPrimitive *primitive : parseLines(text, sheet, /*applyDocumentConfig=*/true))
        sheet->addPrimitive(primitive);
}

QList<GraphicsPrimitive *> parse(const QString &text, Sheet *contextSheet)
{
    return parseLines(text, contextSheet, /*applyDocumentConfig=*/false);
}

bool readFile(const QString &filePath, Sheet *sheet, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    const QString content = stream.readAll();
    // A mid-read I/O error (network share dropped, media yanked) hands back
    // partial text; the parser skips malformed lines by contract, so the
    // truncation would open "successfully" as a mutilated document - and a
    // later Save would overwrite the good file on disk with it. Fail
    // instead of loading half a drawing.
    if (stream.status() != QTextStream::Ok) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    read(content, sheet);
    return true;
}

} // namespace FidoCadReader
