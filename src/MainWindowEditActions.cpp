/*
 * MainWindowEditActions.cpp
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

// MainWindow's Edit menu / selection actions: Mirror, Rotate, Delete, Cut/
// Copy/Paste/Duplicate, Select all, Convert macro to primitives, Create
// macro from selection, Align and Distribute, plus the shared right-click
// context menu that reuses the same QAction objects. See MainWindow.cpp's
// top-of-file comment for why this lives in its own translation unit
// instead of inside MainWindow.cpp itself.

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "MirrorPrimitiveCommand.h"
#include "RotatePrimitiveCommand.h"
#include "DeletePrimitiveCommand.h"
#include "CreatePrimitiveCommand.h"
#include "MovePrimitiveCommand.h"
#include "LibraryManager.h"
#include "PrimitiveMacro.h"
#include "DialogCreateMacro.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMenu>
#include <QMessageBox>
#include <QRandomGenerator>
#include <algorithm>
#include <limits>

// Mirror/Rotate/Delete/SelectAll all operate on the current scene selection.
// Mirror and Rotate pivot around controlPoint(0) of the first selected
// primitive in document order - matching the reference FidoCadJ editor
// (EditorActions.rotateAllSelected/mirrorAllSelected pivot on
// getFirstSelectedPrimitive().getFirstPoint()), rather than e.g. the
// selection's bounding-box center.
void MainWindow::updateEditActionsState()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    const bool hasSelection = !selected.isEmpty();

    bool hasMacro = false;
    for (GraphicsPrimitive *primitive : selected) {
        if (primitive->getPrimitiveType() == GraphicsPrimitive::PartLib) {
            hasMacro = true;
            break;
        }
    }

    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionDuplicate->setEnabled(hasSelection);
    ui->actionDelete->setEnabled(hasSelection);
    ui->actionMirror->setEnabled(hasSelection);
    ui->actionRotate->setEnabled(hasSelection);
    ui->actionCreateMacro->setEnabled(hasSelection);
    ui->actionConvertMacroToPrimitives->setEnabled(hasMacro);

    ui->actionPaste->setEnabled(!QGuiApplication::clipboard()->text().isEmpty());

    const bool hasAtLeastTwo = selected.size() >= 2;
    ui->actionAlignLeft->setEnabled(hasAtLeastTwo);
    ui->actionAlignRight->setEnabled(hasAtLeastTwo);
    ui->actionAlignTop->setEnabled(hasAtLeastTwo);
    ui->actionAlignBottom->setEnabled(hasAtLeastTwo);
    ui->actionAlignCenterHorizontal->setEnabled(hasAtLeastTwo);
    ui->actionAlignCenterVertical->setEnabled(hasAtLeastTwo);

    const bool hasAtLeastThree = selected.size() >= 3;
    ui->actionDistributeHorizontal->setEnabled(hasAtLeastThree);
    ui->actionDistributeVertical->setEnabled(hasAtLeastThree);
}

// Reuses the very same ui->action* objects the Edit menu bar entry is
// built from (rather than a second, parallel set wired to the same slots),
// so this menu's content, enabled state, shortcuts and icons can never drift
// out of sync with the menu bar - see updateEditActionsState().
void MainWindow::showCanvasContextMenu(const QPoint &globalPos)
{
    QMenu menu(this);
    menu.addAction(ui->actionUndo);
    menu.addAction(ui->actionRestore);
    menu.addSeparator();
    menu.addAction(ui->actionCut);
    menu.addAction(ui->actionCopy);
    menu.addAction(ui->actionPaste);
    menu.addAction(ui->actionDuplicate);
    menu.addAction(ui->actionDelete);
    menu.addSeparator();
    menu.addAction(ui->actionRotate);
    menu.addAction(ui->actionMirror);
    menu.addSeparator();
    menu.addAction(ui->actionConvertMacroToPrimitives);
    menu.addAction(ui->actionCreateMacro);
    menu.addSeparator();
    menu.addAction(ui->actionAlignLeft);
    menu.addAction(ui->actionAlignRight);
    menu.addAction(ui->actionAlignTop);
    menu.addAction(ui->actionAlignBottom);
    menu.addAction(ui->actionAlignCenterHorizontal);
    menu.addAction(ui->actionAlignCenterVertical);
    menu.addSeparator();
    menu.addAction(ui->actionDistributeHorizontal);
    menu.addAction(ui->actionDistributeVertical);
    menu.addSeparator();
    menu.addAction(ui->actionSelectAll);
    menu.exec(globalPos);
}

// Mirror/Rotate/Delete are never applied directly here - each pushes an undo
// command instead, whose redo() (auto-invoked once by QUndoStack::push()) is
// the sole place the actual mutation happens. Multiple selected primitives
// are wrapped in a macro so they undo/redo together as one step.
void MainWindow::clickMirrorAction()
{
    GraphicsPrimitive *pivotPrimitive = firstSelectedPrimitive();
    if (!pivotPrimitive)
        return;
    const QPointF pivot = pivotPrimitive->controlPoint(0);

    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Specchia"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new MirrorPrimitiveCommand(primitive, Qt::Horizontal, pivot));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickRotateAction()
{
    GraphicsPrimitive *pivotPrimitive = firstSelectedPrimitive();
    if (!pivotPrimitive)
        return;
    const QPointF pivot = pivotPrimitive->controlPoint(0);

    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Ruota"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new RotatePrimitiveCommand(primitive, pivot));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickConvertMacroToPrimitivesAction()
{
    QList<PrimitiveMacro *> macros;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
        if (auto *macro = dynamic_cast<PrimitiveMacro *>(primitive))
            macros.append(macro);
    }
    if (macros.isEmpty())
        return;

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    // Always grouped, even for a single macro: converting one already means
    // one delete plus N creates, and all of it must undo as one step.
    undo->beginMacro(tr("Converti macro in primitive"));
    for (PrimitiveMacro *macro : macros) {
        const QList<GraphicsPrimitive *> expanded = macro->convertToPrimitives(sheetScene);
        for (GraphicsPrimitive *primitive : expanded) {
            // push() calls redo() synchronously, which is what actually adds
            // the primitive to the scene - only safe to select it afterwards.
            undo->push(new CreatePrimitiveCommand(sheetScene, primitive));
            primitive->setSelected(true);
        }
        undo->push(new DeletePrimitiveCommand(sheetScene, macro));
    }
    undo->endMacro();
}

void MainWindow::clickCreateMacroAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    DialogCreateMacro dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    // A fresh, independent parse of the selection (never the live canvas
    // primitives themselves) so its control points can be freely offset
    // below without touching what's actually on screen - same round-trip
    // Duplicate/Paste already rely on.
    const QString serialized = FidoCadWriter::writeSelection(selected);
    QList<GraphicsPrimitive *> clones = FidoCadReader::parse(serialized, sheetScene);
    if (clones.isEmpty())
        return;

    // Anchors the macro at the selection's top-left corner: every clone is
    // shifted so that corner lands exactly on the library format's
    // conventional (100,100) macro origin, matching how every shipped
    // library symbol is itself laid out.
    QRectF bounds;
    bool first = true;
    for (GraphicsPrimitive *clone : clones) {
        for (int i = 0; i < clone->controlPointCount(); ++i) {
            const QPointF point = clone->controlPoint(i);
            if (first) {
                bounds = QRectF(point, QSizeF(0, 0));
                first = false;
            } else {
                bounds = bounds.united(QRectF(point, QSizeF(0, 0)));
            }
        }
    }
    const QPointF offset = QPointF(100, 100) - bounds.topLeft();
    for (GraphicsPrimitive *clone : clones)
        clone->translateControlPoints(offset);
    const QString body = FidoCadWriter::writeSelection(clones);
    qDeleteAll(clones);

    // The key is never user-chosen - generated the same way the reference
    // FidoCadJ editor's own LibraryModel.createRandomMacroKey() does (a
    // short random number), retried against a collision the same way it
    // does too (up to 20 attempts, though a collision is astronomically
    // unlikely here).
    const QString libraryFilename = dialog.libraryFilename();
    QString key;
    for (int attempt = 0; attempt < 20; ++attempt) {
        key = QString::number(QRandomGenerator::global()->bounded(100000000));
        const QString fullKey = (libraryFilename + QLatin1Char('.') + key).toLower();
        if (!LibraryManager::getInstance().macro(fullKey))
            break;
    }

    QString errorMessage;
    const bool ok = LibraryManager::getInstance().addUserMacro(
                libraryFilename, dialog.libraryDisplayName(),
                key, dialog.macroName(), dialog.category(), body, &errorMessage);
    if (!ok)
        QMessageBox::warning(this, tr("Crea macro"), errorMessage);
}

void MainWindow::clickDeleteAction()
{
    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Elimina"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickSelectAllAction()
{
    for (QGraphicsItem *item : sheetScene->items())
        item->setSelected(true);
}

// Applies each primitive's computed delta as an undoable MovePrimitiveCommand
// (skipping primitives whose delta is a no-op), matching the reference
// FidoCadJ editor's align/distribute (EditorActions.java), which likewise
// moves each affected primitive and saves one undo state per operation.
void MainWindow::moveSelectedPrimitives(const QHash<GraphicsPrimitive *, QPointF> &deltas, const QString &undoLabel)
{
    QList<GraphicsPrimitive *> toMove;
    for (auto it = deltas.constBegin(); it != deltas.constEnd(); ++it) {
        if (!it.value().isNull())
            toMove.append(it.key());
    }
    if (toMove.isEmpty())
        return;

    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = toMove.size() > 1;
    if (multiple)
        undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *primitive : toMove) {
        const QVector<QPointF> before = primitive->controlPointSnapshot();
        primitive->translateControlPoints(deltas.value(primitive));
        undo->push(new MovePrimitiveCommand(primitive, before, primitive->controlPointSnapshot()));
    }
    if (multiple)
        undo->endMacro();
}

// Align/distribute operate on each primitive's own bounding box (in scene
// coordinates, since pos() is always pinned at the origin - see
// GraphicsPrimitive's header comment), mirroring the reference FidoCadJ
// editor's per-primitive getPosition()/getSize() - but computed as a proper
// min/max reduction rather than replicating a latent bug in FidoCadJ's own
// getSize() that mishandles primitives with more than 2 control points
// (polygons, complex curves, macros).
void MainWindow::clickAlignLeftAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal leftmost = std::numeric_limits<qreal>::max();
    for (GraphicsPrimitive *primitive : selected)
        leftmost = qMin(leftmost, primitive->boundingRect().left());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(leftmost - primitive->boundingRect().left(), 0));
    moveSelectedPrimitives(deltas, tr("Allinea a sinistra"));
}

void MainWindow::clickAlignRightAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal rightmost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected)
        rightmost = qMax(rightmost, primitive->boundingRect().right());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(rightmost - primitive->boundingRect().right(), 0));
    moveSelectedPrimitives(deltas, tr("Allinea a destra"));
}

void MainWindow::clickAlignTopAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal topmost = std::numeric_limits<qreal>::max();
    for (GraphicsPrimitive *primitive : selected)
        topmost = qMin(topmost, primitive->boundingRect().top());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, topmost - primitive->boundingRect().top()));
    moveSelectedPrimitives(deltas, tr("Allinea in alto"));
}

void MainWindow::clickAlignBottomAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal bottommost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected)
        bottommost = qMax(bottommost, primitive->boundingRect().bottom());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, bottommost - primitive->boundingRect().bottom()));
    moveSelectedPrimitives(deltas, tr("Allinea in basso"));
}

// Aligns every selected primitive's own vertical center to a shared
// horizontal line (the selection's overall vertical midpoint) - named after
// FidoCadJ's alignHorizontalCenterSelected, which centers things relative to
// a horizontal centerline (what other tools often call "align middle").
void MainWindow::clickAlignCenterHorizontalAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal topmost = std::numeric_limits<qreal>::max();
    qreal bottommost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected) {
        const QRectF rect = primitive->boundingRect();
        topmost = qMin(topmost, rect.top());
        bottommost = qMax(bottommost, rect.bottom());
    }
    const qreal verticalCenter = (topmost + bottommost) / 2.0;

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, verticalCenter - primitive->boundingRect().center().y()));
    moveSelectedPrimitives(deltas, tr("Allinea al centro orizzontale"));
}

// Aligns every selected primitive's own horizontal center to a shared
// vertical line - FidoCadJ's alignVerticalCenterSelected ("align center" in
// more common CAD/DTP terminology).
void MainWindow::clickAlignCenterVerticalAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal leftmost = std::numeric_limits<qreal>::max();
    qreal rightmost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected) {
        const QRectF rect = primitive->boundingRect();
        leftmost = qMin(leftmost, rect.left());
        rightmost = qMax(rightmost, rect.right());
    }
    const qreal horizontalCenter = (leftmost + rightmost) / 2.0;

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(horizontalCenter - primitive->boundingRect().center().x(), 0));
    moveSelectedPrimitives(deltas, tr("Allinea al centro verticale"));
}

// Distributes selected primitives evenly by their left edges, keeping the
// leftmost and rightmost primitives fixed - matching FidoCadJ's
// distributeHorizontallySelected() exactly, including its >=3 requirement
// (with only 2 selected there's nothing "in between" to redistribute).
void MainWindow::clickDistributeHorizontalAction()
{
    QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 3)
        return;

    std::sort(selected.begin(), selected.end(), [](GraphicsPrimitive *a, GraphicsPrimitive *b) {
        return a->boundingRect().left() < b->boundingRect().left();
    });

    const qreal leftmostX = selected.first()->boundingRect().left();
    const qreal rightmostX = selected.last()->boundingRect().left();
    const qreal spacing = (rightmostX - leftmostX) / (selected.size() - 1);

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (int i = 1; i < selected.size() - 1; ++i) {
        GraphicsPrimitive *primitive = selected.at(i);
        const qreal targetX = leftmostX + i * spacing;
        deltas.insert(primitive, QPointF(targetX - primitive->boundingRect().left(), 0));
    }
    moveSelectedPrimitives(deltas, tr("Distribuisci orizzontalmente"));
}

void MainWindow::clickDistributeVerticalAction()
{
    QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 3)
        return;

    std::sort(selected.begin(), selected.end(), [](GraphicsPrimitive *a, GraphicsPrimitive *b) {
        return a->boundingRect().top() < b->boundingRect().top();
    });

    const qreal topmostY = selected.first()->boundingRect().top();
    const qreal bottommostY = selected.last()->boundingRect().top();
    const qreal spacing = (bottommostY - topmostY) / (selected.size() - 1);

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (int i = 1; i < selected.size() - 1; ++i) {
        GraphicsPrimitive *primitive = selected.at(i);
        const qreal targetY = topmostY + i * spacing;
        deltas.insert(primitive, QPointF(0, targetY - primitive->boundingRect().top()));
    }
    moveSelectedPrimitives(deltas, tr("Distribuisci verticalmente"));
}

// Copy/Cut put the selection on the system clipboard as FidoCadJ text
// (FidoCadWriter::writeSelection()) - matching the reference FidoCadJ editor,
// which copies as text rather than an app-private format, so a paste can also
// land in a plain text editor (and, conversely, so hand-written FCD snippets
// can be pasted straight into the drawing).
void MainWindow::clickCopyAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(FidoCadWriter::writeSelection(selected));
}

void MainWindow::clickCutAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(FidoCadWriter::writeSelection(selected));

    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Taglia"));
    for (GraphicsPrimitive *primitive : selected)
        undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
    if (multiple)
        undo->endMacro();
}

// Shared by Paste (parses clipboard text) and Duplicate (parses a freshly
// serialized copy of the current selection without touching the system
// clipboard at all - matching the visible result of the reference FidoCadJ
// editor's Duplicate, which is copySelected()+paste() under the hood, but
// without the side effect of clobbering whatever the user last copied).
void MainWindow::pasteFromText(const QString &text, const QString &undoLabel)
{
    const QList<GraphicsPrimitive *> pasted = FidoCadReader::parse(text, sheetScene);
    if (pasted.isEmpty())
        return;

    // Offset by one grid step and select the result, matching the reference
    // FidoCadJ editor's paste (CopyPasteActions.paste() shifts by the current
    // grid step; PopUpMenu's Paste handler passes that grid step in) - so the
    // pasted copy is visibly distinct from what was copied and immediately
    // ready to drag into place.
    const QVariant stepVal = SettingsManager::getInstance().loadSetting("snap_step");
    const int step = stepVal.isValid() && stepVal.toInt() > 0 ? stepVal.toInt() : 10;
    const QPointF offset(step, step);

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = pasted.size() > 1;
    if (multiple)
        undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *primitive : pasted) {
        primitive->translateControlPoints(offset);
        // push() calls redo() synchronously, which is what actually adds the
        // primitive to the scene - only safe to select it afterwards.
        undo->push(new CreatePrimitiveCommand(sheetScene, primitive));
        primitive->setSelected(true);
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickPasteAction()
{
    const QString text = QGuiApplication::clipboard()->text();
    if (text.isEmpty())
        return;
    pasteFromText(text, tr("Incolla"));
}

void MainWindow::clickDuplicateAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    pasteFromText(FidoCadWriter::writeSelection(selected), tr("Duplica"));
}
