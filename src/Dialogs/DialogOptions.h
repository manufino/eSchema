/*
 * DialogOptions.h
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

#ifndef DIALOGOPTIONS_H
#define DIALOGOPTIONS_H

#include <QDialog>
#include <QMessageBox>

#include "SettingsManager.h"

namespace Ui {
class DialogOptions;
}

// The application options, organized as a sidebar of pages (General/
// Interface/Grid/Snap/Drawing) with a search field that filters the sidebar
// down to the pages containing a matching option. The Grid page carries a
// live preview (GridPreviewWidget) fed straight from its own controls.
// "Restore page defaults" resets only the current page's widgets - nothing
// is written to disk until Apply/OK.
class DialogOptions : public QDialog
{
    Q_OBJECT

public:
    explicit DialogOptions(QWidget *parent = nullptr);
    ~DialogOptions();

protected:
    void loadSettings();
    void saveSettings();

public slots:
    void accept();
    void cancel();
    void apply();
    void restore();

private slots:
    // Only the "Stylesheet" (custom) choice in cboxStyle needs a path -
    // toggles txtStylesheetPath/btnOpenStylesheetPath's enabled state to match.
    void updateStylesheetPathEnabled();

private:
    // Pushes the Grid page's current control values into the live preview.
    void syncGridPreview();
    // Hides sidebar entries whose page contains no label/checkbox/group
    // matching `text` (case-insensitive); an empty text shows everything.
    void filterPages(const QString &text);
    // Sets the given page's widgets back to the built-in defaults (without
    // saving - Apply/OK still decides).
    void restorePageDefaults(int pageIndex);

    Ui::DialogOptions *ui;
    // cboxLanguage's index when the dialog was opened (or last applied) - compared
    // against the current index on apply()/accept() to decide whether to warn that
    // a restart is needed, since the UI isn't retranslated live.
    int m_initialLanguageIndex = 0;
};

#endif // DIALOGOPTIONS_H
