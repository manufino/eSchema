/*
 * sheet.h
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

#ifndef SHEET_H
#define SHEET_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QList>
#include <QUndoStack>
#include <QImage>
#include <QString>
#include <QPointF>
#include <QSizeF>

#include "GraphicsPrimitive.h"

class QPainter;

class Sheet : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Sheet(QObject *parent = 0);

    // Primitives in insertion/document order. QGraphicsScene::items() is
    // returned in an unspecified (roughly z-order) order, but FidoCadJ
    // round-trip idempotency (FIDOSPECS.md 9) requires a stable document order,
    // so Sheet keeps its own ordered list alongside the QGraphicsScene.
    // Both are idempotent (adding an already-added/removing an already-removed
    // primitive is a harmless no-op) because QUndoStack::push() calls redo()
    // immediately on a command that may have already applied its effect live
    // (see e.g. CreatePrimitiveCommand).
    void addPrimitive(GraphicsPrimitive *primitive);
    // Destructive removal (deletes the primitive) - for callers that don't
    // need undo, e.g. clearPrimitives()/loading a new file.
    void removePrimitive(GraphicsPrimitive *primitive);
    // Non-destructive removal - the caller becomes responsible for the
    // primitive's lifetime (used by DeletePrimitiveCommand, which keeps it
    // alive so a later undo can hand it back via addPrimitive()).
    void takePrimitive(GraphicsPrimitive *primitive);
    const QList<GraphicsPrimitive*> &primitives() const { return m_primitives; }
    void clearPrimitives();

    QUndoStack *undoStack() { return &m_undoStack; }

    // Called by GraphicsPrimitive when a body drag starts with Alt held:
    // asks whoever owns the clipboard round-trip machinery (MainWindow) to
    // drop an in-place copy of the current selection before the drag moves
    // it - the classic Alt+drag "drag off a duplicate" gesture.
    void requestAltDragClone() { emit altDragCloneRequested(); }

    // Object snap (see ObjectSnap.h): when enabled, a position within a few
    // screen pixels of a characteristic point of an existing primitive
    // snaps to it - taking precedence over the grid, and highlighting the
    // captured point (drawForeground()). Falls back to plain grid snapping
    // otherwise. Shared by SheetView (placement clicks/previews) and
    // PrimitiveHandleItem (resize/node drags), each passing what it's
    // currently editing as `excluded` so a shape can't snap onto itself.
    QPointF snapPosition(const QPointF &scenePos,
                         const QList<const GraphicsPrimitive *> &excluded = {});
    void setObjectSnapEnabled(bool enabled) { m_objectSnapEnabled = enabled; clearSnapIndicator(); }
    bool objectSnapEnabled() const { return m_objectSnapEnabled; }
    // Hides the captured-point highlight - callers invoke it when their
    // drag/placement interaction ends.
    void clearSnapIndicator();

signals:
    void altDragCloneRequested();

public:

    // Document-wide FidoCadJ settings (FIDOSPECS.md 7's "FJC C/A/B"), each
    // defaulting to the spec's compiled-in default. FidoCadReader sets these
    // from an "FJC" line; FidoCadWriter re-emits one only when it differs from
    // the default, so these round-trip instead of being silently dropped.
    qreal connectionDiameter() const { return m_connectionDiameter; }
    void setConnectionDiameter(qreal diameter) { m_connectionDiameter = diameter; }
    qreal lineWidth() const { return m_lineWidth; }
    void setLineWidth(qreal width) { m_lineWidth = width; }
    qreal lineWidthCircles() const { return m_lineWidthCircles; }
    void setLineWidthCircles(qreal width) { m_lineWidthCircles = width; }

    // Background reference image for tracing - an eSchema extension: the
    // reference FidoCadJ editor's own equivalent (ImageAsCanvas) is
    // session-only and never written to the .fcd file, but eSchema persists
    // it as a "FJC IMG" line instead (FidoCadReader.cpp/FidoCadWriter.cpp),
    // which a real FidoCadJ simply skips as an unrecognized sub-code per the
    // format's own robustness contract. Distinct from PrimitiveImage (a
    // placed, selectable drawing element with its own undo history) - this
    // has neither, and always paints beneath the grid (SheetView::
    // drawTracingImage()).
    bool hasBackgroundImage() const { return !m_backgroundImage.isNull(); }
    const QImage &backgroundImage() const { return m_backgroundImage; }
    QString backgroundImageMimeSubtype() const { return m_backgroundImageMimeSubtype; }
    QString backgroundImageBase64() const { return m_backgroundImageBase64; }
    // Dots per inch, matching the reference editor's own ImageAsCanvas -
    // combined with the image's native pixel size, this determines its
    // on-sheet footprint (see backgroundImageLogicalSize()).
    qreal backgroundImageResolution() const { return m_backgroundImageResolution; }
    void setBackgroundImageResolution(qreal resolution) { m_backgroundImageResolution = resolution; }
    QPointF backgroundImageCorner() const { return m_backgroundImageCorner; }
    void setBackgroundImageCorner(const QPointF &corner) { m_backgroundImageCorner = corner; }
    // Decodes `base64Data`, replacing any previous background image.
    void setBackgroundImage(const QString &mimeSubtype, const QString &base64Data,
                             qreal resolution, const QPointF &corner);
    void clearBackgroundImage();
    // The image's footprint in logical units: FidoCadJ's logical unit is
    // fixed at 1/200 inch, so at the reference resolution (200 dpi) one
    // image pixel maps to exactly one logical unit; any other resolution
    // scales proportionally - matching the reference editor's own
    // ImageAsCanvas.drawCanvasImage() formula.
    QSizeF backgroundImageLogicalSize() const;

signals:
    // Fired whenever m_primitives changes (add/remove/clear) - drives the
    // status bar's live primitive/macro counters rather than those polling
    // primitives() on some timer.
    void primitivesChanged();

protected:
    // Repaints every pad's hole on top of everything else painted this
    // frame. QGraphicsScene paints items strictly in z/insertion order, so
    // a pad's own paint() can only punch its hole through whatever was
    // already drawn *before* it - a primitive placed on top of the pad
    // afterward would still show through the hole otherwise.
    // drawForeground() runs after every item regardless, so redoing the
    // hole here keeps it clean no matter what gets drawn after the pad too.
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private slots:
    // Connected to LayerList::layerAppearanceChanged - resyncs every
    // primitive's on-screen visibility/selectability (see
    // GraphicsPrimitive::syncLayerAppearance()) whenever a layer's eye/lock
    // state changes anywhere (toolbar combobox, DialogLayerList, ...).
    void refreshLayerAppearance();

private:
    QList<GraphicsPrimitive*> m_primitives;
    QUndoStack m_undoStack;
    bool m_objectSnapEnabled = false;
    bool m_snapIndicatorVisible = false;
    QPointF m_snapIndicator;
    qreal m_connectionDiameter = 2.0;
    qreal m_lineWidth = 0.5;
    qreal m_lineWidthCircles = 0.35;
    QImage m_backgroundImage;
    QString m_backgroundImageMimeSubtype;
    QString m_backgroundImageBase64;
    qreal m_backgroundImageResolution = 200.0;
    QPointF m_backgroundImageCorner;
};

#endif // SHEET_H
