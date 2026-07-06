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

class LinkLabel : public QLabel {
    Q_OBJECT

public:
    explicit LinkLabel(QWidget* parent = nullptr) : QLabel(parent) {
        setOpenExternalLinks(true);
        setCursor(Qt::PointingHandCursor);
    }

    void setLink(QString text, QUrl url) {
        url_ = url;
        setText(QString("<a href=\"%1\">%2</a>").arg(url.toString(), text));
    }

signals:
    void linkActivated(const QUrl& url);

protected:
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
