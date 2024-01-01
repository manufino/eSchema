#include "MainWindow.h"
#include "ui_MainWindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //SettingsManager::getInstance().restoreDefaultSettings();

    ui->setupUi(this);

    setWindowTitle(QString("  eSchema  [ Ver. ") + APP_VERSION + QString(" BETA ]  -  Nuovo disegno* (non salvato)"));

    LayerComboBox *lcb = new LayerComboBox(this);
    lcb->setMinimumSize(180,32);
    ui->toolBarTools->addWidget(lcb);

    scene = new Sheet();
    scene->setSceneRect(0,0,5000,5000);

    QGraphicsRectItem *recti = new QGraphicsRectItem(100,100,100,100);
    recti->setFlags(QGraphicsItem::ItemIsSelectable |
                QGraphicsItem::ItemIsMovable |
                QGraphicsItem::ItemSendsGeometryChanges);
    recti->setPen(QPen(QColor("red")));

    scene->addItem(recti);
    scene->addRect(10,10,100,100,QPen(QColor("black"), 2));



    ui->graphicsView->setScene(scene);

    connect(ui->graphicsView, &SheetView::mouseMoved, ui->statusbar, &StatusBar::SceneMousePos);
    connect(ui->actionOptions, SIGNAL(triggered()), this, SLOT(ClickOptionMenu()));
    connect(ui->actionInformation, SIGNAL(triggered()), this, SLOT(ClickAboutMenu()));
    connect(ui->statusbar->btnGrid, &QPushButton::toggled, ui->graphicsView, &SheetView::EnableGrid);
    connect(ui->graphicsView, &SheetView::ZoomScaleIsChanged, ui->statusbar, &StatusBar::ZoomLevel);
    connect(ui->actionAdjustView, &QAction::triggered, ui->graphicsView, &SheetView::AdjustView);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ClickOptionMenu()
{
    optionDialog = new OptionsDialog(this);
    optionDialog->show();
}

void MainWindow::ClickAboutMenu()
{
    aboutDialog = new AboutDialog(this);
    aboutDialog->show();
}

