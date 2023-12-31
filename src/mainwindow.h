#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsSceneMouseEvent>
#include <QAction>

#include "sheetview.h"
#include "sheet.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "LayerComboBox.h"
#include "settings_manager.h"



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void ClickOptionMenu();
    void ClickAboutMenu();

private:
    Ui::MainWindow *ui;
    Sheet *scene;
    OptionsDialog *optionDialog;
    AboutDialog *aboutDialog;
};
#endif // MAINWINDOW_H
