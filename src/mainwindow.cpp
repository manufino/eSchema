#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "MirrorPrimitiveCommand.h"
#include "RotatePrimitiveCommand.h"
#include "DeletePrimitiveCommand.h"
#include "CreatePrimitiveCommand.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QGuiApplication>
#include <QClipboard>
#include <QCloseEvent>

namespace {
// FidoCadJ's 16 default layers and colors (FIDOSPECS.md 3.1). Populating all
// of them (rather than just a single master layer) at startup matters for
// FidoCadJ round-trip fidelity: opening a file that references layer index 2
// must land on that layer, not silently collapse to layer 0 for want of one
// existing.
void createDefaultLayers()
{
    static const QColor colors[16] = {
        QColor(0, 0, 0),        QColor(0, 0, 128),      QColor(255, 0, 0),      QColor(0, 128, 128),
        QColor(255, 200, 0),    QColor(127, 255, 0),    QColor(0, 255, 255),    QColor(0, 128, 0),
        QColor(154, 205, 50),   QColor(255, 20, 147),   QColor(181, 155, 12),   QColor(1, 128, 255),
        QColor(225, 225, 225, 242), QColor(162, 162, 162, 230), QColor(95, 95, 95, 230), QColor(0, 0, 0)
    };
    for (int i = 0; i < 16; ++i)
        LayerList::getInstance().addLayer(new Layer(QString("Layer %1").arg(i), colors[i], i == 0));
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    createDefaultLayers();

    ui->setupUi(this);
    updateWindowTitle();

    layerToolBarWidget = new LayerToolBarWidget(this);
    ui->toolBarTools->addWidget(layerToolBarWidget); // aggiungo il layer combobox alla toolbar

    sheetScene = new Sheet();
    sheetScene->setSceneRect(0,0,5000,5000); // fisso le dimensioni della scena

    ui->graphicsView->setScene(sheetScene);

    placementController = new PrimitivePlacementController(ui->graphicsView, sheetScene,
                                                             ui->toolBarPrimitive, ui->cbPropLayer,
                                                             ui->checkBox, this);
    ui->graphicsView->setPlacementController(placementController);

    selectionHandleController = new SelectionHandleController(sheetScene, this);

    setConnections();
}

MainWindow::~MainWindow()
{
    Utils::instance().DeleteSafely(sheetScene);
    Utils::instance().DeleteSafely(layerToolBarWidget);
    Utils::instance().DeleteSafely(ui);
}

void MainWindow::setConnections()
{
    connect(ui->graphicsView, &SheetView::mouseMoved, ui->statusbar, &StatusBar::sceneMousePos);
    connect(ui->actionOptions, &QAction::triggered, this, &MainWindow::clickOptionAction);
    connect(ui->actionInformation, &QAction::triggered, this, &MainWindow::clickAboutAction);
    connect(ui->statusbar->btnGrid, &QPushButton::toggled, ui->graphicsView, &SheetView::enableGrid);
    connect(ui->graphicsView, &SheetView::zoomScaleIsChanged, ui->statusbar, &StatusBar::zoomLevel);
    connect(ui->actionAdjustView, &QAction::triggered, ui->graphicsView, &SheetView::adjustView);
    connect(ui->actionLayerManager, &QAction::triggered, this, &MainWindow::clickLayerManagerAction);
    connect(ui->actionShortcuts, &QAction::triggered, this, &MainWindow::clickShortcutsAction);
    connect(ui->DSpinBoxLineHeight, &QSpinBox::valueChanged, ui->cbPropLineStyle, &PenStyleComboBox::lineWidthChanged);
    connect(ui->actionMirror, &QAction::triggered, this, &MainWindow::clickMirrorAction);
    connect(ui->actionRotate, &QAction::triggered, this, &MainWindow::clickRotateAction);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::clickDeleteAction);
    connect(ui->actionCut, &QAction::triggered, this, &MainWindow::clickCutAction);
    connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::clickCopyAction);
    connect(ui->actionPaste, &QAction::triggered, this, &MainWindow::clickPasteAction);
    connect(ui->actionDuplicate, &QAction::triggered, this, &MainWindow::clickDuplicateAction);
    connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::clickSelectAllAction);
    connect(ui->actionNewDraw, &QAction::triggered, this, &MainWindow::clickNewAction);
    connect(ui->actionOpenFile, &QAction::triggered, this, &MainWindow::clickOpenAction);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::clickSaveAction);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::clickSaveAsAction);
    // QWidget::close() reaches closeEvent(), which does the unsaved-changes
    // check - so File > Close and the window's own titlebar X button behave
    // identically instead of needing separate logic.
    connect(ui->actionClose, &QAction::triggered, this, &QWidget::close);

    QUndoStack *undo = sheetScene->undoStack();
    connect(ui->actionUndo, &QAction::triggered, undo, &QUndoStack::undo);
    connect(ui->actionRestore, &QAction::triggered, undo, &QUndoStack::redo);
    connect(undo, &QUndoStack::canUndoChanged, ui->actionUndo, &QAction::setEnabled);
    connect(undo, &QUndoStack::canRedoChanged, ui->actionRestore, &QAction::setEnabled);
    connect(undo, &QUndoStack::undoTextChanged, this, [this](const QString &text) {
        ui->actionUndo->setText(text.isEmpty() ? tr("Annulla") : tr("Annulla: %1").arg(text));
    });
    connect(undo, &QUndoStack::redoTextChanged, this, [this](const QString &text) {
        ui->actionRestore->setText(text.isEmpty() ? tr("Ripristina") : tr("Ripristina: %1").arg(text));
    });
    ui->actionUndo->setEnabled(undo->canUndo());
    ui->actionRestore->setEnabled(undo->canRedo());
}

void MainWindow::clickOptionAction()
{
    optionDialog = new DialogOptions(this);
    connect(optionDialog, &QDialog::finished, optionDialog, &QObject::deleteLater);
    optionDialog->show();
}

void MainWindow::clickAboutAction()
{
    aboutDialog = new DialogAbout(this);
    connect(aboutDialog, &QDialog::finished, aboutDialog, &QObject::deleteLater);
    aboutDialog->show();
}

void MainWindow::clickShortcutsAction()
{
    shortcutsDialog = new DialogShortcuts(this);
    connect(shortcutsDialog, &QDialog::finished, shortcutsDialog, &QObject::deleteLater);
    shortcutsDialog->show();
}

void MainWindow::clickLayerManagerAction()
{
    layerManager = new DialogLayerList(this);
    connect(layerManager, &QDialog::finished, layerManager, &QObject::deleteLater);
    layerManager->show();
}

// Mirror/Rotate/Delete/SelectAll all operate on the current scene selection.
// Mirror and Rotate pivot around controlPoint(0) of the first selected
// primitive in document order - matching the reference FidoCadJ editor
// (EditorActions.rotateAllSelected/mirrorAllSelected pivot on
// getFirstSelectedPrimitive().getFirstPoint()), rather than e.g. the
// selection's bounding-box center.
GraphicsPrimitive *MainWindow::firstSelectedPrimitive() const
{
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        if (primitive->isSelected())
            return primitive;
    }
    return nullptr;
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

QList<GraphicsPrimitive *> MainWindow::selectedPrimitivesInOrder() const
{
    QList<GraphicsPrimitive *> result;
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        if (primitive->isSelected())
            result.append(primitive);
    }
    return result;
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

void MainWindow::updateWindowTitle()
{
    const QString name = currentFilePath.isEmpty()
            ? tr("Nuovo disegno* (non salvato)")
            : QFileInfo(currentFilePath).fileName();
    setWindowTitle(QString("  eSchema  [ Ver. ") + APP_VERSION + QString(" BETA ]  -  ") + name);
}

void MainWindow::setCurrentFilePath(const QString &filePath)
{
    currentFilePath = filePath;
    updateWindowTitle();
}

bool MainWindow::saveToPath(const QString &filePath)
{
    QString error;
    if (!FidoCadWriter::writeFile(sheetScene, filePath, &error)) {
        QMessageBox::warning(this, tr("Errore"), tr("Impossibile salvare il file:\n%1").arg(error));
        return false;
    }
    // Marks the undo stack's current position as "the saved state" - isClean()
    // (used by confirmDiscardChanges()) reports true again until the next
    // change, rather than staying permanently "dirty" after the first edit.
    sheetScene->undoStack()->setClean();
    return true;
}

// If there are unsaved changes, asks whether to save them before whatever the
// caller is about to do (close the app, discard the document for New/Open).
// Returns false only when the user cancels - callers must abort in that case.
bool MainWindow::confirmDiscardChanges()
{
    if (sheetScene->undoStack()->isClean())
        return true;

    const auto answer = QMessageBox::question(
                this, tr("Modifiche non salvate"),
                tr("Ci sono modifiche non salvate. Vuoi salvarle?"),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);

    if (answer == QMessageBox::Cancel)
        return false;
    if (answer == QMessageBox::Discard)
        return true;

    clickSaveAction();
    // clickSaveAction() falls back to Save As for a never-saved document; if
    // the user then cancels that dialog, the stack is still dirty - treat
    // that the same as cancelling here, rather than closing/discarding anyway.
    return sheetScene->undoStack()->isClean();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscardChanges())
        event->accept();
    else
        event->ignore();
}

void MainWindow::clickNewAction()
{
    sheetScene->clearPrimitives();
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(QString());
}

void MainWindow::clickOpenAction()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Apri disegno"), QString(),
                                                        tr("FidoCadJ (*.fcd)"));
    if (path.isEmpty())
        return;

    openFile(path);
}

bool MainWindow::openFile(const QString &filePath)
{
    QString error;
    if (!FidoCadReader::readFile(filePath, sheetScene, &error)) {
        QMessageBox::warning(this, tr("Errore"), tr("Impossibile aprire il file:\n%1").arg(error));
        return false;
    }
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(filePath);
    return true;
}

void MainWindow::clickSaveAction()
{
    if (currentFilePath.isEmpty()) {
        clickSaveAsAction();
        return;
    }
    saveToPath(currentFilePath);
}

void MainWindow::clickSaveAsAction()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Salva disegno con nome"), QString(),
                                                 tr("FidoCadJ (*.fcd)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".fcd"), Qt::CaseInsensitive))
        path += QStringLiteral(".fcd");

    if (saveToPath(path))
        setCurrentFilePath(path);
}
