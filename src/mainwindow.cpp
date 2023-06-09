#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGraphicsSceneMouseEvent>
#include "sheetview.h"
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    scene = new Sheet();
    scene->setSceneRect(0,0,2000,2000);

    ui->graphicsView->setScene(scene);

    connect(ui->graphicsView, &SheetView::mouseMoved, ui->statusbar, &StatusBar::GraphicsViewMousePos);
    connect(ui->actionOptions, SIGNAL(triggered()), this, SLOT(ClickOptionMenu()));
    connect(ui->actionInformation, SIGNAL(triggered()), this, SLOT(ClickAboutMenu()));
    connect(ui->statusbar->btnGrid, &QPushButton::toggled, ui->graphicsView, &SheetView::EnableGrid);

}

MainWindow::~MainWindow()
{
    delete ui;
}

