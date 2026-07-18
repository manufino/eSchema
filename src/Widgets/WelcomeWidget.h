/*
 * WelcomeWidget.h
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

#ifndef WELCOMEWIDGET_H
#define WELCOMEWIDGET_H

#include <QWidget>

class QVBoxLayout;

// The welcome card shown centered over a brand-new empty drawing: app name
// and version, the recent files as clickable links (openFileRequested()),
// a few quick tips, and a "show on new drawings" checkbox persisting the
// "welcome_enabled" setting. It is an overlay child of the DocumentView
// (kept centered by watching the parent's resizes), so it never interferes
// with the layout - and it gets out of the way on its own: clicking
// anywhere on it, picking a drawing tool, loading a file, or the first
// primitive appearing all dismiss() it (see MainWindow::createDocument()'s
// wiring).
class WelcomeWidget : public QWidget
{
    Q_OBJECT

public:
    // `parent` is the DocumentView the card floats over; the card centers
    // itself on it now and on every parent resize.
    explicit WelcomeWidget(QWidget *parent);

    // Hides the card for good (for this document) - safe to call twice.
    void dismiss();

signals:
    // A recent-file link was clicked; MainWindow routes this to openFile().
    void openFileRequested(const QString &path);

protected:
    // The rounded, slightly translucent card body, painted from the live
    // palette so every theme renders it correctly.
    void paintEvent(QPaintEvent *event) override;
    // A click anywhere on the card that no child link/control consumed
    // dismisses it - so the canvas underneath is one click away.
    void mousePressEvent(QMouseEvent *event) override;
    // A drag wandering over the card (e.g. a macro from the Libraries
    // panel) dismisses it and lets the drag continue onto the now-exposed
    // canvas underneath.
    void dragEnterEvent(QDragEnterEvent *event) override;
    // Watches the parent DocumentView's resizes to stay centered.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // Fills the recent-files section from the "recent_files" setting (up
    // to 5 entries), or a "no recent files" note.
    void buildRecentFiles(QVBoxLayout *layout);
    // Re-centers the card on the parent widget.
    void recenter();
};

#endif // WELCOMEWIDGET_H
