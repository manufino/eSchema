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
class QTreeWidget;
class QTreeWidgetItem;
class QDragEnterEvent;
class QDropEvent;
class QMimeData;
class QTimer;
class QImage;
class QUrl;
class DialogFind;
class UpdateChecker;
class QToolBar;
namespace BooleanOps { enum class Operation; }

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
    // QMainWindow's default toolbar/dock right-click menu (the visibility
    // toggles), extended with "Customize toolbars...".
    QMenu *createPopupMenu() override;
    // Dropping a .fcd (open) or .dxf (import) file anywhere on the window
    // loads it, exactly like the corresponding File menu entry - including
    // the unsaved-changes prompt. Other drops are refused at enter time.
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public slots:
    void clickOptionAction();
    void clickCustomizeToolbarsAction();
    void clickCommandPaletteAction();
    void clickAboutAction();
    void clickShortcutsAction();
    void clickCheckUpdatesAction();
    void clickLayerManagerAction();
    void clickAttachImageAction();
    // FCD code dock (dockFcdCode): Apply re-parses the text box and replaces
    // the whole drawing with it as one undo step; Refresh discards any local
    // edit and regenerates the text from the current drawing. See
    // syncFcdCodeFromSheet()/refreshFcdCodeIfClean() for the auto-refresh
    // side of it.
    void clickApplyFcdCodeAction();
    void clickRefreshFcdCodeAction();
    void clickMirrorAction();
    void clickMirrorCopyAction();
    void clickRotateAction();
    void clickConvertMacroToPrimitivesAction();
    void clickCreateMacroAction();
    void clickDeleteAction();
    void clickCutAction();
    void clickCopyAction();
    void clickCopySplitAction();
    void clickCopyAsImageAction();
    void clickPasteAction();
    void clickPasteInPlaceAction();
    void clickDuplicateAction();
    void clickSelectAllAction();
    void clickFindAction();
    void clickAlignLeftAction();
    void clickAlignRightAction();
    void clickAlignTopAction();
    void clickAlignBottomAction();
    void clickAlignCenterHorizontalAction();
    void clickAlignCenterVerticalAction();
    void clickDistributeHorizontalAction();
    void clickDistributeVerticalAction();
    // Boolean path operations between the selected closed shapes (rectangle,
    // ellipse, polygon, closed complex curve) - see BooleanOps::combine().
    void clickBooleanUnionAction();
    void clickBooleanSubtractAction();
    void clickBooleanIntersectAction();
    // Shape submenu: rewriting a primitive's geometry in place (as one
    // delete+create undo step, like the boolean operations).
    void clickConvertToPolygonAction();
    void clickConvertToCurveAction();
    void clickSimplifyNodesAction();
    void clickFilletCornersAction();
    void clickChamferCornersAction();
    void clickOffsetOutlineAction();
    // Splits the single selected line/track/Bezier/open curve in two at a
    // clicked (snappable) point on it.
    void clickSplitAtPointAction();
    // AutoCAD-style trim/extend: click a portion of a line/track to remove
    // it up to its intersections with the other primitives, or click near
    // one of its ends to stretch that end to the first primitive it meets.
    void clickTrimToIntersectionAction();
    void clickExtendToIntersectionAction();
    // Whole-selection transforms and replication.
    void clickScaleSelectionAction();
    void clickRotateByAngleAction();
    void clickArrayAction();
    void clickSnapSelectionToGridAction();
    // Selection conveniences.
    void clickSelectSameTypeAction();
    void clickInvertSelectionAction();
    void clickNewAction();
    void clickOpenAction();
    void clickImportDxfAction();
    void clickSaveAction();
    void clickSaveAsAction();
    void clickSaveSplitAction();
    void clickPrintAction();
    void clickExportAction();
    // Builds and execs the right-click menu at `globalPos` - wired to
    // SheetView::contextMenuRequested(), which has already adjusted the
    // selection (if needed) by the time this runs. `scenePos` is where on
    // the drawing the click landed, used for context-dependent entries like
    // add/remove node.
    void showCanvasContextMenu(const QPoint &globalPos, const QPointF &scenePos);

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
    // Right-click menu for one library's tree, wired per-page in
    // buildLibraryPanel() - a no-op on a standard library (read-only, per
    // LibraryManager::isStandardLibraryFilename()). What's under `pos`
    // decides the scope: empty space offers to rename/delete the whole
    // library, a top-level (category) item offers to rename/delete that
    // category and everything in it, a leaf (macro) item offers to rename/
    // delete just that macro.
    void showLibraryContextMenu(QTreeWidget *tree, const QString &libraryFilename, const QPoint &pos);
    void renameLibraryInteractive(const QString &filename);
    void deleteLibraryInteractive(const QString &filename);
    void renameCategoryInteractive(const QString &filename, const QString &category);
    void deleteCategoryInteractive(const QString &filename, const QString &category);
    void renameMacroInteractive(const QString &key);
    void deleteMacroInteractive(const QString &key);
    // Syncs the Properties panel to the current selection: populates every
    // field from the first selected primitive (a representative value for a
    // mixed-type multi-selection) and enables/disables each one by whether
    // that primitive type actually uses it (e.g. "Fill" only applies
    // to closed shapes, "Text font" only to text) - wired to
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
    // offset by one grid step (unless `inPlace`, which keeps the original
    // coordinates - Paste in place) and selected - the shared tail end of
    // Paste, Paste in place and Duplicate (see pasteFromText()'s doc comment
    // for why they share this instead of Duplicate routing through the
    // clipboard).
    void pasteFromText(const QString &text, const QString &undoLabel, bool inPlace = false);
    // Companion to pasteFromText() for when the clipboard holds a bitmap
    // instead of FCD text - builds a new PrimitiveImage from it and adds it
    // the same undo-stack way. See its definition for details.
    void pasteImageFromClipboard(const QImage &image);
    // Shared by DialogFind's findNext()/findPrevious() signals - direction
    // is +1 or -1. Searches every primitive's name/value labels, text
    // content, and macro display names across the whole sheet, wrapping
    // around, and centers+selects whatever it lands on. See its definition
    // for the matching/cycling details.
    void findInDrawing(const QString &text, int direction);
    // Shared by "Copy split"/"Save split as...": replaces every PrimitiveMacro
    // in `source` with its convertToPrimitives() expansion (single level -
    // same depth clickConvertMacroToPrimitivesAction() itself commits to the
    // sheet, matching one convention across both features), leaving
    // non-macro primitives untouched. The returned list mixes borrowed (the
    // untouched originals - never delete these) and freshly-allocated
    // primitives; every newly-allocated one is also appended to `owned` so
    // the caller can qDeleteAll() it once done, since none of them are ever
    // added to any Sheet.
    QList<GraphicsPrimitive *> expandMacrosOneLevel(const QList<GraphicsPrimitive *> &source,
                                                      QList<GraphicsPrimitive *> &owned) const;
    // Shared tail end of every align/distribute action: applies each
    // primitive's computed delta (skipping no-ops) as an undoable
    // MovePrimitiveCommand, macro-grouped so the whole alignment undoes in
    // one step.
    void moveSelectedPrimitives(const QHash<GraphicsPrimitive *, QPointF> &deltas, const QString &undoLabel);
    // Moves the whole selection by one snap step in `direction` (a unit
    // vector), as one undoable step - wired to Alt+arrow shortcuts in
    // setConnections(), matching the reference FidoCadJ editor's own
    // Alt+arrows nudge.
    void nudgeSelection(const QPointF &direction);
    // Shared tail end of the three boolean-operation actions: replaces the
    // eligible selected shapes (see GraphicsPrimitive::supportsBooleanOps())
    // with BooleanOps::combine()'s result, as one undoable step. Ineligible
    // selected primitives (macros, images, pads, open shapes, ...) are left
    // untouched on the sheet.
    void applyBooleanOperation(BooleanOps::Operation operation, const QString &undoLabel);
    // Shared tail end of Convert to polygon / Convert to complex curve.
    void convertSelectionTo(bool toCurve, const QString &undoLabel);
    // Shared tail end of Fillet corners / Chamfer corners.
    void roundSelectedCorners(bool chamfer);
    // The line/PCB track nearest `scenePos` within a few screen pixels -
    // what a trim/extend click operates on. Null when none is close enough.
    GraphicsPrimitive *pickedLineAt(const QPointF &scenePos) const;
    // Drops an unselected in-place copy of the current selection, as one
    // undo step - the Alt+drag duplicate gesture (see Sheet::
    // altDragCloneRequested() and GraphicsPrimitive::mouseMoveEvent()).
    void cloneSelectionInPlace();
    // Stamps each panel's windowIcon onto QMainWindow's internal dock tab
    // bars, matching tabs to docks by title. Qt draws those tabs text-only
    // and exposes no public API for them (long-standing limitation), so
    // findChildren<QTabBar*>() is the accepted workaround - called after
    // every dock move/re-tab, see the constructor's dock signal wiring.
    void refreshDockTabIcons();
    // Re-applies the settings that map onto live widget/scene state (toolbar
    // icon size, object snap, the "smooth boolean results" toggle) - wired
    // to SettingsManager::settingIsChanged() so the Options dialog's Apply
    // takes effect immediately, and called once at startup.
    void applyLiveSettings();

    // --- Toolbar customization (View > Customize toolbars...) -------------
    // Every command offered by the customize dialog: the actions reachable
    // from the menu bar (recursively through submenus) that have a unique
    // objectName - excluding pure view toggles like the toolbar/rulers
    // visibility actions.
    QList<QAction *> toolbarActionCatalog() const;
    // The customizable toolbars (the two command toolbars - the drawing-tool
    // palette is tied to the tool selector and stays fixed).
    QList<QToolBar *> customizableToolbars() const;
    // `layout` as stored: action objectNames, with "separator" entries.
    // Widget actions already on the toolbar (e.g. the layer combobox) are
    // preserved in place; the commands are rebuilt before them.
    void applyToolbarLayout(QToolBar *toolBar, const QStringList &layout);
    // The current command layout of `toolBar`, in the same format.
    QStringList toolbarLayoutOf(QToolBar *toolBar) const;
    // Captures the shipped defaults, then applies any persisted
    // customization ("toolbar_custom_<objectName>") - called once at startup.
    void loadToolbarCustomizations();
    QHash<QToolBar *, QStringList> m_defaultToolbarLayouts;

    // --- Command catalog / keyboard shortcuts ------------------------------
    // Every command the command palette and the shortcut editor expose:
    // the menu bar's commands (grouped under their top-level menu's title,
    // recursively through submenus, skipping dynamically built entries) plus
    // the drawing tools. Same objectName-based rules as
    // toolbarActionCatalog(), but keeping the category alongside.
    QList<QPair<QString, QAction *>> commandCatalogByCategory() const;
    // Captures each command's shipped shortcut, then applies any persisted
    // customization ("shortcut_custom_<objectName>") - called once at
    // startup, before the user can open the editor.
    void loadShortcutCustomizations();
    QHash<QAction *, QKeySequence> m_defaultShortcuts;
    // Inserts a new vertex into `primitive` (a polygon or complex curve, per
    // GraphicsPrimitive::supportsNodeEditing()) at the edge nearest
    // `scenePos`, as one undoable step - wired to the canvas context menu's
    // add-node entry.
    void addNodeToPrimitive(GraphicsPrimitive *primitive, const QPointF &scenePos);
    // Removes `primitive`'s vertex nearest `scenePos`, as one undoable step -
    // wired to the canvas context menu's remove-node entry. A no-op if only
    // two vertices remain (removing one more would leave a degenerate point).
    void removeNodeFromPrimitive(GraphicsPrimitive *primitive, const QPointF &scenePos);
    // Replaces the sheet's contents with `filePath`'s (same bulk-load
    // contract as openFile(), but as an import: the document stays untitled
    // so the next Save goes through Save As). Shared by File > Importa da
    // DXF and dropping a .dxf onto the window. Returns false on read errors
    // (already reported to the user).
    bool importDxfFile(const QString &filePath);
    // The droppable local file carried by a drag's mime data - empty if the
    // drag holds none (wrong extension, non-file payload). Shared by
    // dragEnterEvent (to decide acceptance) and dropEvent.
    static QString droppableFilePath(const QMimeData *mimeData);
    bool saveToPath(const QString &filePath);
    void setCurrentFilePath(const QString &filePath);
    void updateWindowTitle();
    // Moves (or inserts) `filePath` to the top of the persisted recent-files
    // list and rebuilds the File > Apri recenti submenu from it.
    void addToRecentFiles(const QString &filePath);
    // Rebuilds the File > Apri recenti submenu from the "recent_files"
    // setting: one action per still-listed path (clicking it opens the file,
    // with the same unsaved-changes prompt as File > Apri) plus a trailing
    // "clear list" entry. Disabled entirely while the list is empty.
    void updateRecentFilesMenu();
    // Reads ui->graphicsView's current transform/scroll position and pushes
    // the resulting origin+scale to both rulers - wired to SheetView::
    // viewTransformChanged() (pan/zoom/resize) so they always match what's
    // actually visible.
    void updateRulers();
    // Shows/hides both rulers and the corner widget between them per the
    // "rulers_visible" setting - wired to SettingsManager::settingIsChanged
    // and called once at startup.
    void updateRulersVisibility();
    // Asks "save changes?" if the undo stack isn't clean. Returns true if the
    // caller may proceed (nothing to save, changes were saved, or the user
    // chose to discard them) and false if the user cancelled.
    bool confirmDiscardChanges();

    // Where autosaveTick() writes the recovery sidecar for the document
    // currently open: "<name>.autosave.fcd" next to a named file, or one
    // fixed slot under the app data directory for an untitled document
    // (there is only ever one open document in this single-document app,
    // so no collision risk).
    QString autosaveTargetPath() const;
    // Deletes the sidecar file (if any) and forgets it in the settings -
    // called whenever the document transitions away from needing recovery:
    // a successful Save, New, Open/Import (the previous document's context
    // is gone either way), or a clean, deliberate close.
    void clearAutosave();
    // (Re)starts the autosave timer per the "autosave_enabled"/
    // "autosave_interval_minutes" settings - wired to
    // SettingsManager::settingIsChanged so a change in the Options dialog
    // takes effect immediately, not just on the next launch.
    void restartAutosaveTimer();
    // Writes the open drawing to autosaveTargetPath() if there are unsaved
    // changes (a clean document is already safe, nothing to recover) and
    // records the sidecar's path in the settings - wired to the autosave
    // timer's timeout().
    void autosaveTick();
    // Called once at the tail of the constructor, before main.cpp can open
    // any command-line file: if a previous run left a recovery sidecar
    // behind (i.e. it wasn't cleared by a clean shutdown - see
    // clearAutosave()), offers to load it. A plain QUndoCommand is pushed
    // after a successful recovery purely to make the stack report dirty
    // (recovered content was never actually written to the real file, if
    // any) - QUndoCommand's base redo()/undo() are no-ops, so this changes
    // nothing about the drawing itself, only isClean()'s answer.
    void checkForAutosaveRecovery();
    // Reactions to UpdateChecker's signals - shared by both the silent
    // startup check and the manual "Check for updates" action. Only the
    // manual one shows anything for upToDate()/checkFailed(); an available
    // update is always offered, regardless of which triggered the check.
    // See manualUpdateCheck's own comment for how they tell which is which.
    void handleUpdateAvailable(const QString &version, const QUrl &releaseUrl);
    void handleUpdateUpToDate();
    void handleUpdateCheckFailed();
    // Unconditionally overwrites the FCD code dock's text with a fresh
    // FidoCadWriter::write(sheetScene) and clears its "modified" flag -
    // used by both the Refresh button and clickApplyFcdCodeAction() (which
    // wants the canonical re-serialization of what it just applied, not
    // whatever the user happened to type).
    void syncFcdCodeFromSheet();
    // Wired to the undo stack's indexChanged() - resyncs the code dock from
    // the sheet on every external change (drawing on the canvas, undo/redo),
    // but only while the user hasn't started editing the text themselves;
    // otherwise their in-progress edit would keep getting silently
    // discarded out from under them.
    void refreshFcdCodeIfClean();

private:
    Ui::MainWindow *ui;
    Sheet *sheetScene;
    DialogOptions *optionDialog;
    DialogAbout *aboutDialog;
    DialogLayerList *layerManager;
    DialogShortcuts *shortcutsDialog;
    DialogFind *findDialog = nullptr; // created lazily on first Ctrl+F
    QString lastFindText;
    int lastFindIndex = -1;
    UpdateChecker *updateChecker;
    // Only one check is ever in flight at a time (UpdateChecker itself
    // no-ops a second call), so a single flag - set right before calling
    // checkForUpdates() - is enough to tell the manual action's check apart
    // from the silent startup one when the result comes back.
    bool manualUpdateCheck = false;
    LayerToolBarWidget *layerToolBarWidget;
    PrimitivePlacementController *placementController;
    SelectionHandleController *selectionHandleController;
    QString currentFilePath; // empty = new/unsaved drawing
    QTimer *autosaveTimer;
    // Set by clickPrintAction() from the just-accepted DialogPrintOptions,
    // read back by renderForPrint() - see its own comment for why these
    // can't just be local variables passed as arguments.
    bool m_printMirror = false;
    bool m_printBlackWhite = false;
    Layer *m_printOneLayer = nullptr; // non-null = print only this layer
    bool m_printRealScale = false;    // false = fit to page (the default)
    double m_printScalePercent = 100.0;
    // Index (into sheetScene->guides()) of the guide a ruler drag-out is
    // currently placing, -1 when none - see the ruler wiring in
    // setConnections().
    int m_rulerGuideIndex = -1;
};
#endif // MAINWINDOW_H
