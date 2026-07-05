#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

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
    connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::clickSelectAllAction);
    connect(ui->actionNewDraw, &QAction::triggered, this, &MainWindow::clickNewAction);
    connect(ui->actionOpenFile, &QAction::triggered, this, &MainWindow::clickOpenAction);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::clickSaveAction);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::clickSaveAsAction);
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

void MainWindow::clickMirrorAction()
{
    GraphicsPrimitive *pivotPrimitive = firstSelectedPrimitive();
    if (!pivotPrimitive)
        return;
    const QPointF pivot = pivotPrimitive->controlPoint(0);

    for (QGraphicsItem *item : sheetScene->selectedItems()) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            primitive->mirror(Qt::Horizontal, pivot);
    }
}

void MainWindow::clickRotateAction()
{
    GraphicsPrimitive *pivotPrimitive = firstSelectedPrimitive();
    if (!pivotPrimitive)
        return;
    const QPointF pivot = pivotPrimitive->controlPoint(0);

    for (QGraphicsItem *item : sheetScene->selectedItems()) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            primitive->rotate90(pivot);
    }
}

void MainWindow::clickDeleteAction()
{
    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            sheetScene->removePrimitive(primitive);
    }
}

void MainWindow::clickSelectAllAction()
{
    for (QGraphicsItem *item : sheetScene->items())
        item->setSelected(true);
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
    return true;
}

void MainWindow::clickNewAction()
{
    sheetScene->clearPrimitives();
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
