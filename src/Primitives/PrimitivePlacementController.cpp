/*
 * PrimitivePlacementController.cpp
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
#include "PrimitivePcbTrack.h"
#include "PrimitivePad.h"
#include "PrimitiveMacro.h"
#include "PrimitiveImage.h"
#include "CreatePrimitiveCommand.h"
#include <QAction>
#include <QCheckBox>
#include <QKeyEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>

PrimitivePlacementController::PrimitivePlacementController(SheetView *view, Sheet *sheet,
                                                             ToolBarPrimitive *toolBar,
                                                             LayerComboBox *layerCombo,
                                                             QCheckBox *fillCheckBox,
                                                             QObject *parent)
    : QObject(parent), m_view(view), m_sheet(sheet), m_toolBar(toolBar),
      m_layerCombo(layerCombo), m_fillCheckBox(fillCheckBox)
{
    if (m_toolBar) {
        connect(m_toolBar, &QToolBar::actionTriggered,
                this, &PrimitivePlacementController::handleToolBarActionTriggered);
    }
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
    if (name == QStringLiteral("actionPcbTrack")) return Tool::PcbTrack;
    if (name == QStringLiteral("actionPad")) return Tool::Pad;
    if (name == QStringLiteral("actionImage")) return Tool::Image;
    return Tool::Select;
}

bool PrimitivePlacementController::isPlacementToolActive() const
{
    return m_activePrimitive != nullptr || currentTool() != Tool::Select || !m_armedMacroKey.isEmpty();
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
    case Tool::PcbTrack: return 2;
    case Tool::Pad: return 1;
    case Tool::Macro: return 1;
    case Tool::Image: return 2; // click-drag a box, like Rectangle
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

bool PrimitivePlacementController::isChainableTool(Tool tool) const
{
    return tool == Tool::Line || tool == Tool::PcbTrack;
}

GraphicsPrimitive *PrimitivePlacementController::createPrimitiveForTool(Tool tool) const
{
    switch (tool) {
    case Tool::Line: return new PrimitiveLine();
    case Tool::Rectangle: return new PrimitiveRectangle();
    case Tool::Ellipse: return new PrimitiveEllipse();
    case Tool::Bezier: return new PrimitiveBezier();
    case Tool::Connection: return new PrimitiveConnection();
    case Tool::PcbTrack: return new PrimitivePcbTrack();
    case Tool::Pad: return new PrimitivePad();
    case Tool::Polygon: return new PrimitivePolygon();
    case Tool::Curve: return new PrimitiveComplexCurve();
    case Tool::Text: return new PrimitiveText(); // caller sets its text content
    case Tool::Macro: return new PrimitiveMacro(); // caller sets its macro key
    case Tool::Image: return new PrimitiveImage(); // caller sets its image data
    case Tool::Select: return nullptr;
    }
    return nullptr;
}

void PrimitivePlacementController::applyDefaults(GraphicsPrimitive *primitive) const
{
    if (m_layerCombo && m_layerCombo->selectedLayer())
        primitive->setLayer(m_layerCombo->selectedLayer());
    if (m_fillCheckBox)
        primitive->setIsFilled(m_fillCheckBox->isChecked());
}

void PrimitivePlacementController::startPlacement(const QPointF &scenePos)
{
    // An explicit toolbar tool always takes priority over an armed macro -
    // e.g. the user armed a macro from the library panel, then changed their
    // mind and picked "Line" from the toolbar instead.
    if (currentTool() != Tool::Select)
        m_armedMacroKey.clear();

    if (!m_armedMacroKey.isEmpty()) {
        m_activeTool = Tool::Macro;
        auto *macro = static_cast<PrimitiveMacro *>(createPrimitiveForTool(Tool::Macro));
        macro->setMacroName(m_armedMacroKey);
        m_activePrimitive = macro;
        m_pointsPlaced = 1;
        m_activePrimitive->setControlPoint(0, scenePos);
        m_sheet->addPrimitive(m_activePrimitive);
        finishPlacement();
        return;
    }

    m_activeTool = currentTool();
    if (m_activeTool == Tool::Select)
        return;

    if (m_activeTool == Tool::Text) {
        const QString text = QInputDialog::getText(m_view, tr("Testo"), tr("Contenuto:"));
        if (text.isEmpty()) {
            m_activeTool = Tool::Select;
            return; // user cancelled - nothing to place
        }
        auto *primitiveText = static_cast<PrimitiveText *>(createPrimitiveForTool(Tool::Text));
        primitiveText->setText(text);
        m_activePrimitive = primitiveText;
    } else {
        m_activePrimitive = createPrimitiveForTool(m_activeTool);
    }

    applyDefaults(m_activePrimitive);

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

    // The 50% preview opacity armImagePlacement() set while this was still
    // following the cursor was never the user's actual choice - restore the
    // real default now that it's actually being placed.
    if (m_activePrimitive && m_activeTool == Tool::Image)
        static_cast<PrimitiveImage *>(m_activePrimitive)->setOpacity(1.0);

    GraphicsPrimitive *finished = m_activePrimitive;
    const Tool finishedTool = m_activeTool;
    // Line/PCB-track chaining needs the endpoint just placed - grab it before
    // finished's ownership/selection state changes below.
    const bool shouldChain = finished && isChainableTool(finishedTool);
    const QPointF chainStart = shouldChain
            ? finished->controlPoint(finished->controlPointCount() - 1)
            : QPointF();

    // Select the just-finished primitive so its resize handles appear
    // immediately, without an extra click - and only it, so drawing several
    // primitives in a row doesn't pile up a multi-selection.
    if (finished) {
        m_sheet->clearSelection();
        finished->setSelected(true);
        // The primitive is already in the sheet (added back in
        // startPlacement()) - push() calling redo() once more is a harmless
        // no-op (Sheet::addPrimitive() is idempotent).
        m_sheet->undoStack()->push(new CreatePrimitiveCommand(m_sheet, finished));
    }

    m_activePrimitive = nullptr;
    m_pointsPlaced = 0;
    m_activeTool = Tool::Select;
    m_armedMacroKey.clear();

    // Unlike Rectangle/Text/etc., staying on the tool wouldn't let the next
    // click place another image anyway - it always needs a fresh file pick
    // first (armImagePlacement(), triggered from the toolbar button, not
    // from a click here) - so leaving "Immagine" checked would just make
    // the next canvas click silently do nothing.
    if (finishedTool == Tool::Image)
        switchToolBarToSelectTool();

    if (shouldChain)
        startChainedSegment(finishedTool, chainStart);
}

void PrimitivePlacementController::armMacroPlacement(const QString &macroKey)
{
    if (m_activePrimitive)
        cancelPlacement();
    switchToolBarToSelectTool();
    m_armedMacroKey = macroKey;
}

void PrimitivePlacementController::startChainedSegment(Tool tool, const QPointF &startPoint)
{
    m_activeTool = tool;
    m_activePrimitive = createPrimitiveForTool(tool);
    applyDefaults(m_activePrimitive);
    m_activePrimitive->setControlPoint(0, startPoint);
    m_activePrimitive->setControlPoint(1, startPoint);
    m_pointsPlaced = 1;
    m_sheet->addPrimitive(m_activePrimitive);
}

void PrimitivePlacementController::discardActivePrimitive()
{
    if (m_activePrimitive)
        m_sheet->removePrimitive(m_activePrimitive);
    m_activePrimitive = nullptr;
    m_pointsPlaced = 0;
}

void PrimitivePlacementController::cancelPlacement()
{
    discardActivePrimitive();
    m_activeTool = Tool::Select;
    switchToolBarToSelectTool();
}

void PrimitivePlacementController::handleToolBarActionTriggered(QAction *action)
{
    discardActivePrimitive();
    m_armedMacroKey.clear();

    if (action && action->objectName() == QStringLiteral("actionImage"))
        armImagePlacement();
}

void PrimitivePlacementController::armImagePlacement()
{
    const QString path = QFileDialog::getOpenFileName(
                m_view, tr("Inserisci immagine"), QString(),
                tr("Immagini (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (path.isEmpty()) {
        switchToolBarToSelectTool();
        return; // user cancelled - nothing to place
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(m_view, tr("Inserisci immagine"),
                              tr("Impossibile leggere il file:\n%1").arg(path));
        switchToolBarToSelectTool();
        return;
    }
    const QByteArray data = file.readAll();

    // FIDOSPECS.md 5.12's mime subtype, not a full "image/..." string -
    // ".jpg" is the common file extension but "jpeg" is the real subtype.
    QString mimeSubtype = QFileInfo(path).suffix().toLower();
    if (mimeSubtype == QStringLiteral("jpg"))
        mimeSubtype = QStringLiteral("jpeg");

    auto *primitiveImage = static_cast<PrimitiveImage *>(createPrimitiveForTool(Tool::Image));
    const QString base64Data = QString::fromLatin1(data.toBase64());
    primitiveImage->setImageData(mimeSubtype, base64Data);
    // Half-transparent while it's still just following the cursor, so it
    // reads as a not-yet-placed preview rather than the real, final image -
    // finishPlacement() restores full opacity the moment it's actually
    // dropped.
    primitiveImage->setOpacity(0.5);
    applyDefaults(primitiveImage);

    // Decodes the same bytes already in memory rather than a second disk
    // read, just to learn the natural pixel size - scaled down to a sane
    // on-sheet footprint (scene units == FidoCadJ grid units, 5 mil each;
    // a photo's raw pixel dimensions would dwarf the whole sheet) while
    // keeping its aspect ratio.
    QPixmap pixmap;
    pixmap.loadFromData(QByteArray::fromBase64(base64Data.toLatin1()));
    constexpr qreal MaxDimension = 100.0;
    qreal width = MaxDimension, height = MaxDimension;
    if (!pixmap.isNull() && pixmap.width() > 0 && pixmap.height() > 0) {
        const qreal scale = MaxDimension / qMax(pixmap.width(), pixmap.height());
        width = pixmap.width() * scale;
        height = pixmap.height() * scale;
    }
    m_imageHalfSize = QPointF(width / 2.0, height / 2.0);

    m_activeTool = Tool::Image;
    m_activePrimitive = primitiveImage;
    m_pointsPlaced = 0; // still tracking the cursor, not placed yet

    // Seeds it at the view's current center so it's visible right away,
    // before the very next mouse-move takes over and starts following the
    // cursor instead.
    const QPointF center = m_view ? m_view->mapToScene(m_view->viewport()->rect().center()) : QPointF();
    primitiveImage->setControlPoint(0, center - m_imageHalfSize);
    primitiveImage->setControlPoint(1, center + m_imageHalfSize);

    m_sheet->addPrimitive(primitiveImage);
}

bool PrimitivePlacementController::handleMouseRightClick()
{
    if (!m_activePrimitive || !isChainableTool(m_activeTool))
        return false;

    // Ends the chain (discarding the not-yet-placed next segment) without
    // leaving the tool - matching FidoCadJ, where right-click resets
    // clickNumber to 0 but leaves actionSelected on LINE/PCB_LINE, ready for
    // a fresh, independent segment. That's different from Escape, which
    // leaves drawing mode entirely.
    m_sheet->removePrimitive(m_activePrimitive);
    m_activePrimitive = nullptr;
    m_pointsPlaced = 0;
    return true;
}

void PrimitivePlacementController::switchToolBarToSelectTool()
{
    if (!m_toolBar)
        return;
    for (QAction *action : m_toolBar->actions()) {
        if (action->objectName() == QStringLiteral("actionSelect")) {
            // trigger(), not setChecked() - it needs to emit triggered() so
            // ToolBarPrimitive's actionTriggered handler runs and actually
            // unchecks the drawing tool that was active (currentTool() reads
            // back from the toolbar's checked action, not from m_activeTool).
            if (!action->isChecked())
                action->trigger();
            return;
        }
    }
}

bool PrimitivePlacementController::handleMousePress(const QPointF &scenePos)
{
    if (!m_activePrimitive) {
        startPlacement(scenePos);
        return m_activePrimitive != nullptr;
    }

    if (m_activeTool == Tool::Image) {
        // Both corners move together, at their already-fixed size - a click
        // just confirms wherever it's currently centered, unlike Rectangle's
        // two independent corner clicks.
        m_activePrimitive->setControlPoint(0, scenePos - m_imageHalfSize);
        m_activePrimitive->setControlPoint(1, scenePos + m_imageHalfSize);
        finishPlacement();
        return true;
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

    if (m_activeTool == Tool::Image) {
        m_activePrimitive->setControlPoint(0, scenePos - m_imageHalfSize);
        m_activePrimitive->setControlPoint(1, scenePos + m_imageHalfSize);
        return true;
    }

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
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_activePrimitive && isVariableVertexTool(m_activeTool)) {
            finishPlacement();
            return true;
        }
        return false;
    }

    if (event->key() != Qt::Key_Escape)
        return false;

    // Escape always leaves drawing mode entirely and returns to Select,
    // regardless of which of these three "something is active" states it
    // catches: an in-progress primitive to discard, an armed macro (from
    // the library panel) with no click yet, or just a drawing tool picked
    // on the toolbar with nothing placed at all - matching the reference
    // FidoCadJ editor (PopUpMenu's Escape binding goes to the same
    // "selection" action as pressing A).
    if (m_activePrimitive) {
        cancelPlacement();
        return true;
    }
    if (!m_armedMacroKey.isEmpty()) {
        m_armedMacroKey.clear();
        switchToolBarToSelectTool();
        return true;
    }
    if (currentTool() != Tool::Select) {
        switchToolBarToSelectTool();
        return true;
    }
    return false;
}
