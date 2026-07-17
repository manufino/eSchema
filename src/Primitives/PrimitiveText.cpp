/*
 * PrimitiveText.cpp
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

#include "PrimitiveText.h"
#include "DecoratedText.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QFontMetricsF>
#include <QTransform>

namespace {

// FidoCadJ sizes text by its x-size: the font's em height, in drawing
// units, is sizex*12/7 + 0.5 (PrimitiveAdvText.draw() in the reference
// editor). The font itself is built at a fixed large pixel size and the
// painter scaled down to the target: QFont pixel sizes are integers, so
// sizing the font directly at ~5 units would round away up to 10% of the
// em, and point sizes are out - they resolve against each output device's
// DPI (screen, printer, SVG generator), moving text between devices.
constexpr int BaseFontPixels = 84;

// FidoCadJ falls back to 10/7 when either size is zero.
inline int effectiveSizeX(int sizeX, int sizeY) { return (sizeX == 0 || sizeY == 0) ? 7 : sizeX; }
inline int effectiveSizeY(int sizeX, int sizeY) { return (sizeX == 0 || sizeY == 0) ? 10 : sizeY; }

// Painter scale from the base font down to the target em.
qreal renderScaleFor(int sizeX, int sizeY)
{
    return (effectiveSizeX(sizeX, sizeY) * 12.0 / 7.0 + 0.5) / BaseFontPixels;
}

// The reference editor's vertical stretch: PrimitiveAdvText.draw() tests
// "siy / six != 10 / 7", whose right side is an *integer* division (== 1),
// so any text whose two sizes differ is additionally scaled vertically by
// siy/six * 22/40 - which in practice normalizes the glyph height to
// roughly sizey drawing units. The default 4x3 text is therefore drawn
// squashed to ~73% of the font's natural height, and skipping this left
// every eSchema text visibly taller/lower than in FidoCadJ.
qreal stretchFactorFor(int sizeX, int sizeY)
{
    const qreal six = effectiveSizeX(sizeX, sizeY);
    const qreal siy = effectiveSizeY(sizeX, sizeY);
    if (qFuzzyCompare(six, siy))
        return 1.0;
    return siy / six * 22.0 / 40.0;
}

}

PrimitiveText::PrimitiveText(QGraphicsItem *parent)
    : GraphicsPrimitive(Text, parent)
{
}

QFont PrimitiveText::styledFont() const
{
    QFont font(m_fontName);
    font.setPixelSize(BaseFontPixels);
    font.setBold(m_styleFlags & Bold);
    font.setItalic(m_styleFlags & Italic);
    return font;
}

QRectF PrimitiveText::boundingRect() const
{
    const QFont font = styledFont();
    const QFontMetricsF metrics(font);
    // Decorated measurement: ^/_ superscripts and subscripts change both the
    // width (shrunk chunks) and the vertical extent (glyphs shifted outside
    // the plain ascent/descent box) of the rendered text.
    const qreal width = DecoratedText::width(font, m_text);
    qreal top = 0, bottom = 0;
    DecoratedText::verticalExtent(font, m_text, top, bottom);

    // Local rect around the baseline used by paint() (anchor at the text's
    // top, baseline one ascent below it), in base-font units, then mapped
    // through paint()'s own scale/rotation/mirror so a scaled, rotated or
    // mirrored text is never clipped.
    const QRectF local(0, metrics.ascent() + top, width, bottom - top);
    const qreal scale = renderScaleFor(m_sizeX, m_sizeY);
    QTransform transform;
    const QPointF anchor = mapFromScene(m_pos);
    transform.translate(anchor.x(), anchor.y());
    transform.rotate(-m_orientationDeg);
    if (m_styleFlags & Mirrored)
        transform.scale(-1, 1);
    transform.scale(scale, scale * stretchFactorFor(m_sizeX, m_sizeY));
    return transform.mapRect(local).adjusted(-2, -2, 2, 2);
}

void PrimitiveText::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible() || m_text.isEmpty())
        return;

    const QFont font = styledFont();
    painter->setPen(drawColor());

    painter->save();
    painter->translate(mapFromScene(m_pos));
    painter->rotate(-m_orientationDeg); // FCD orientation is counter-clockwise
    if (m_styleFlags & Mirrored)
        painter->scale(-1, 1);
    // Base font scaled down to the FidoCadJ-exact em (and vertically to the
    // reference editor's stretch) - see the helpers at the top of this file.
    painter->scale(renderScaleFor(m_sizeX, m_sizeY),
                   renderScaleFor(m_sizeX, m_sizeY) * stretchFactorFor(m_sizeX, m_sizeY));
    // FIDOSPECS anchors a text primitive at the TOP of its bounding box
    // (matches PrimitiveAdvText.java, which tracks the text's extent
    // downward from its anchor by the font ascent), but text is drawn from
    // a BASELINE - without this offset every text primitive rendered one
    // font-ascent too high compared to the reference editor, most noticeable
    // on the small, tightly-packed pin/value labels inside macro bodies.
    // DecoratedText handles the FidoCadJ ^/_ superscript/subscript markup.
    DecoratedText::draw(painter, font, QPointF(0, QFontMetricsF(font).ascent()), m_text);
    painter->restore();
}

QPointF PrimitiveText::controlPoint(int) const
{
    return m_pos;
}

void PrimitiveText::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

void PrimitiveText::mirror(Qt::Orientation axis, const QPointF &pivot)
{
    GraphicsPrimitive::mirror(axis, pivot); // moves the anchor point
    m_styleFlags ^= Mirrored;
}

void PrimitiveText::rotate90(const QPointF &pivot)
{
    GraphicsPrimitive::rotate90(pivot); // moves the anchor point
    m_orientationDeg = (m_orientationDeg + 90) % 360;
}

QStringList PrimitiveText::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("TY"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        QString::number(m_sizeY), QString::number(m_sizeX),
        QString::number(m_orientationDeg), QString::number(m_styleFlags),
        QString::number(layerIndex()),
        encodeFontName(m_fontName),
        m_text
    };
}
