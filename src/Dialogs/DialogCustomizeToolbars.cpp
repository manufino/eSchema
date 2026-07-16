/*
 * DialogCustomizeToolbars.cpp
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "DialogCustomizeToolbars.h"
#include "ui_DialogCustomizeToolbars.h"

#include <QAction>
#include <QListWidgetItem>

namespace {
const QString SeparatorMarker = QStringLiteral("separator");

// The action text as a list label: no accelerator ampersands, no trailing
// "..." - "Array of copies..." and "Array of copies" are the same command.
QString cleanText(const QAction *action)
{
    QString text = action->text();
    text.remove(QLatin1Char('&'));
    while (text.endsWith(QLatin1Char('.')))
        text.chop(1);
    return text;
}
}

DialogCustomizeToolbars::DialogCustomizeToolbars(const QList<QAction *> &catalog,
                                                 const QList<ToolbarEntry> &toolbars,
                                                 QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogCustomizeToolbars),
      m_catalog(catalog), m_toolbars(toolbars)
{
    ui->setupUi(this);
    setModal(true);

    for (const ToolbarEntry &entry : m_toolbars)
        ui->comboToolbar->addItem(entry.title);

    connect(ui->comboToolbar, &QComboBox::currentIndexChanged, this, [this](int index) {
        storeCurrentList();
        loadToolbar(index);
    });
    connect(ui->btnAdd, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = ui->listAvailable->currentItem();
        if (!item)
            return;
        ui->listCurrent->addItem(makeCommandItem(item->data(Qt::UserRole).toString()));
        delete item;
    });
    connect(ui->btnRemove, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = ui->listCurrent->currentItem();
        if (!item)
            return;
        delete item;
        // The available list is rebuilt (rather than surgically re-inserting
        // one row) so it keeps its stable catalog order.
        storeCurrentList();
        rebuildLists();
    });
    connect(ui->btnUp, &QPushButton::clicked, this, [this]() { moveCurrentRow(-1); });
    connect(ui->btnDown, &QPushButton::clicked, this, [this]() { moveCurrentRow(+1); });
    connect(ui->btnSeparator, &QPushButton::clicked, this, [this]() {
        const int row = ui->listCurrent->currentRow();
        ui->listCurrent->insertItem(row >= 0 ? row + 1 : ui->listCurrent->count(),
                                    makeCommandItem(SeparatorMarker));
    });
    connect(ui->btnReset, &QPushButton::clicked, this, [this]() {
        if (m_currentIndex >= 0) {
            m_toolbars[m_currentIndex].currentLayout = m_toolbars.at(m_currentIndex).defaultLayout;
            loadToolbar(m_currentIndex);
        }
    });
    // Double click as a quicker add/remove, like most customize dialogs.
    connect(ui->listAvailable, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *) { ui->btnAdd->click(); });
    connect(ui->listCurrent, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *) { ui->btnRemove->click(); });

    loadToolbar(0);
}

DialogCustomizeToolbars::~DialogCustomizeToolbars()
{
    delete ui;
}

QListWidgetItem *DialogCustomizeToolbars::makeCommandItem(const QString &objectName) const
{
    if (objectName == SeparatorMarker) {
        auto *item = new QListWidgetItem(QStringLiteral("--- %1 ---").arg(tr("Separator")));
        item->setData(Qt::UserRole, SeparatorMarker);
        return item;
    }
    for (QAction *action : m_catalog) {
        if (action->objectName() == objectName) {
            auto *item = new QListWidgetItem(action->icon(), cleanText(action));
            item->setData(Qt::UserRole, objectName);
            return item;
        }
    }
    return nullptr;
}

void DialogCustomizeToolbars::loadToolbar(int index)
{
    m_currentIndex = index;
    rebuildLists();
}

void DialogCustomizeToolbars::rebuildLists()
{
    ui->listAvailable->clear();
    ui->listCurrent->clear();
    if (m_currentIndex < 0 || m_currentIndex >= m_toolbars.size())
        return;

    const QStringList &layout = m_toolbars.at(m_currentIndex).currentLayout;
    for (const QString &name : layout) {
        if (QListWidgetItem *item = makeCommandItem(name))
            ui->listCurrent->addItem(item);
    }
    // Everything from the catalog not already on this toolbar, in catalog
    // (menu) order.
    for (QAction *action : m_catalog) {
        if (!layout.contains(action->objectName())) {
            if (QListWidgetItem *item = makeCommandItem(action->objectName()))
                ui->listAvailable->addItem(item);
        }
    }
}

void DialogCustomizeToolbars::storeCurrentList()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_toolbars.size())
        return;
    QStringList layout;
    for (int i = 0; i < ui->listCurrent->count(); ++i)
        layout.append(ui->listCurrent->item(i)->data(Qt::UserRole).toString());
    m_toolbars[m_currentIndex].currentLayout = layout;
}

void DialogCustomizeToolbars::moveCurrentRow(int delta)
{
    const int row = ui->listCurrent->currentRow();
    const int target = row + delta;
    if (row < 0 || target < 0 || target >= ui->listCurrent->count())
        return;
    QListWidgetItem *item = ui->listCurrent->takeItem(row);
    ui->listCurrent->insertItem(target, item);
    ui->listCurrent->setCurrentRow(target);
}

QHash<QToolBar *, QStringList> DialogCustomizeToolbars::layouts() const
{
    // The visible list may hold unsaved edits for the current toolbar.
    const_cast<DialogCustomizeToolbars *>(this)->storeCurrentList();
    QHash<QToolBar *, QStringList> result;
    for (const ToolbarEntry &entry : m_toolbars)
        result.insert(entry.toolBar, entry.currentLayout);
    return result;
}
