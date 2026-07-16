/*
 * CommandPalette.cpp
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

#include "CommandPalette.h"

#include <QAction>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QCoreApplication>
#include <QScreen>
#include <QVBoxLayout>

namespace {
// Menu/action texts carry '&' mnemonic markers ("&Edit") that must neither
// be displayed in the palette nor get in the way of matching.
QString cleanText(const QString &text)
{
    QString cleaned = text;
    cleaned.remove(QLatin1Char('&'));
    return cleaned;
}
}

CommandPalette::CommandPalette(QWidget *parent)
    : QDialog(parent, Qt::Popup)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    m_edit = new QLineEdit(this);
    m_edit->setPlaceholderText(tr("Type a command name..."));
    m_edit->setClearButtonEnabled(true);
    m_edit->installEventFilter(this);
    layout->addWidget(m_edit);

    m_list = new QListWidget(this);
    m_list->setUniformItemSizes(true);
    // The click already picks a visible, highlighted row - the usual
    // double-click activation would just feel unresponsive in a palette.
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *) {
        triggerCurrent();
    });
    layout->addWidget(m_list);

    connect(m_edit, &QLineEdit::textChanged, this, &CommandPalette::refilter);
    connect(m_edit, &QLineEdit::returnPressed, this, &CommandPalette::triggerCurrent);
}

void CommandPalette::addCommand(const QString &category, QAction *action)
{
    m_entries.append({ category, action });
}

void CommandPalette::popup(const QPoint &topCenter, const QString &initialText)
{
    resize(520, 420);
    if (initialText.isEmpty())
        refilter(QString());
    else
        m_edit->setText(initialText); // textChanged runs the filter
    // Clamped to the available screen area: anchoring under the menu bar's
    // right-corner search field would otherwise push part of the popup
    // off-screen.
    QPoint topLeft(topCenter.x() - width() / 2, topCenter.y());
    if (QScreen *scr = screen()) {
        const QRect available = scr->availableGeometry();
        topLeft.setX(qBound(available.left(), topLeft.x(), available.right() - width()));
        topLeft.setY(qBound(available.top(), topLeft.y(), available.bottom() - height()));
    }
    move(topLeft);
    m_edit->setFocus();
    exec();
}

bool CommandPalette::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit && event->type() == QEvent::KeyPress) {
        const int key = static_cast<QKeyEvent *>(event)->key();
        if (key == Qt::Key_Up || key == Qt::Key_Down
                || key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
            QCoreApplication::sendEvent(m_list, event);
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void CommandPalette::refilter(const QString &needle)
{
    m_list->clear();
    const QString trimmed = needle.trimmed();
    for (const Entry &entry : m_entries) {
        const QString name = cleanText(entry.action->text());
        // Both the command's own name and its category match, so "edit mir"
        // style narrowing works the way command palettes usually do: every
        // whitespace-separated term must appear somewhere.
        const QString haystack = entry.category + QLatin1Char(' ') + name;
        bool matches = true;
        const QStringList terms = trimmed.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString &term : terms) {
            if (!haystack.contains(term, Qt::CaseInsensitive)) {
                matches = false;
                break;
            }
        }
        if (!matches)
            continue;

        QString label = name + QStringLiteral("  —  ") + entry.category;
        const QKeySequence shortcut = entry.action->shortcut();
        if (!shortcut.isEmpty())
            label += QStringLiteral("  (") + shortcut.toString(QKeySequence::NativeText)
                    + QLatin1Char(')');
        auto *item = new QListWidgetItem(entry.action->icon(), label, m_list);
        item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void *>(entry.action)));
        if (!entry.action->isEnabled())
            item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
    }

    // Preselect the first runnable row so a plain Enter always does
    // something sensible.
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->flags() & Qt::ItemIsEnabled) {
            m_list->setCurrentRow(i);
            break;
        }
    }
}

void CommandPalette::triggerCurrent()
{
    QListWidgetItem *item = m_list->currentItem();
    if (!item || !(item->flags() & Qt::ItemIsEnabled))
        return;
    auto *action = static_cast<QAction *>(item->data(Qt::UserRole).value<void *>());
    accept();
    if (action)
        action->trigger();
}
