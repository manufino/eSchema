/*
 * DecoratedText.h
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

#ifndef DECORATEDTEXT_H
#define DECORATEDTEXT_H

#include <QFont>
#include <QPointF>
#include <QString>

class QPainter;

// FidoCadJ-compatible "decorated" text (DecoratedText.java): inside any text
// string, "^" makes what follows a superscript and "_" a subscript, until the
// next "^"/"_" changes the level again ("R^2", "V_out", "x^2^3_-3_-4" nest).
// "\^" and "\_" produce the literal characters. Each nesting level shrinks
// the font (x0.8/0.7/0.6/0.5) and shifts the baseline by half the shrunk
// font size per level, exactly like the reference editor, so a drawing
// written there renders identically here.
//
// All positions are expressed relative to the BASELINE of the un-shifted
// (level 0) text, matching what QPainter::drawText(QPointF) expects.
class DecoratedText
{
public:
    DecoratedText() = delete;

    // Total advance width of the decorated string rendered with baseFont.
    static qreal width(const QFont &baseFont, const QString &str);

    // How far the decorated string's glyphs stray above (top, negative) and
    // below (bottom, positive) the level-0 baseline - a plain string yields
    // [-ascent, +descent], nested superscripts push top further up, nested
    // subscripts push bottom further down. Mirrors FidoCadJ's
    // getDecoratedVerticalExtent() so hit/bounding boxes cover every glyph.
    static void verticalExtent(const QFont &baseFont, const QString &str,
                               qreal &top, qreal &bottom);

    // Draw the decorated string with its level-0 baseline starting at
    // baselinePos. The painter's font is modified (per-chunk sizes) and NOT
    // restored - callers already save()/restore() or re-set their font.
    static void draw(QPainter *painter, const QFont &baseFont,
                     const QPointF &baselinePos, const QString &str);
};

#endif // DECORATEDTEXT_H
