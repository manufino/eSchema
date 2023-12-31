#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    settings = new SettingsManager();
    settings->restoreDefaultSettings();

    ui->setupUi(this);

    setWindowTitle(QString("  eSchema  [ Ver. ") + APP_VERSION + QString(" BETA ]  -  Nuovo disegno* (non salvato)"));

    LayerComboBox *lcb = new LayerComboBox(this);
    lcb->setMinimumSize(180,32);
    ui->toolBarTools->addWidget(lcb);

    scene = new Sheet();
    scene->setSceneRect(0,0,5000,5000);
    scene->addRect(10,10,100,100,QPen(QColor("black"), 2));
    ui->graphicsView->setScene(scene);

    connect(ui->graphicsView, &SheetView::mouseMoved, ui->statusbar, &StatusBar::SceneMousePos);
    connect(ui->actionOptions, SIGNAL(triggered()), this, SLOT(ClickOptionMenu()));
    connect(ui->actionInformation, SIGNAL(triggered()), this, SLOT(ClickAboutMenu()));
    connect(ui->statusbar->btnGrid, &QPushButton::toggled, ui->graphicsView, &SheetView::EnableGrid);
    connect(ui->graphicsView, &SheetView::ZoomLevel, ui->statusbar, &StatusBar::ZoomLevel);

}

MainWindow::~MainWindow()
{
    delete ui;
}

