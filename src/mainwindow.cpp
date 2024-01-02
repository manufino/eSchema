#include "MainWindow.h"
#include "ui_MainWindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString("  eSchema  [ Ver. ") + APP_VERSION + QString(" BETA ]  -  Nuovo disegno* (non salvato)"));

    layWidget = new LayerToolBarWidget(this);
    ui->toolBarTools->addWidget(layWidget);

    scene = new Sheet();
    scene->setSceneRect(0,0,5000,5000);

    ////
    QGraphicsRectItem *recti = new QGraphicsRectItem(100,100,100,100);
    recti->setFlags(QGraphicsItem::ItemIsSelectable |
                QGraphicsItem::ItemIsMovable |
                QGraphicsItem::ItemSendsGeometryChanges);
    recti->setPen(QPen(QColor("red")));
    scene->addItem(recti);
    scene->addRect(10,10,100,100,QPen(QColor("black"), 2));
    ////


    ui->graphicsView->setScene(scene);

    connect(ui->graphicsView, &SheetView::mouseMoved, ui->statusbar, &StatusBar::sceneMousePos);
    connect(ui->actionOptions, &QAction::triggered, this, &MainWindow::clickOptionAction);
    connect(ui->actionInformation, &QAction::triggered, this, &MainWindow::clickAboutAction);
    connect(ui->statusbar->btnGrid, &QPushButton::toggled, ui->graphicsView, &SheetView::enableGrid);
    connect(ui->graphicsView, &SheetView::zoomScaleIsChanged, ui->statusbar, &StatusBar::zoomLevel);
    connect(ui->actionAdjustView, &QAction::triggered, ui->graphicsView, &SheetView::adjustView);
    connect(ui->actionLayerManager, &QAction::triggered, this, &MainWindow::clickLayerManagerAction);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::clickOptionAction()
{
    optionDialog = new OptionsDialog(this);
    optionDialog->show();
}

void MainWindow::clickAboutAction()
{
    aboutDialog = new AboutDialog(this);
    aboutDialog->show();
}

void MainWindow::clickLayerManagerAction()
{
    layerManager = new LayerDialog(this);
    layerManager->show();
}

