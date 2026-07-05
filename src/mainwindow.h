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
#include "PrimitivePlacementController.h"
#include "SelectionHandleController.h"



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Loads a .fcd file, e.g. one passed on the command line (matching
    // FidoCadJ's "eschema file.fcd" behaviour). Shows an error dialog and
    // returns false if the file can't be read.
    bool openFile(const QString &filePath);

public slots:
    void clickOptionAction();
    void clickAboutAction();
    void clickShortcutsAction();
    void clickLayerManagerAction();
    void clickMirrorAction();
    void clickRotateAction();
    void clickDeleteAction();
    void clickCutAction();
    void clickCopyAction();
    void clickPasteAction();
    void clickDuplicateAction();
    void clickSelectAllAction();
    void clickNewAction();
    void clickOpenAction();
    void clickSaveAction();
    void clickSaveAsAction();

private:
    void setConnections();
    GraphicsPrimitive *firstSelectedPrimitive() const;
    // Selected primitives in stable document order (sheet->primitives()
    // order), not QGraphicsScene::selectedItems()'s unspecified order - Copy
    // needs this so pasting reproduces a predictable, repeatable layout.
    QList<GraphicsPrimitive *> selectedPrimitivesInOrder() const;
    // Parses `text` as FidoCadJ primitives and adds them to the sheet,
    // offset by one grid step and selected - the shared tail end of both
    // Paste and Duplicate (see pasteFromText()'s doc comment for why they
    // share this instead of Duplicate routing through the clipboard).
    void pasteFromText(const QString &text, const QString &undoLabel);
    bool saveToPath(const QString &filePath);
    void setCurrentFilePath(const QString &filePath);
    void updateWindowTitle();

private:
    Ui::MainWindow *ui;
    Sheet *sheetScene;
    DialogOptions *optionDialog;
    DialogAbout *aboutDialog;
    DialogLayerList *layerManager;
    DialogShortcuts *shortcutsDialog;
    LayerToolBarWidget *layerToolBarWidget;
    PrimitivePlacementController *placementController;
    SelectionHandleController *selectionHandleController;
    QString currentFilePath; // empty = new/unsaved drawing
};
#endif // MAINWINDOW_H
