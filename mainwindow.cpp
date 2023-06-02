#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGraphicsSceneMouseEvent>
#include "sheetview.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    scene = new Sheet();
    scene->setSceneRect(0,0,2000,2000);


    connect(ui->graphicsView, &SheetView::mouseMoved, ui->statusbar, &StatusBar::GraphicsViewMousePos);

    ui->graphicsView->setScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}

