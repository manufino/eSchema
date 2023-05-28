#include "toolbarprimitive.h"

ToolBarPrimitive::ToolBarPrimitive(QWidget *parent) : QToolBar(parent)
{
     connect(this, &QToolBar::actionTriggered, this, &ToolBarPrimitive::handleActionTriggered);

     for (QAction *action : actions()) {
         if (action->isChecked()) {
             lastChecked = action;
         }
     }
}

void ToolBarPrimitive::handleActionTriggered(QAction *action)
{
    // Porta in stato unchecked tutte le altre azioni della toolbar
    uncheckOtherActions(action);
    lastChecked = action;
}

void ToolBarPrimitive::uncheckOtherActions(QAction *selectedAction)
{
    // Itera attraverso tutte le azioni della toolbar
    for (QAction *action : actions()) {
        if (action != selectedAction && action->isCheckable()) {
            action->setChecked(false);
        }
    }
}
