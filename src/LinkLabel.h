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
