#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "sheet.h"
#include "optionsdialog.h"
#include "aboutdialog.h"

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
    void ClickOptionMenu(){
        OptionsDialog *od = new OptionsDialog(this);
        od->show();
    };

    void ClickAboutMenu(){
        AboutDialog *ad = new AboutDialog(this);
        ad->show();
    }

private:
    Ui::MainWindow *ui;
    Sheet *scene;
};
#endif // MAINWINDOW_H
