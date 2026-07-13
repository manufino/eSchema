/*
 * MacroPreviewWidget.cpp
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

#include "MacroPreviewWidget.h"
#include "GraphicsPrimitive.h"
#include "LibraryManager.h"

#include <QPainter>
#include <QPaintEvent>

MacroPreviewWidget::MacroPreviewWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Sunken);
}

QSize MacroPreviewWidget::sizeHint() const
{
    return QSize(160, 160);
}

void MacroPreviewWidget::setMacroKey(const QString &key)
{
    if (m_key == key)
        return;
    m_key = key;
    update();
}

void MacroPreviewWidget::paintEvent(QPaintEvent *event)
{
    // Always a plain white sheet, independent of the app's light/dark theme -
    // matching the drawing canvas's own default background, since the
    // macros themselves are drawn in colors chosen against a white sheet.
    {
        QPainter background(this);
        background.fillRect(contentsRect(), Qt::white);
    }
    QFrame::paintEvent(event);

    if (m_key.isEmpty())
        return;

    const QList<GraphicsPrimitive *> body = LibraryManager::getInstance().expandedBody(m_key);
    if (body.isEmpty())
        return;

    QRectF bounds;
    for (GraphicsPrimitive *primitive : body)
        bounds = bounds.united(primitive->boundingRect());
    if (!bounds.isValid() || bounds.width() <= 0 || bounds.height() <= 0)
        return;

    const QRectF area = contentsRect();
    const qreal margin = qMin(area.width(), area.height()) * 0.1;
    const qreal scale = qMin((area.width() - 2 * margin) / bounds.width(),
                              (area.height() - 2 * margin) / bounds.height());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(area.center());
    painter.scale(scale, scale);
    painter.translate(-bounds.center());
    for (GraphicsPrimitive *primitive : body)
        primitive->paint(&painter, nullptr, nullptr);
}
