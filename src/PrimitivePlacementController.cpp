#include "PrimitivePlacementController.h"
#include "Sheet.h"
#include "SheetView.h"
#include "ToolBarPrimitive.h"
#include "LayerComboBox.h"
#include "PrimitiveLine.h"
#include "PrimitiveBezier.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
#include "PrimitivePolygon.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitiveConnection.h"
#include "PrimitiveText.h"
#include <QAction>
#include <QCheckBox>
#include <QKeyEvent>
#include <QInputDialog>

PrimitivePlacementController::PrimitivePlacementController(SheetView *view, Sheet *sheet,
                                                             ToolBarPrimitive *toolBar,
                                                             LayerComboBox *layerCombo,
                                                             QCheckBox *fillCheckBox,
                                                             QObject *parent)
    : QObject(parent), m_view(view), m_sheet(sheet), m_toolBar(toolBar),
      m_layerCombo(layerCombo), m_fillCheckBox(fillCheckBox)
{
}

PrimitivePlacementController::Tool PrimitivePlacementController::currentTool() const
{
    QAction *action = m_toolBar ? m_toolBar->GetCurrent() : nullptr;
    if (!action)
        return Tool::Select;

    const QString name = action->objectName();
    if (name == QStringLiteral("actionLine")) return Tool::Line;
    if (name == QStringLiteral("actionRectangle")) return Tool::Rectangle;
    if (name == QStringLiteral("actionPolygon")) return Tool::Polygon;
    if (name == QStringLiteral("actionEllipse")) return Tool::Ellipse;
    if (name == QStringLiteral("actionBezier")) return Tool::Bezier;
    if (name == QStringLiteral("actionCurve")) return Tool::Curve;
    if (name == QStringLiteral("actionText")) return Tool::Text;
    if (name == QStringLiteral("actionConnection")) return Tool::Connection;
    return Tool::Select;
}

bool PrimitivePlacementController::isPlacementToolActive() const
{
    return m_activePrimitive != nullptr || currentTool() != Tool::Select;
}

int PrimitivePlacementController::requiredPointCount(Tool tool) const
{
    switch (tool) {
    case Tool::Line: return 2;
    case Tool::Rectangle: return 2;
    case Tool::Ellipse: return 2;
    case Tool::Bezier: return 4;
    case Tool::Connection: return 1;
    case Tool::Text: return 1;
    case Tool::Polygon: return -1;
    case Tool::Curve: return -1;
    case Tool::Select: return 0;
    }
    return 0;
}

bool PrimitivePlacementController::isVariableVertexTool(Tool tool) const
{
    return tool == Tool::Polygon || tool == Tool::Curve;
}

void PrimitivePlacementController::startPlacement(const QPointF &scenePos)
{
    m_activeTool = currentTool();
    if (m_activeTool == Tool::Select)
        return;

    switch (m_activeTool) {
    case Tool::Line: m_activePrimitive = new PrimitiveLine(); break;
    case Tool::Rectangle: m_activePrimitive = new PrimitiveRectangle(); break;
    case Tool::Ellipse: m_activePrimitive = new PrimitiveEllipse(); break;
    case Tool::Bezier: m_activePrimitive = new PrimitiveBezier(); break;
    case Tool::Connection: m_activePrimitive = new PrimitiveConnection(); break;
    case Tool::Polygon: m_activePrimitive = new PrimitivePolygon(); break;
    case Tool::Curve: m_activePrimitive = new PrimitiveComplexCurve(); break;
    case Tool::Text: {
        const QString text = QInputDialog::getText(m_view, tr("Testo"), tr("Contenuto:"));
        if (text.isEmpty()) {
            m_activeTool = Tool::Select;
            return; // user cancelled - nothing to place
        }
        auto *primitiveText = new PrimitiveText();
        primitiveText->setText(text);
        m_activePrimitive = primitiveText;
        break;
    }
    case Tool::Select: return;
    }

    if (m_layerCombo && m_layerCombo->selectedLayer())
        m_activePrimitive->setLayer(m_layerCombo->selectedLayer());
    if (m_fillCheckBox)
        m_activePrimitive->setIsFilled(m_fillCheckBox->isChecked());

    m_pointsPlaced = 1;

    if (isVariableVertexTool(m_activeTool)) {
        // vertex 0 is fixed at the click; vertex 1 is the "live" preview vertex
        // that mouse-move updates until the next click.
        if (m_activeTool == Tool::Polygon) {
            auto *polygon = static_cast<PrimitivePolygon *>(m_activePrimitive);
            polygon->appendVertex(scenePos);
            polygon->appendVertex(scenePos);
        } else {
            auto *curve = static_cast<PrimitiveComplexCurve *>(m_activePrimitive);
            curve->appendVertex(scenePos);
            curve->appendVertex(scenePos);
        }
    } else {
        const int count = m_activePrimitive->controlPointCount();
        for (int i = 0; i < count; ++i)
            m_activePrimitive->setControlPoint(i, scenePos);
    }

    m_sheet->addPrimitive(m_activePrimitive);

    // Single-point tools (Connection/Text) have nothing left to drag - finish now.
    if (requiredPointCount(m_activeTool) == 1)
        finishPlacement();
}

void PrimitivePlacementController::finishPlacement()
{
    // Variable-vertex tools end with one extra "live" preview vertex that was
    // never actually clicked - drop it before handing the primitive over.
    if (m_activePrimitive && isVariableVertexTool(m_activeTool)) {
        if (m_activeTool == Tool::Polygon)
            static_cast<PrimitivePolygon *>(m_activePrimitive)->removeLastVertex();
        else
            static_cast<PrimitiveComplexCurve *>(m_activePrimitive)->removeLastVertex();
    }
    m_activePrimitive = nullptr;
    m_pointsPlaced = 0;
    m_activeTool = Tool::Select;
}

void PrimitivePlacementController::cancelPlacement()
{
    if (m_activePrimitive)
        m_sheet->removePrimitive(m_activePrimitive);
    m_activePrimitive = nullptr;
    m_pointsPlaced = 0;
    m_activeTool = Tool::Select;
}

bool PrimitivePlacementController::handleMousePress(const QPointF &scenePos)
{
    if (!m_activePrimitive) {
        startPlacement(scenePos);
        return m_activePrimitive != nullptr;
    }

    if (isVariableVertexTool(m_activeTool)) {
        // Fix the current live vertex, then start a new live vertex for the
        // next segment.
        if (m_activeTool == Tool::Polygon) {
            auto *polygon = static_cast<PrimitivePolygon *>(m_activePrimitive);
            polygon->setControlPoint(polygon->vertexCount() - 1, scenePos);
            polygon->appendVertex(scenePos);
        } else {
            auto *curve = static_cast<PrimitiveComplexCurve *>(m_activePrimitive);
            curve->setControlPoint(curve->vertexCount() - 1, scenePos);
            curve->appendVertex(scenePos);
        }
        return true;
    }

    m_activePrimitive->setControlPoint(m_pointsPlaced, scenePos);
    ++m_pointsPlaced;
    if (m_pointsPlaced >= requiredPointCount(m_activeTool))
        finishPlacement();
    return true;
}

bool PrimitivePlacementController::handleMouseMove(const QPointF &scenePos)
{
    if (!m_activePrimitive)
        return false;

    if (isVariableVertexTool(m_activeTool)) {
        if (m_activeTool == Tool::Polygon) {
            auto *polygon = static_cast<PrimitivePolygon *>(m_activePrimitive);
            polygon->setControlPoint(polygon->vertexCount() - 1, scenePos);
        } else {
            auto *curve = static_cast<PrimitiveComplexCurve *>(m_activePrimitive);
            curve->setControlPoint(curve->vertexCount() - 1, scenePos);
        }
        return true;
    }

    // Preview every not-yet-fixed control point at the current mouse position.
    const int count = m_activePrimitive->controlPointCount();
    for (int i = m_pointsPlaced; i < count; ++i)
        m_activePrimitive->setControlPoint(i, scenePos);
    return true;
}

bool PrimitivePlacementController::handleMouseDoubleClick(const QPointF &scenePos)
{
    if (!m_activePrimitive || !isVariableVertexTool(m_activeTool))
        return false;

    Q_UNUSED(scenePos);
    finishPlacement();
    return true;
}

bool PrimitivePlacementController::handleKeyPress(QKeyEvent *event)
{
    if (!m_activePrimitive)
        return false;

    if (event->key() == Qt::Key_Escape) {
        cancelPlacement();
        return true;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isVariableVertexTool(m_activeTool)) {
            finishPlacement();
            return true;
        }
    }
    return false;
}
