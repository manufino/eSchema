#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "GraphicsItemResizer.h"

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

    QGraphicsRectItem *item = new QGraphicsRectItem(QRectF(0, 0, 100, 100));
    item->setPos(100, 100);
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setPen(QColor(102, 102, 102));
    item->setBrush(QColor(158, 204, 255));
    sheetScene->addItem(item);





    GraphicsItemResizer *resizer = new GraphicsItemResizer(item);
    resizer->setBrush(QColor(255, 0, 0));
    resizer->setMinSize(QSizeF(10, 10));
    resizer->setTargetSize(item->boundingRect().size());
    resizer->setHandlersIgnoreTransformations(true);
    QObject::connect(
        resizer,
        &GraphicsItemResizer::targetRectChanged,
        [item](const QRectF & rect)
        {
            QPointF pos = item->pos();
            item->setPos(pos + rect.topLeft());
            QRectF old = item->rect();
            item->setRect(QRectF(old.topLeft(), rect.size()));

        }
        );


    ui->graphicsView->setScene(sheetScene);

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
