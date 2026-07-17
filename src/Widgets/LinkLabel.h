/*
 * LinkLabel.h
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

#ifndef LINKLABEL_H
#define LINKLABEL_H

#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QMouseEvent>

// A QLabel rendered as a hyperlink that opens its URL in the system browser
// when clicked anywhere on the label (not just on the anchor text). Used by
// the About dialog and the update-available notice.
class LinkLabel : public QLabel {
    Q_OBJECT

public:
    explicit LinkLabel(QWidget* parent = nullptr) : QLabel(parent) {
        setOpenExternalLinks(true);
        setCursor(Qt::PointingHandCursor);
    }

    // Shows `text` as the clickable anchor for `url`.
    void setLink(QString text, QUrl url) {
        url_ = url;
        setText(QString("<a href=\"%1\">%2</a>").arg(url.toString(), text));
    }

signals:
    // The label was clicked and the URL handed to QDesktopServices.
    void linkActivated(const QUrl& url);

protected:
    // Any left click on the label counts as following the link.
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            QDesktopServices::openUrl(url_);
            emit linkActivated(url_);
        }

        QLabel::mousePressEvent(event);
    }

private:
    QUrl url_;
};


#endif // LINKLABEL_H
