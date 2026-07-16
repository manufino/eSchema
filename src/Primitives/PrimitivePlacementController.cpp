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
#include "SettingsManager.h"
#include <QAction>
#include <QCheckBox>
#include <QKeyEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QLineF>
#include <QtMath>
#include <QUndoStack>
#include <cmath>
#include <utility>

namespace {

// Points sampled along the circular arc from `from` to `to` that passes
// through `via` (their circumcircle), spaced closely enough that the
// interpolating spline drawn through them is visually indistinguishable
// from the true arc. Falls back to the plain straight segment when the
// three points are (nearly) collinear - the circle degenerates to a line.
QVector<QPointF> sampleArcThrough(const QPointF &from, const QPointF &via, const QPointF &to)
{
    const qreal ax = from.x(), ay = from.y();
    const qreal bx = via.x(), by = via.y();
    const qreal cx = to.x(), cy = to.y();
    const qreal det = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    if (qAbs(det) < 1e-6)
        return { from, to };

    const qreal aSq = ax * ax + ay * ay;
    const qreal bSq = bx * bx + by * by;
    const qreal cSq = cx * cx + cy * cy;
    const QPointF center((aSq * (by - cy) + bSq * (cy - ay) + cSq * (ay - by)) / det,
                         (aSq * (cx - bx) + bSq * (ax - cx) + cSq * (bx - ax)) / det);
    const qreal radius = QLineF(center, from).length();

    const qreal angleFrom = std::atan2(ay - center.y(), ax - center.x());
    const qreal angleVia = std::atan2(by - center.y(), bx - center.x());
    const qreal angleTo = std::atan2(cy - center.y(), cx - center.x());

    // Counterclockwise sweep from `fromAngle` up to `toAngle`, in [0, 2pi).
    auto ccwSweep = [](qreal fromAngle, qreal toAngle) {
        qreal sweep = toAngle - fromAngle;
        while (sweep < 0.0)
            sweep += 2.0 * M_PI;
        while (sweep >= 2.0 * M_PI)
            sweep -= 2.0 * M_PI;
        return sweep;
    };
    // Go whichever way around the circle actually passes through `via`.
    const qreal sweepTo = ccwSweep(angleFrom, angleTo);
    const qreal sweepVia = ccwSweep(angleFrom, angleVia);
    const qreal sweep = (sweepVia <= sweepTo) ? sweepTo : sweepTo - 2.0 * M_PI;

    const qreal arcLength = radius * qAbs(sweep);
    const qreal sampleValue = SettingsManager::getInstance()
            .loadSetting("curve_sampling_step").toDouble();
    const qreal sampleStep = sampleValue > 0 ? qBound(1.0, sampleValue, 50.0) : 5.0;
    const int steps = qBound(8, qRound(arcLength / sampleStep), 120);
    QVector<QPointF> points;
    points.reserve(steps + 1);
    for (int i = 0; i <= steps; ++i) {
        const qreal angle = angleFrom + sweep * i / steps;
        points.append(center + radius * QPointF(std::cos(angle), std::sin(angle)));
    }
    // The sampled endpoints are the clicked ones up to rounding - pin them
    // exactly so the arc starts and ends precisely where the user clicked.
    points.first() = from;
    points.last() = to;
    return points;
}

void replaceCurveVertices(PrimitiveComplexCurve *curve, const QVector<QPointF> &points)
{
    while (curve->vertexCount() > 0)
        curve->removeLastVertex();
    for (const QPointF &point : points)
        curve->appendVertex(point);
}

}

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
    if (name == QStringLiteral("actionRegularPolygon")) return Tool::RegularPolygon;
    if (name == QStringLiteral("actionEllipse")) return Tool::Ellipse;
    if (name == QStringLiteral("actionBezier")) return Tool::Bezier;
    if (name == QStringLiteral("actionCurve")) return Tool::Curve;
    if (name == QStringLiteral("actionArc")) return Tool::Arc;
    if (name == QStringLiteral("actionText")) return Tool::Text;
    if (name == QStringLiteral("actionConnection")) return Tool::Connection;
    if (name == QStringLiteral("actionPcbTrack")) return Tool::PcbTrack;
    if (name == QStringLiteral("actionPad")) return Tool::Pad;
    if (name == QStringLiteral("actionImage")) return Tool::Image;
    if (name == QStringLiteral("actionMeasure")) return Tool::Measure;
    if (name == QStringLiteral("actionDimension")) return Tool::Dimension;
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
    case Tool::Arc: return 3;
    case Tool::RegularPolygon: return 2; // center, then a vertex
    case Tool::Measure: return 2;
    case Tool::Dimension: return 3; // two measured points + the line's offset
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
    case Tool::RegularPolygon: return new PrimitivePolygon();
    case Tool::Curve: return new PrimitiveComplexCurve();
    case Tool::Arc: return new PrimitiveComplexCurve(); // open, resampled along the arc while placing
    case Tool::Measure: return new PrimitiveLine(); // dashed rubber line, removed at the second click
    case Tool::Dimension: return new PrimitiveLine(); // the dimension line itself (see m_dimensionExtras)
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
        const QString text = QInputDialog::getText(m_view, tr("Text"), tr("Content:"));
        if (text.isEmpty()) {
            m_activeTool = Tool::Select;
            return; // user cancelled - nothing to place
        }
        auto *primitiveText = static_cast<PrimitiveText *>(createPrimitiveForTool(Tool::Text));
        primitiveText->setText(text);
        const QString defaultFont = SettingsManager::getInstance()
                .loadSetting("text_default_font").toString();
        if (!defaultFont.isEmpty())
            primitiveText->setFontName(defaultFont);
        m_activePrimitive = primitiveText;
    } else if (m_activeTool == Tool::RegularPolygon) {
        // Same ask-at-first-click pattern as the Text tool, remembering the
        // last chosen side count across placements and sessions.
        const int lastSides = SettingsManager::getInstance().loadSetting("regular_polygon_sides").toInt();
        bool ok = false;
        const int sides = QInputDialog::getInt(m_view, tr("Regular polygon"), tr("Number of sides:"),
                                               lastSides >= 3 ? lastSides : 6, 3, 64, 1, &ok);
        if (!ok) {
            m_activeTool = Tool::Select;
            return; // user cancelled - nothing to place
        }
        SettingsManager::getInstance().saveSetting("regular_polygon_sides", sides);
        m_regularPolygonSides = sides;
        m_activePrimitive = createPrimitiveForTool(m_activeTool);
    } else {
        m_activePrimitive = createPrimitiveForTool(m_activeTool);
    }

    applyDefaults(m_activePrimitive);

    m_pointsPlaced = 1;

    if (m_activeTool == Tool::Arc) {
        // Vertex 0 is the fixed start point; vertex 1 previews the end point
        // until the second click. The through-point click (third) never adds
        // a vertex - it resamples the whole curve along the arc it picks.
        auto *curve = static_cast<PrimitiveComplexCurve *>(m_activePrimitive);
        m_arcStart = scenePos;
        curve->appendVertex(scenePos);
        curve->appendVertex(scenePos);
    } else if (m_activeTool == Tool::RegularPolygon) {
        // The click is the center; all vertices collapse there until the
        // mouse moves and updateRegularPolygonVertices() fans them out.
        auto *polygon = static_cast<PrimitivePolygon *>(m_activePrimitive);
        m_regularPolygonCenter = scenePos;
        for (int i = 0; i < m_regularPolygonSides; ++i)
            polygon->appendVertex(scenePos);
    } else if (m_activeTool == Tool::Measure) {
        m_measureStart = scenePos;
        m_activePrimitive->penStyleIsChanged(Qt::DashLine);
        m_activePrimitive->setControlPoint(0, scenePos);
        m_activePrimitive->setControlPoint(1, scenePos);
        emit measureUpdated(measureText(scenePos, scenePos));
    } else if (m_activeTool == Tool::Dimension) {
        // First click: the first measured point. The line previews plain
        // (no arrows) until the second point fixes what is being measured.
        m_dimensionStart = scenePos;
        m_activePrimitive->setControlPoint(0, scenePos);
        m_activePrimitive->setControlPoint(1, scenePos);
        emit measureUpdated(measureText(scenePos, scenePos));
    } else if (isVariableVertexTool(m_activeTool)) {
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

// The measure readout: deltas, distance (drawing units and millimeters -
// one unit is 1/200 inch = 0.127 mm, FIDOSPECS.md 3), and the visual angle
// (y grows downward on screen, hence the sign flip).
QString PrimitivePlacementController::measureText(const QPointF &from, const QPointF &to) const
{
    const qreal dx = to.x() - from.x();
    const qreal dy = to.y() - from.y();
    const qreal length = std::hypot(dx, dy);
    const qreal angle = qRadiansToDegrees(std::atan2(-dy, dx));
    // Same unit preference as the status bar's coordinates readout
    // (Options > Interface > Coordinates display).
    const int unitsDisplay = qBound(0, SettingsManager::getInstance()
                                    .loadSetting("units_display").toInt(), 2);
    QString lengthText;
    switch (unitsDisplay) {
    case 1:
        lengthText = QString::number(length, 'f', 1);
        break;
    case 2:
        lengthText = tr("%1 mm").arg(length * 0.127, 0, 'f', 2);
        break;
    default:
        lengthText = tr("%1 (%2 mm)").arg(length, 0, 'f', 1).arg(length * 0.127, 0, 'f', 2);
        break;
    }
    return tr("dx: %1   dy: %2   length: %3   angle: %4°")
            .arg(dx, 0, 'f', 1)
            .arg(dy, 0, 'f', 1)
            .arg(lengthText)
            .arg(angle, 0, 'f', 1);
}

// Fans the polygon's vertices out around the fixed center, with the cursor
// as the first vertex - so both the radius and the orientation follow the
// mouse until the second click freezes them.
void PrimitivePlacementController::updateRegularPolygonVertices(const QPointF &vertexPos)
{
    auto *polygon = static_cast<PrimitivePolygon *>(m_activePrimitive);
    const QPointF delta = vertexPos - m_regularPolygonCenter;
    const qreal radius = std::hypot(delta.x(), delta.y());
    const qreal startAngle = std::atan2(delta.y(), delta.x());
    const int count = polygon->vertexCount();
    for (int i = 0; i < count; ++i) {
        const qreal angle = startAngle + 2.0 * M_PI * i / count;
        polygon->setControlPoint(i, m_regularPolygonCenter
                                 + radius * QPointF(std::cos(angle), std::sin(angle)));
    }
}

void PrimitivePlacementController::discardActivePrimitive()
{
    if (m_activePrimitive)
        m_sheet->removePrimitive(m_activePrimitive);
    m_activePrimitive = nullptr;
    // A dimension preview spans several primitives, not just the active one.
    for (GraphicsPrimitive *extra : std::as_const(m_dimensionExtras))
        m_sheet->removePrimitive(extra);
    m_dimensionExtras.clear();
    m_pointsPlaced = 0;
}

// The measured distance as the dimension label's text: always the physical
// length in millimeters (one logical unit = 0.127 mm) - a dimension
// annotation states a real-world size, unlike the status bar readout with
// its configurable units. The single place to extend if an imperial-units
// mode is ever added.
QString PrimitivePlacementController::dimensionLabel(qreal length) const
{
    return tr("%1 mm").arg(length * 0.127, 0, 'f', 2);
}

// Lays the whole annotation out for the dimension line passing through
// `offsetPoint`: the line runs parallel to the measured span at the
// cursor's perpendicular offset, one extension line connects each measured
// point to it, and the distance label sits just past its midpoint. The
// extension lines and the label are created lazily on the first preview.
void PrimitivePlacementController::updateDimensionPreview(const QPointF &offsetPoint)
{
    if (m_dimensionExtras.isEmpty()) {
        auto *extensionA = new PrimitiveLine();
        auto *extensionB = new PrimitiveLine();
        auto *label = new PrimitiveText();
        applyDefaults(extensionA);
        applyDefaults(extensionB);
        applyDefaults(label);
        const int textSize = SettingsManager::getInstance()
                .loadSetting("dimension_text_size").toInt();
        if (textSize > 0)
            label->setSize(textSize, qMax(1, textSize * 3 / 4));
        const QString defaultFont = SettingsManager::getInstance()
                .loadSetting("text_default_font").toString();
        if (!defaultFont.isEmpty())
            label->setFontName(defaultFont);
        m_dimensionExtras = { extensionA, extensionB, label };
        for (GraphicsPrimitive *extra : std::as_const(m_dimensionExtras))
            m_sheet->addPrimitive(extra);
    }

    const QPointF span = m_dimensionEnd - m_dimensionStart;
    const qreal length = std::hypot(span.x(), span.y());
    if (length <= 0.0)
        return;
    const QPointF normal(-span.y() / length, span.x() / length);
    const qreal offset = QPointF::dotProduct(offsetPoint - m_dimensionStart, normal);
    const QPointF lineStart = m_dimensionStart + normal * offset;
    const QPointF lineEnd = m_dimensionEnd + normal * offset;

    m_activePrimitive->setControlPoint(0, lineStart);
    m_activePrimitive->setControlPoint(1, lineEnd);
    m_dimensionExtras.at(0)->setControlPoint(0, m_dimensionStart);
    m_dimensionExtras.at(0)->setControlPoint(1, lineStart);
    m_dimensionExtras.at(1)->setControlPoint(0, m_dimensionEnd);
    m_dimensionExtras.at(1)->setControlPoint(1, lineEnd);

    auto *label = static_cast<PrimitiveText *>(m_dimensionExtras.at(2));
    label->setText(dimensionLabel(length));
    // Just past the dimension line's midpoint, on the side away from what
    // is being measured (the offset's own side), so the text never sits on
    // top of the arrows.
    const qreal side = offset >= 0.0 ? 1.0 : -1.0;
    label->setControlPoint(0, (lineStart + lineEnd) / 2.0
                           + normal * (side * (label->sizeY() / 2.0 + 1.0)));
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
                m_view, tr("Insert image"), QString(),
                tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (path.isEmpty()) {
        switchToolBarToSelectTool();
        return; // user cancelled - nothing to place
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(m_view, tr("Insert image"),
                              tr("Unable to read the file:\n%1").arg(path));
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

    if (m_activeTool == Tool::Arc) {
        auto *curve = static_cast<PrimitiveComplexCurve *>(m_activePrimitive);
        if (m_pointsPlaced == 1) {
            // Second click: fix the end point; the curve is a straight
            // two-vertex segment until the through-point is picked.
            m_arcEnd = scenePos;
            replaceCurveVertices(curve, { m_arcStart, m_arcEnd });
            m_pointsPlaced = 2;
        } else {
            // Third click: the through-point. Resample and finish.
            replaceCurveVertices(curve, sampleArcThrough(m_arcStart, scenePos, m_arcEnd));
            finishPlacement();
        }
        return true;
    }

    if (m_activeTool == Tool::RegularPolygon) {
        // Second click: the first vertex - radius and orientation both fixed.
        updateRegularPolygonVertices(scenePos);
        finishPlacement();
        return true;
    }

    if (m_activeTool == Tool::Measure) {
        // Second click: freeze the readout, discard the rubber line (it
        // never touches the undo stack), stay armed for the next measure.
        emit measureUpdated(measureText(m_measureStart, scenePos));
        m_sheet->removePrimitive(m_activePrimitive);
        m_activePrimitive = nullptr;
        m_pointsPlaced = 0;
        return true;
    }

    if (m_activeTool == Tool::Dimension) {
        if (m_pointsPlaced == 1) {
            // Second click: the other measured point. A zero-length span
            // has nothing to dimension - just keep waiting for a real one.
            if (scenePos == m_dimensionStart)
                return true;
            m_dimensionEnd = scenePos;
            m_pointsPlaced = 2;
            m_activePrimitive->setArrowAtStart(true);
            m_activePrimitive->setArrowAtEnd(true);
            updateDimensionPreview(scenePos);
            return true;
        }
        // Third click: the dimension line's final position. Everything is
        // already on the sheet as the live preview - push it all onto the
        // undo stack as one step (Sheet::addPrimitive() is idempotent, so
        // push()'s synchronous redo() is a harmless no-op), then stay armed
        // for the next dimension, like Measure does.
        updateDimensionPreview(scenePos);
        m_sheet->clearSelection();
        QUndoStack *undo = m_sheet->undoStack();
        undo->beginMacro(tr("Dimension"));
        undo->push(new CreatePrimitiveCommand(m_sheet, m_activePrimitive));
        for (GraphicsPrimitive *extra : std::as_const(m_dimensionExtras))
            undo->push(new CreatePrimitiveCommand(m_sheet, extra));
        undo->endMacro();
        m_activePrimitive->setSelected(true);
        m_activePrimitive = nullptr;
        m_dimensionExtras.clear();
        m_pointsPlaced = 0;
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

    if (m_activeTool == Tool::Arc) {
        auto *curve = static_cast<PrimitiveComplexCurve *>(m_activePrimitive);
        if (m_pointsPlaced == 1) {
            // End point still being chosen - straight-segment preview.
            curve->setControlPoint(curve->vertexCount() - 1, scenePos);
        } else {
            // Both endpoints fixed - live-preview the arc bulging through
            // the cursor.
            replaceCurveVertices(curve, sampleArcThrough(m_arcStart, scenePos, m_arcEnd));
        }
        return true;
    }

    if (m_activeTool == Tool::RegularPolygon) {
        updateRegularPolygonVertices(scenePos);
        return true;
    }

    if (m_activeTool == Tool::Measure) {
        m_activePrimitive->setControlPoint(1, scenePos);
        emit measureUpdated(measureText(m_measureStart, scenePos));
        return true;
    }

    if (m_activeTool == Tool::Dimension) {
        if (m_pointsPlaced == 1) {
            // Still choosing the second measured point - plain line preview.
            m_activePrimitive->setControlPoint(1, scenePos);
            emit measureUpdated(measureText(m_dimensionStart, scenePos));
        } else {
            // Both points fixed - the whole annotation follows the cursor's
            // perpendicular offset.
            updateDimensionPreview(scenePos);
        }
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
