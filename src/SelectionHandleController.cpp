#include "SelectionHandleController.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include "PrimitiveHandleItem.h"
#include <utility>

SelectionHandleController::SelectionHandleController(Sheet *sheet, QObject *parent)
    : QObject(parent), m_sheet(sheet)
{
    connect(sheet, &QGraphicsScene::selectionChanged, this, &SelectionHandleController::onSelectionChanged);
    connect(sheet, &QGraphicsScene::changed, this, &SelectionHandleController::refreshHandlePositions);
}

void SelectionHandleController::onSelectionChanged()
{
    clearHandles();

    const QList<QGraphicsItem *> selected = m_sheet->selectedItems();
    if (selected.size() != 1)
        return;

    auto *primitive = dynamic_cast<GraphicsPrimitive *>(selected.first());
    if (!primitive)
        return;

    const int count = primitive->controlPointCount();
    for (int i = 0; i < count; ++i) {
        auto *handle = new PrimitiveHandleItem(primitive, i);
        m_sheet->addItem(handle);
        m_handles.append(handle);
    }
}

void SelectionHandleController::refreshHandlePositions()
{
    for (PrimitiveHandleItem *handle : std::as_const(m_handles))
        handle->setPos(handle->target()->controlPoint(handle->controlPointIndex()));
}

void SelectionHandleController::clearHandles()
{
    for (PrimitiveHandleItem *handle : std::as_const(m_handles)) {
        m_sheet->removeItem(handle);
        delete handle;
    }
    m_handles.clear();
}
