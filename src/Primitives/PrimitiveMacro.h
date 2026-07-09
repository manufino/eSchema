/*
 * PrimitiveMacro.h
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

#ifndef PRIMITIVEMACRO_H
#define PRIMITIVEMACRO_H

#include "GraphicsPrimitive.h"
#include <QTransform>
#include <QList>

class Sheet;

// FCD "MC" - an instance of a library macro (FIDOSPECS.md 5.10). The macro
// key, position, orientation and mirror flag are the only state stored here;
// the actual body is looked up from LibraryManager and expanded/placed at
// paint time (see placementTransform()), or unrecognized keys fall back to a
// dashed placeholder box labeled with the key.
class PrimitiveMacro : public GraphicsPrimitive
{
public:
    explicit PrimitiveMacro(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 1; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    // A macro's "rotation"/"mirror" are the discrete orientation/mirror fields
    // FidoCadJ stores, not a geometric transform of its single reference point.
    void mirror(Qt::Orientation axis, const QPointF &pivot) override;
    void rotate90(const QPointF &pivot) override;

    QString macroName() const { return m_macroName; }
    void setMacroName(const QString &name) { m_macroName = name; }
    int orientation() const { return m_orientation; }
    void setOrientation(int orientation) { m_orientation = orientation % 4; }
    bool isMirrored() const { return m_mirrored; }
    void setMirrored(bool mirrored) { m_mirrored = mirrored; }

    bool isDegenerate() const override { return false; } // a macro instance is always meaningful
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }
    // Matches FIDOSPECS.md's worked example (11), where a macro's name/value
    // labels sit further from the anchor than other primitives'.
    QPointF labelOffset(int labelIndex) const override { return QPointF(10, labelIndex == 0 ? 5 : 10); }

    // Expands this one macro instance into standalone, independent
    // primitives in final scene coordinates - "Converti macro in primitive"
    // (Edit menu). The result is a fresh, exclusively-owned copy (re-parsed
    // from the library body text, not the same objects LibraryManager
    // caches for painting), not yet added to any Sheet - the caller decides
    // how, typically one CreatePrimitiveCommand per primitive. If the body
    // itself contains a nested "MC" line, it becomes another (still
    // unexpanded) PrimitiveMacro at the correct final position/orientation -
    // run this again on it to flatten further. name()/value(), when set, are
    // preserved as a standalone text primitive each (matching where
    // paintLabels() would have drawn them) rather than silently discarded.
    // Returns an empty list for an unrecognized macro key.
    QList<GraphicsPrimitive *> convertToPrimitives(Sheet *contextSheet) const;

private:
    // Builds the QTransform that places the macro's local (100,100)-centered
    // body at its final position/orientation/mirror - shared by paint() and
    // boundingRect() so both agree. Matches the reference FidoCadJ editor's
    // MapCoordinates.mapXr()/mapYr() when isMacro is set: local point p maps
    // to translate(m_pos) * [mirror: scale(-1,1)] * rotate(90*orientation) *
    // translate(-100,-100) * p (mirror is a horizontal flip applied *after*
    // rotation, not before - Java's mapXr() mirror-cases only negate the
    // already-rotated X, mapYr() ignores the mirror flag entirely).
    QTransform placementTransform() const;

    QPointF m_pos;
    int m_orientation = 0; // 0-3, each step = 90 degrees
    bool m_mirrored = false;
    QString m_macroName;
};

#endif // PRIMITIVEMACRO_H
