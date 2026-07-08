/*
 * mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDoubleSpinBox>

#include "SheetView.h"
#include "Sheet.h"
#include "DialogOptions.h"
#include "DialogAbout.h"
#include "LayerComboBox.h"
#include "SettingsManager.h"
#include "Layer.h"
#include "LayerList.h"
#include "LayerToolBarWidget.h"
#include "DialogLayerList.h"
#include "DialogShortcuts.h"
#include "GlobalUtils.h"
#include "PrimitivePlacementController.h"
#include "SelectionHandleController.h"



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class QPrinter;
class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Loads a .fcd file, e.g. one passed on the command line (matching
    // FidoCadJ's "eschema file.fcd" behaviour). Shows an error dialog and
    // returns false if the file can't be read.
    bool openFile(const QString &filePath);

protected:
    // Prompts to save unsaved changes (if any) before the window - and with
    // it, the whole application, since this is the only top-level window -
    // actually closes. Triggered both by File > Close and the window's own
    // titlebar close button, since QWidget::close() reaches here either way.
    void closeEvent(QCloseEvent *event) override;

public slots:
    void clickOptionAction();
    void clickAboutAction();
    void clickShortcutsAction();
    void clickLayerManagerAction();
    void clickMirrorAction();
    void clickRotateAction();
    void clickConvertMacroToPrimitivesAction();
    void clickCreateMacroAction();
    void clickDeleteAction();
    void clickCutAction();
    void clickCopyAction();
    void clickPasteAction();
    void clickDuplicateAction();
    void clickSelectAllAction();
    void clickAlignLeftAction();
    void clickAlignRightAction();
    void clickAlignTopAction();
    void clickAlignBottomAction();
    void clickAlignCenterHorizontalAction();
    void clickAlignCenterVerticalAction();
    void clickDistributeHorizontalAction();
    void clickDistributeVerticalAction();
    void clickNewAction();
    void clickOpenAction();
    void clickSaveAction();
    void clickSaveAsAction();
    void clickPrintAction();
    // Builds and execs the right-click menu at `globalPos` - wired to
    // SheetView::contextMenuRequested(), which has already adjusted the
    // selection (if needed) by the time this runs.
    void showCanvasContextMenu(const QPoint &globalPos);

private:
    // Renders the drawing onto the printer/preview page, scaled (preserving
    // aspect ratio) to fit within the printable area and centered on it.
    void renderForPrint(QPrinter *printer);
    void setConnections();
    // Fills ui->toolBoxLib with one page per loaded macro library, each a
    // QTreeWidget grouping macro icon+name leaves under their category node
    // - built once at startup from LibraryManager, which has already scanned
    // the "lib" directory next to the executable by the time this runs.
    void buildLibraryPanel();
    // Hides macro leaves in every library page whose name doesn't contain
    // `text` (hiding a now-empty category node too), and jumps to the first
    // page with a surviving match - wired to ui->txtSearch.
    void filterLibraryPanel(const QString &text);
    // Arms the clicked macro for placement (see
    // PrimitivePlacementController::armMacroPlacement()) - the next click on
    // the sheet drops an instance of it there. Category nodes carry no key
    // and are ignored.
    void clickLibraryMacroItem(QTreeWidgetItem *item);
    // Syncs the Proprietà panel to the current selection: populates every
    // field from the first selected primitive (a representative value for a
    // mixed-type multi-selection) and enables/disables each one by whether
    // that primitive type actually uses it (e.g. "Riempimento" only applies
    // to closed shapes, "Font testo" only to text) - wired to
    // sheetScene's selectionChanged(). Widgets are updated with their
    // signals blocked, since this reflects the selection rather than being
    // an edit itself.
    void updatePropertiesPanel();
    // Enables/disables every selection-dependent Edit action (Delete/Cut/
    // Copy/Duplicate/Mirror/Rotate/Convert macro/Create macro/Align/
    // Distribute/Paste/Select all) by the current selection's size and
    // content - shared by the menu bar's own Modifica menu (whose actions
    // this directly sets) and the right-click context menu, which reuses
    // those same QAction objects rather than keeping a second copy of this
    // logic in sync.
    void updateEditActionsState();
    GraphicsPrimitive *firstSelectedPrimitive() const;
    // Selected primitives in stable document order (sheet->primitives()
    // order), not QGraphicsScene::selectedItems()'s unspecified order - Copy
    // needs this so pasting reproduces a predictable, repeatable layout.
    QList<GraphicsPrimitive *> selectedPrimitivesInOrder() const;
    // Parses `text` as FidoCadJ primitives and adds them to the sheet,
    // offset by one grid step and selected - the shared tail end of both
    // Paste and Duplicate (see pasteFromText()'s doc comment for why they
    // share this instead of Duplicate routing through the clipboard).
    void pasteFromText(const QString &text, const QString &undoLabel);
    // Shared tail end of every align/distribute action: applies each
    // primitive's computed delta (skipping no-ops) as an undoable
    // MovePrimitiveCommand, macro-grouped so the whole alignment undoes in
    // one step.
    void moveSelectedPrimitives(const QHash<GraphicsPrimitive *, QPointF> &deltas, const QString &undoLabel);
    bool saveToPath(const QString &filePath);
    void setCurrentFilePath(const QString &filePath);
    void updateWindowTitle();
    // Asks "save changes?" if the undo stack isn't clean. Returns true if the
    // caller may proceed (nothing to save, changes were saved, or the user
    // chose to discard them) and false if the user cancelled.
    bool confirmDiscardChanges();

private:
    Ui::MainWindow *ui;
    Sheet *sheetScene;
    DialogOptions *optionDialog;
    DialogAbout *aboutDialog;
    DialogLayerList *layerManager;
    DialogShortcuts *shortcutsDialog;
    LayerToolBarWidget *layerToolBarWidget;
    PrimitivePlacementController *placementController;
    SelectionHandleController *selectionHandleController;
    QString currentFilePath; // empty = new/unsaved drawing
};
#endif // MAINWINDOW_H
