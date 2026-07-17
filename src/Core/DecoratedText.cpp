/*
 * DecoratedText.cpp
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

#include "DecoratedText.h"
#include <QFontMetricsF>
#include <QPainter>

namespace {

// Tokenizer for the ^/_/\ markup, a direct port of DecoratedText.getToken()
// from the reference FidoCadJ editor so both accept the exact same strings:
// "^" and "_" are level changes, "\" makes the next character literal, and
// everything else accumulates into a chunk until the next special character.
enum class Token { Chunk, Index, Exponent, End };

class Tokenizer
{
public:
    explicit Tokenizer(const QString &str) : m_str(str) {}

    Token next()
    {
        if (m_pos >= m_str.size())
            return Token::End;

        QChar c = m_str.at(m_pos);
        if (c == QLatin1Char('\\')) {
            ++m_pos;
            if (m_pos >= m_str.size())
                return Token::End; // lone trailing backslash: nothing to show
            c = m_str.at(m_pos);
        } else if (c == QLatin1Char('_')) {
            ++m_pos;
            return Token::Index;
        } else if (c == QLatin1Char('^')) {
            ++m_pos;
            return Token::Exponent;
        }

        m_chunk.clear();
        while (true) {
            m_chunk.append(c);
            ++m_pos;
            if (m_pos >= m_str.size())
                break;
            c = m_str.at(m_pos);
            if (c == QLatin1Char('_') || c == QLatin1Char('^') || c == QLatin1Char('\\'))
                break;
        }
        return Token::Chunk;
    }

    QString chunk() const { return m_chunk; }

private:
    const QString &m_str;
    int m_pos = 0;
    QString m_chunk;
};

// Font shrink factor per |nesting level| (DecoratedText.getSizeMultLevel()).
qreal sizeMultForLevel(int level)
{
    switch (qAbs(level)) {
    case 0: return 1.0;
    case 1: return 0.8;
    case 2: return 0.7;
    case 3: return 0.6;
    default: return 0.5;
    }
}

// The base font's nominal size, whether it was set in points (labels/UI
// text) or in pixels (PrimitiveText's FidoCadJ-compatible sizing, where
// pointSizeF() reports -1).
qreal nominalSize(const QFont &font)
{
    return font.pointSizeF() > 0 ? font.pointSizeF() : font.pixelSize();
}

// A copy of `base` resized to `mult` times its nominal size, in whichever
// unit the base font itself uses.
QFont derivedFont(const QFont &base, qreal mult)
{
    QFont font(base);
    if (base.pointSizeF() > 0)
        font.setPointSizeF(base.pointSizeF() * mult);
    else
        font.setPixelSize(qMax(1, qRound(base.pixelSize() * mult)));
    return font;
}

} // namespace

qreal DecoratedText::width(const QFont &baseFont, const QString &str)
{
    Tokenizer tokenizer(str);
    qreal totalWidth = 0;
    int level = 0;

    for (Token t = tokenizer.next(); t != Token::End; t = tokenizer.next()) {
        if (t == Token::Exponent) {
            ++level;
        } else if (t == Token::Index) {
            --level;
        } else {
            const QFont chunkFont = derivedFont(baseFont, sizeMultForLevel(level));
            totalWidth += QFontMetricsF(chunkFont).horizontalAdvance(tokenizer.chunk());
        }
    }
    return totalWidth;
}

void DecoratedText::verticalExtent(const QFont &baseFont, const QString &str,
                                   qreal &top, qreal &bottom)
{
    const qreal baseSize = nominalSize(baseFont);
    const QFontMetricsF baseMetrics(baseFont);
    const qreal baseAscent = baseMetrics.ascent();
    const qreal baseDescent = baseMetrics.descent();
    Tokenizer tokenizer(str);
    int level = 0;
    top = 0;
    bottom = 0;

    for (Token t = tokenizer.next(); t != Token::End; t = tokenizer.next()) {
        if (t == Token::Exponent) {
            ++level;
        } else if (t == Token::Index) {
            --level;
        } else {
            // Same shift formula as draw(): the chunk's baseline sits at
            // "y - shift", so its extremes relative to the level-0 baseline
            // are offset by -shift.
            const qreal mult = sizeMultForLevel(level);
            const qreal shift = level * baseSize * mult * 0.5;
            top = qMin(top, -shift - baseAscent * mult);
            bottom = qMax(bottom, -shift + baseDescent * mult);
        }
    }
}

void DecoratedText::draw(QPainter *painter, const QFont &baseFont,
                         const QPointF &baselinePos, const QString &str)
{
    const qreal baseSize = nominalSize(baseFont);
    Tokenizer tokenizer(str);
    qreal x = baselinePos.x();
    int level = 0;

    for (Token t = tokenizer.next(); t != Token::End; t = tokenizer.next()) {
        if (t == Token::Exponent) {
            ++level;
        } else if (t == Token::Index) {
            --level;
        } else {
            const qreal mult = sizeMultForLevel(level);
            const QFont chunkFont = derivedFont(baseFont, mult);
            painter->setFont(chunkFont);
            // Positive level = superscript = raised above the baseline.
            const qreal shift = level * baseSize * mult * 0.5;
            painter->drawText(QPointF(x, baselinePos.y() - shift), tokenizer.chunk());
            x += QFontMetricsF(chunkFont).horizontalAdvance(tokenizer.chunk());
        }
    }
}
