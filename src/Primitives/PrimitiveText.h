/*
 * PrimitiveText.h
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

#ifndef PRIMITIVETEXT_H
#define PRIMITIVETEXT_H

#include "GraphicsPrimitive.h"
#include <QFont>

// FCD "TY" (advanced) / "TE" (legacy, always upgraded to TY on save) - a
// standalone text primitive (FIDOSPECS.md 5.11). Note: when a TY line instead
// serves as another primitive's attached name/value label (the "dual role of
// TY" in 5.11), it is NOT represented as a PrimitiveText - FidoCadReader writes
// that text straight into the owning primitive's name()/value() instead. This
// class only models genuinely standalone text.
class PrimitiveText : public GraphicsPrimitive
{
public:
    // FCJ-independent style bits, matching the FCD "style" field (5.11).
    enum StyleFlag { Bold = 1, Italic = 2, Mirrored = 4 };

    explicit PrimitiveText(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 1; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    // A text's own reading direction/orientation is a scalar field, not
    // something derived from moving its single anchor point - like
    // PrimitiveMacro, mirror/rotate must additionally update that field
    // (matches FidoCadJ's PrimitiveAdvText.rotatePrimitive/mirrorPrimitive).
    void mirror(Qt::Orientation axis, const QPointF &pivot) override;
    void rotate90(const QPointF &pivot) override;

    QString text() const { return m_text; }
    // May contain FidoCadJ's ^/_ superscript/subscript markup (rendered via
    // DecoratedText). prepareGeometryChange()/update(): editable live from
    // the Properties panel, and the content directly drives boundingRect().
    void setText(const QString &text) { prepareGeometryChange(); m_text = text; update(); }
    int sizeY() const { return m_sizeY; }
    int sizeX() const { return m_sizeX; }
    // prepareGeometryChange()/update(): can now be changed live from the
    // Properties panel on an already-visible selection, not just at parse time -
    // size/orientation/style-flags (bold/italic) all affect boundingRect().
    void setSize(int sizeY, int sizeX) { prepareGeometryChange(); m_sizeY = sizeY; m_sizeX = sizeX; update(); }
    int orientationDeg() const { return m_orientationDeg; }
    void setOrientationDeg(int deg) { prepareGeometryChange(); m_orientationDeg = deg; update(); }
    int styleFlags() const { return m_styleFlags; }
    void setStyleFlags(int flags) { prepareGeometryChange(); m_styleFlags = flags; update(); }
    QString fontName() const { return m_fontName; }
    // prepareGeometryChange()/update() matter here (unlike the other setters
    // in this class, currently only called at parse/creation time) because
    // this is the one the properties panel wires to a live selection - a
    // font change can resize the text's boundingRect().
    void setFontName(const QString &font) { prepareGeometryChange(); m_fontName = font; update(); }

    bool isDegenerate() const override { return m_text.isEmpty(); }
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }

private:
    // The QFont carrying this primitive's family/size/bold/italic, shared by
    // paint() and boundingRect() so drawing and invalidation always agree.
    QFont styledFont() const;

    QPointF m_pos;
    int m_sizeY = 4, m_sizeX = 3;
    int m_orientationDeg = 0;
    int m_styleFlags = 0;
    QString m_fontName = QStringLiteral("Courier New");
    QString m_text;
};

#endif // PRIMITIVETEXT_H
