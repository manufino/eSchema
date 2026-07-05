#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    LayerList::getInstance().addLayer(new Layer("Layer 0", QColor("black"), true));

    ui->setupUi(this);
    setWindowTitle(QString("  eSchema  [ Ver. ") + APP_VERSION + QString(" BETA ]  -  Nuovo disegno* (non salvato)"));
    
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
