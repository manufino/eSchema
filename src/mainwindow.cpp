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

    ////
    QGraphicsRectItem *recti = new QGraphicsRectItem(100,100,100,100);
    recti->setFlags(QGraphicsItem::ItemIsSelectable |
                QGraphicsItem::ItemIsMovable |
                QGraphicsItem::ItemSendsGeometryChanges);
    recti->setPen(QPen(QColor("red")));
    sheetScene->addItem(recti);
    sheetScene->addRect(10,10,100,100,QPen(QColor("black"), 2));
    ////

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
