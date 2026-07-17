/*
 * MacroPreviewWidget.h
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

#ifndef MACROPREVIEWWIDGET_H
#define MACROPREVIEWWIDGET_H

#include <QFrame>
#include <QString>

// A large, live rendering of one library macro - the Libraries dock's
// "click a tree item, see it big" preview, complementing the small per-row
// icons buildLibraryPanel() already sets via LibraryManager::icon().
class MacroPreviewWidget : public QFrame
{
    Q_OBJECT
public:
    explicit MacroPreviewWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;

public slots:
    // Empty key clears the preview - used when a category row (not a macro
    // leaf) is clicked in the library tree.
    void setMacroKey(const QString &key);

protected:
    // Renders the macro's expanded body (LibraryManager::expandedBody())
    // fitted and centered in the frame, or nothing for an empty key.
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_key;
};

#endif // MACROPREVIEWWIDGET_H
