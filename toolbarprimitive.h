#ifndef TOOLBARPRIMITIVE_H
#define TOOLBARPRIMITIVE_H

#include <QToolBar>

class ToolBarPrimitive : public QToolBar
{

public:
    ToolBarPrimitive(QWidget *parent = nullptr);

    QAction *GetCurrent() { return lastChecked; }

private slots:
    void handleActionTriggered(QAction *action);


private:
    void uncheckOtherActions(QAction *selectedAction);

    QAction *lastChecked = nullptr;

};

#endif // TOOLBARPRIMITIVE_H
