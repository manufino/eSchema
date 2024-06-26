#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDoubleSpinBox>

#include "SheetView.h"
#include "Sheet.h"
#include "DialogOptions.h"
#include "DialogAbout.h"
#include "LayerComboBox.h"
#include "SettingsManager.h"
#include "Layer.h"
#include "LayerList.h"
#include "LayerToolBarWidget.h"
#include "DialogLayerList.h"
#include "DialogShortcuts.h"
#include "GlobalUtils.h"



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
    void clickOptionAction();
    void clickAboutAction();
    void clickShortcutsAction();
    void clickLayerManagerAction();

private:
    void setConnections();

private:
    Ui::MainWindow *ui;
    Sheet *sheetScene;
    DialogOptions *optionDialog;
    DialogAbout *aboutDialog;
    DialogLayerList *layerManager;
    DialogShortcuts *shortcutsDialog;
    LayerToolBarWidget *layerToolBarWidget;
};
#endif // MAINWINDOW_H
