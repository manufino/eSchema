/*
 * DialogShortcuts.cpp
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

#include "DialogShortcuts.h"
#include "ui_DialogShortcuts.h"
#include "SettingsManager.h"

#include <QAction>
#include <QMessageBox>
#include <QTreeWidgetItem>

namespace {
QString cleanText(const QString &text)
{
    QString cleaned = text;
    cleaned.remove(QLatin1Char('&'));
    return cleaned;
}

QString settingKeyFor(const QAction *action)
{
    return QStringLiteral("shortcut_custom_") + action->objectName();
}
}

DialogShortcuts::DialogShortcuts(const QList<Command> &commands, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogShortcuts),
    m_commands(commands)
{
    ui->setupUi(this);

    // One expanded top-level row per category, one leaf per command, in the
    // catalog's own (menu) order.
    QTreeWidgetItem *categoryItem = nullptr;
    QString currentCategory;
    for (int i = 0; i < m_commands.size(); ++i) {
        const Command &command = m_commands.at(i);
        if (!categoryItem || command.category != currentCategory) {
            categoryItem = new QTreeWidgetItem(ui->treeCommands,
                                               { command.category, QString() });
            currentCategory = command.category;
        }
        auto *item = new QTreeWidgetItem(categoryItem);
        item->setText(0, cleanText(command.action->text()));
        item->setIcon(0, command.action->icon());
        item->setText(1, command.action->shortcut().toString(QKeySequence::NativeText));
        item->setData(0, Qt::UserRole, i);
        m_commandItems.append(item);
    }
    ui->treeCommands->expandAll();
    ui->treeCommands->resizeColumnToContents(0);

    connect(ui->txtShortcutSearch, &QLineEdit::textChanged, this, &DialogShortcuts::filterTree);
    connect(ui->treeCommands, &QTreeWidget::currentItemChanged,
            this, &DialogShortcuts::syncEditorToSelection);
    connect(ui->btnAssign, &QPushButton::clicked, this, &DialogShortcuts::assignCurrent);
    connect(ui->btnClear, &QPushButton::clicked, this, &DialogShortcuts::clearCurrent);
    connect(ui->btnResetAll, &QPushButton::clicked, this, &DialogShortcuts::restoreAllDefaults);
    // Typing a sequence and hitting Enter is the natural gesture - the
    // capture field's editingFinished fires on Enter/focus-out, at which
    // point assigning is what the user meant.
    connect(ui->keyEdit, &QKeySequenceEdit::editingFinished, this, [this]() {
        if (!ui->keyEdit->keySequence().isEmpty())
            assignCurrent();
    });
}

DialogShortcuts::~DialogShortcuts()
{
    delete ui;
}

int DialogShortcuts::commandIndexOf(QTreeWidgetItem *item) const
{
    if (!item || !item->parent())
        return -1;
    return item->data(0, Qt::UserRole).toInt();
}

void DialogShortcuts::filterTree(const QString &needle)
{
    const QString trimmed = needle.trimmed();
    for (int top = 0; top < ui->treeCommands->topLevelItemCount(); ++top) {
        QTreeWidgetItem *categoryItem = ui->treeCommands->topLevelItem(top);
        bool anyVisible = false;
        for (int child = 0; child < categoryItem->childCount(); ++child) {
            QTreeWidgetItem *item = categoryItem->child(child);
            const bool matches = trimmed.isEmpty()
                    || item->text(0).contains(trimmed, Qt::CaseInsensitive)
                    || item->text(1).contains(trimmed, Qt::CaseInsensitive)
                    || categoryItem->text(0).contains(trimmed, Qt::CaseInsensitive);
            item->setHidden(!matches);
            anyVisible = anyVisible || matches;
        }
        categoryItem->setHidden(!anyVisible);
    }
}

void DialogShortcuts::syncEditorToSelection()
{
    const int index = commandIndexOf(ui->treeCommands->currentItem());
    const bool isCommand = index >= 0;
    ui->keyEdit->setEnabled(isCommand);
    ui->btnAssign->setEnabled(isCommand);
    ui->btnClear->setEnabled(isCommand);
    ui->keyEdit->setKeySequence(isCommand ? m_commands.at(index).action->shortcut()
                                          : QKeySequence());
}

void DialogShortcuts::applyShortcut(int index, const QKeySequence &sequence)
{
    const Command &command = m_commands.at(index);
    command.action->setShortcut(sequence);
    SettingsManager::getInstance().saveSetting(
            settingKeyFor(command.action),
            sequence.toString(QKeySequence::PortableText));
    m_commandItems.at(index)->setText(1, sequence.toString(QKeySequence::NativeText));
}

void DialogShortcuts::assignCurrent()
{
    const int index = commandIndexOf(ui->treeCommands->currentItem());
    if (index < 0)
        return;
    const QKeySequence sequence = ui->keyEdit->keySequence();
    if (sequence.isEmpty())
        return;

    // A shortcut can only belong to one command - Qt would otherwise treat
    // every press as ambiguous and deliver it to none of them.
    for (int other = 0; other < m_commands.size(); ++other) {
        if (other == index || m_commands.at(other).action->shortcut() != sequence)
            continue;
        const QMessageBox::StandardButton answer = QMessageBox::question(
                this, tr("Shortcut in use"),
                tr("\"%1\" is already assigned to \"%2\".\nReassign it to the selected command?")
                        .arg(sequence.toString(QKeySequence::NativeText),
                             cleanText(m_commands.at(other).action->text())));
        if (answer != QMessageBox::Yes)
            return;
        applyShortcut(other, QKeySequence());
        break;
    }

    applyShortcut(index, sequence);
}

void DialogShortcuts::clearCurrent()
{
    const int index = commandIndexOf(ui->treeCommands->currentItem());
    if (index < 0)
        return;
    applyShortcut(index, QKeySequence());
    ui->keyEdit->setKeySequence(QKeySequence());
}

void DialogShortcuts::restoreAllDefaults()
{
    const QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Restore defaults"),
            tr("Restore every command's default shortcut?"));
    if (answer != QMessageBox::Yes)
        return;
    for (int i = 0; i < m_commands.size(); ++i)
        applyShortcut(i, m_commands.at(i).defaultShortcut);
    syncEditorToSelection();
}
