/*
 * PrimitiveMacro.cpp
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

#include "PrimitiveMacro.h"
#include "FidoCadTokenUtils.h"
#include "LibraryManager.h"
#include "FidoCadReader.h"
#include "PrimitiveText.h"
#include <QStyleOptionGraphicsItem>

namespace {
// Drawn instead of the real macro body when the key isn't found in any
// loaded library (e.g. a custom part not yet supported) - just big enough to
// show the key text.
constexpr qreal PlaceholderSize = 40.0;
}

PrimitiveMacro::PrimitiveMacro(QGraphicsItem *parent)
    : GraphicsPrimitive(PartLib, parent)
{
}

QTransform PrimitiveMacro::placementTransform() const
{
    const QPointF anchor = mapFromScene(m_pos);
    QTransform transform;
    transform.translate(anchor.x(), anchor.y());
    if (m_mirrored)
        transform.scale(-1, 1);
    transform.rotate(90.0 * m_orientation);
    transform.translate(-100.0, -100.0);
    return transform;
}

QRectF PrimitiveMacro::boundingRect() const
{
    const QList<GraphicsPrimitive *> body = LibraryManager::getInstance().expandedBody(m_macroName);
    if (body.isEmpty()) {
        const qreal half = PlaceholderSize / 2;
        return QRectF(mapFromScene(m_pos) - QPointF(half, half), QSizeF(PlaceholderSize, PlaceholderSize))
                .united(labelBoundingRect());
    }

    const QTransform transform = placementTransform();
    QRectF bounds;
    for (GraphicsPrimitive *primitive : body)
        bounds = bounds.united(transform.mapRect(primitive->boundingRect()));
    return bounds.united(labelBoundingRect());
}

void PrimitiveMacro::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (!isVisible())
        return;

    const QList<GraphicsPrimitive *> body = LibraryManager::getInstance().expandedBody(m_macroName);
    if (body.isEmpty()) {
        QPen pen(drawColor());
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        const QPointF center = mapFromScene(m_pos);
        const qreal half = PlaceholderSize / 2;
        const QRectF box(center - QPointF(half, half), QSizeF(PlaceholderSize, PlaceholderSize));
        painter->drawRect(box);
        painter->drawText(box, Qt::AlignCenter | Qt::TextWordWrap, m_macroName);
    } else {
        // The body primitives are never added to a Sheet, so their own
        // mapFromScene() is the identity - painting them under this extra
        // transform is what actually places/orients/mirrors them.
        painter->save();
        painter->setTransform(placementTransform(), true);
        for (GraphicsPrimitive *primitive : body)
            primitive->paint(painter, option, widget);
        painter->restore();
    }

    paintLabels(painter);
}

QPointF PrimitiveMacro::controlPoint(int) const
{
    return m_pos;
}

void PrimitiveMacro::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

void PrimitiveMacro::mirror(Qt::Orientation, const QPointF &)
{
    prepareGeometryChange();
    m_mirrored = !m_mirrored;
}

void PrimitiveMacro::rotate90(const QPointF &)
{
    prepareGeometryChange();
    m_orientation = (m_orientation + 1) % 4;
}

QList<GraphicsPrimitive *> PrimitiveMacro::convertToPrimitives(Sheet *contextSheet) const
{
    const MacroDescriptor *descriptor = LibraryManager::getInstance().macro(m_macroName);
    if (!descriptor)
        return {};

    // A fresh, independent parse (not LibraryManager::expandedBody()'s
    // shared/cached prototypes - those must never be mutated or handed to a
    // Sheet, see its own doc comment) so the result is safe to own and edit
    // from here on.
    QList<GraphicsPrimitive *> result = FidoCadReader::parse(descriptor->body, contextSheet);

    const QTransform transform = placementTransform();
    for (GraphicsPrimitive *primitive : result) {
        const int count = primitive->controlPointCount();
        for (int i = 0; i < count; ++i)
            primitive->setControlPoint(i, transform.map(primitive->controlPoint(i)));
    }

    if (!name().isEmpty()) {
        auto *nameText = new PrimitiveText();
        nameText->setLayer(objLayer);
        nameText->setControlPoint(0, m_pos + labelOffset(0));
        nameText->setText(name());
        result.append(nameText);
    }
    if (!value().isEmpty()) {
        auto *valueText = new PrimitiveText();
        valueText->setLayer(objLayer);
        valueText->setControlPoint(0, m_pos + labelOffset(1));
        valueText->setText(value());
        result.append(valueText);
    }

    return result;
}

QStringList PrimitiveMacro::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("MC"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        QString::number(m_orientation), m_mirrored ? QStringLiteral("1") : QStringLiteral("0"),
        m_macroName
    };
}
