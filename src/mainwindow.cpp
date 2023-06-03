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


    connect(ui->graphicsView, &SheetView::mouseMoved, ui->statusbar, &StatusBar::GraphicsViewMousePos);

    ui->graphicsView->setScene(scene);

    connect(ui->actionOptions, SIGNAL(triggered()), this, SLOT(ClickOptionMenu()));
    connect(ui->actionInformation, SIGNAL(triggered()), this, SLOT(ClickAboutMenu()));

    QPushButton *btnGrid = new QPushButton(ui->statusbar);
    btnGrid->setCheckable(true);
    btnGrid->setChecked(true);

    connect(btnGrid, &QPushButton::toggled, ui->graphicsView, &SheetView::EnableGrid);

    QIcon icon(":/res/resources/grid.ico");
    btnGrid->setIcon(icon);
    btnGrid->setIconSize(QSize(24,24));
    btnGrid->setMinimumSize(QSize(24,24));
    btnGrid->setMaximumSize(QSize(24,24));
    btnGrid->setToolTip("Attiva o disattiva la griglia");

    ui->statusbar->addWidget(btnGrid);
}

MainWindow::~MainWindow()
{
    delete ui;
}

