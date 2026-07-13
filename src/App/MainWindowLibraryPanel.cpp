/*
 * MainWindowLibraryPanel.cpp
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

// MainWindow's Libraries dock: building/filtering the per-library QTreeWidget
// pages and the rename/delete context menu for libraries, categories and
// macros. See MainWindow.cpp's top-of-file comment for why this lives in its
// own translation unit instead of inside MainWindow.cpp itself.

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "LibraryManager.h"
#include <QTreeWidget>
#include <QFont>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

void MainWindow::buildLibraryPanel()
{
    while (ui->toolBoxLib->count() > 0) {
        QWidget *page = ui->toolBoxLib->widget(0);
        ui->toolBoxLib->removeItem(0);
        delete page;
    }

    // Falls back to the compiled-in default (matches SettingsManager::
    // restoreDefaultSettings()) rather than the 0 QSettings::value() would
    // return for a settings file saved before this option existed.
    int iconSize = SettingsManager::getInstance().loadSetting("macro_icon_size").toInt();
    if (iconSize <= 0)
        iconSize = 32;
    const QSize iconQSize(iconSize, iconSize);

    // Independent of the always-on MacroPreviewWidget panel below the tree -
    // this only toggles the small per-row icon, for users who find a long
    // library faster to scan as plain text (or just prefer the tree
    // uncluttered now that clicking a row shows a full-size preview anyway).
    const QVariant treeIconsSetting = SettingsManager::getInstance().loadSetting("macro_tree_icons_enabled");
    const bool showTreeIcons = treeIconsSetting.isValid() ? treeIconsSetting.toBool() : true;

    for (const MacroLibrary &library : LibraryManager::getInstance().libraries()) {
        // A tree (library page -> category node -> macro leaf) instead of a
        // flat icon list, so a library's categories - the second grouping
        // level the .fcl format itself defines via "{...}" lines - actually
        // read as sections instead of being just another same-sized cell in
        // an icon grid.
        auto *tree = new QTreeWidget(ui->toolBoxLib);
        tree->setHeaderHidden(true);
        tree->setIconSize(showTreeIcons ? iconQSize : QSize(0, 0));
        tree->setIndentation(12);
        // NOT uniform: category rows (text-only) and macro rows (icon-sized,
        // which the user can set anywhere from 16 to 128px) are genuinely
        // different heights - forcing a single shared height was clipping/
        // overlapping the icons once they got larger than whatever height
        // Qt had cached from the first (short, text-only) row it measured.

        for (const QString &category : library.categoryOrder) {
            auto *categoryItem = new QTreeWidgetItem(tree, QStringList(category));
            categoryItem->setFlags(Qt::ItemIsEnabled);
            QFont categoryFont = categoryItem->font(0);
            categoryFont.setBold(true);
            categoryItem->setFont(0, categoryFont);
            // Collapsed by default: several libraries here have 15-24
            // categories (PCB Footprints, IHRAM, Simboli Elettrotecnica) -
            // expanding all of them at once overflows the panel so badly
            // that only the first one or two categories are ever visible,
            // making a perfectly complete library look empty/broken.

            for (const MacroDescriptor &descriptor : library.macrosByCategory.value(category)) {
                auto *item = new QTreeWidgetItem(categoryItem, QStringList(descriptor.name));
                if (showTreeIcons)
                    item->setIcon(0, LibraryManager::getInstance().icon(descriptor.key, iconSize));
                item->setData(0, Qt::UserRole, descriptor.key);
                item->setToolTip(0, descriptor.name);
                // An explicit size hint, not just the icon size passed to the
                // tree above: some styles' default item delegate doesn't
                // reliably grow the row to fit a large icon on its own. With
                // icons off, let the delegate size the row from the text
                // alone instead of reserving icon-sized space for nothing.
                if (showTreeIcons)
                    item->setSizeHint(0, QSize(iconSize + 24, iconSize + 4));
            }
        }

        connect(tree, &QTreeWidget::itemClicked, this, &MainWindow::clickLibraryMacroItem);

        // Rename/delete via right-click - standard libraries stay read-only
        // (showLibraryContextMenu() itself checks and no-ops), only user
        // libraries get an actual menu.
        tree->setContextMenuPolicy(Qt::CustomContextMenu);
        const QString libraryFilename = library.filename;
        connect(tree, &QTreeWidget::customContextMenuRequested, this, [this, tree, libraryFilename](const QPoint &pos) {
            showLibraryContextMenu(tree, libraryFilename, pos);
        });

        const QString label = !library.displayName.isEmpty() ? library.displayName
                : (library.filename.isEmpty() ? tr("Standard") : library.filename);
        ui->toolBoxLib->addItem(tree, label);
    }
}

void MainWindow::filterLibraryPanel(const QString &text)
{
    for (int page = 0; page < ui->toolBoxLib->count(); ++page) {
        auto *tree = qobject_cast<QTreeWidget *>(ui->toolBoxLib->widget(page));
        if (!tree)
            continue;

        bool pageHasMatch = false;
        for (int c = 0; c < tree->topLevelItemCount(); ++c) {
            QTreeWidgetItem *categoryItem = tree->topLevelItem(c);
            bool categoryHasMatch = false;
            for (int m = 0; m < categoryItem->childCount(); ++m) {
                QTreeWidgetItem *macroItem = categoryItem->child(m);
                const bool matches = text.isEmpty() || macroItem->text(0).contains(text, Qt::CaseInsensitive);
                macroItem->setHidden(!matches);
                categoryHasMatch = categoryHasMatch || matches;
            }
            categoryItem->setHidden(!categoryHasMatch);
            // Categories are collapsed by default (see buildLibraryPanel()) -
            // while actively searching, expand every category with a match
            // so its children are actually visible instead of just present-
            // but-collapsed; collapse back down once the search is cleared.
            categoryItem->setExpanded(!text.isEmpty() && categoryHasMatch);
            pageHasMatch = pageHasMatch || categoryHasMatch;
        }
        if (!text.isEmpty() && pageHasMatch)
            ui->toolBoxLib->setCurrentIndex(page); // jump to the first library with a match
    }
}

void MainWindow::clickLibraryMacroItem(QTreeWidgetItem *item)
{
    const QString key = item->data(0, Qt::UserRole).toString();
    ui->macroPreview->setMacroKey(key);
    ui->lblMacroPreviewName->setText(key.isEmpty() ? QString() : item->text(0));
    if (!key.isEmpty())
        placementController->armMacroPlacement(key);
}

void MainWindow::showLibraryContextMenu(QTreeWidget *tree, const QString &libraryFilename, const QPoint &pos)
{
    if (LibraryManager::getInstance().isStandardLibraryFilename(libraryFilename))
        return;

    QTreeWidgetItem *item = tree->itemAt(pos);
    QMenu menu(this);

    if (!item) {
        QAction *renameAction = menu.addAction(tr("Rename library..."));
        QAction *deleteAction = menu.addAction(tr("Delete library..."));
        QAction *chosen = menu.exec(tree->viewport()->mapToGlobal(pos));
        if (chosen == renameAction)
            renameLibraryInteractive(libraryFilename);
        else if (chosen == deleteAction)
            deleteLibraryInteractive(libraryFilename);
        return;
    }

    if (!item->parent()) {
        const QString category = item->text(0);
        QAction *renameAction = menu.addAction(tr("Rename category..."));
        QAction *deleteAction = menu.addAction(tr("Delete category..."));
        QAction *chosen = menu.exec(tree->viewport()->mapToGlobal(pos));
        if (chosen == renameAction)
            renameCategoryInteractive(libraryFilename, category);
        else if (chosen == deleteAction)
            deleteCategoryInteractive(libraryFilename, category);
        return;
    }

    const QString key = item->data(0, Qt::UserRole).toString();
    QAction *renameAction = menu.addAction(tr("Rename macro..."));
    QAction *deleteAction = menu.addAction(tr("Delete macro..."));
    QAction *chosen = menu.exec(tree->viewport()->mapToGlobal(pos));
    if (chosen == renameAction)
        renameMacroInteractive(key);
    else if (chosen == deleteAction)
        deleteMacroInteractive(key);
}

void MainWindow::renameLibraryInteractive(const QString &filename)
{
    QString currentName = filename;
    for (const MacroLibrary &library : LibraryManager::getInstance().libraries()) {
        if (library.filename.compare(filename, Qt::CaseInsensitive) == 0) {
            currentName = library.displayName.isEmpty() ? library.filename : library.displayName;
            break;
        }
    }

    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Rename library"), tr("Library name:"),
                                                    QLineEdit::Normal, currentName, &ok);
    if (!ok || newName.trimmed().isEmpty())
        return;

    QString errorMessage;
    if (!LibraryManager::getInstance().renameLibrary(filename, newName, &errorMessage))
        QMessageBox::warning(this, tr("Rename library"), errorMessage);
}

void MainWindow::deleteLibraryInteractive(const QString &filename)
{
    const auto answer = QMessageBox::question(
                this, tr("Delete library"),
                tr("Permanently delete this library and all the macros it contains?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    QString errorMessage;
    if (!LibraryManager::getInstance().deleteLibrary(filename, &errorMessage))
        QMessageBox::warning(this, tr("Delete library"), errorMessage);
}

void MainWindow::renameCategoryInteractive(const QString &filename, const QString &category)
{
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Rename category"), tr("Category name:"),
                                                    QLineEdit::Normal, category, &ok);
    if (!ok || newName.trimmed().isEmpty())
        return;

    QString errorMessage;
    if (!LibraryManager::getInstance().renameCategory(filename, category, newName, &errorMessage))
        QMessageBox::warning(this, tr("Rename category"), errorMessage);
}

void MainWindow::deleteCategoryInteractive(const QString &filename, const QString &category)
{
    const auto answer = QMessageBox::question(
                this, tr("Delete category"),
                tr("Delete category \"%1\" and all macros it contains?").arg(category),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    QString errorMessage;
    if (!LibraryManager::getInstance().deleteCategory(filename, category, &errorMessage))
        QMessageBox::warning(this, tr("Delete category"), errorMessage);
}

void MainWindow::renameMacroInteractive(const QString &key)
{
    const MacroDescriptor *descriptor = LibraryManager::getInstance().macro(key);
    if (!descriptor)
        return;

    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Rename macro"), tr("Macro name:"),
                                                    QLineEdit::Normal, descriptor->name, &ok);
    if (!ok || newName.trimmed().isEmpty())
        return;

    QString errorMessage;
    if (!LibraryManager::getInstance().renameMacro(key, newName, &errorMessage))
        QMessageBox::warning(this, tr("Rename macro"), errorMessage);
}

void MainWindow::deleteMacroInteractive(const QString &key)
{
    const MacroDescriptor *descriptor = LibraryManager::getInstance().macro(key);
    const QString name = descriptor ? descriptor->name : key;

    const auto answer = QMessageBox::question(
                this, tr("Delete macro"),
                tr("Permanently delete macro \"%1\"?").arg(name),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    QString errorMessage;
    if (!LibraryManager::getInstance().deleteMacro(key, &errorMessage))
        QMessageBox::warning(this, tr("Delete macro"), errorMessage);
}
