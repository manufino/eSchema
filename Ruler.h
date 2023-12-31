#ifndef RULER_H
#define RULER_H

#include <QWidget>
#include <QAbstractScrollArea>
#include <QPainter>
#include <QScrollBar>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QScreen>
#include <QApplication>

class Ruler: public QWidget
{
    Q_OBJECT
public:
    Ruler(QAbstractScrollArea* parent=nullptr): QWidget(parent),
        offset(0)
    {
        setFixedSize(40, parent->height());
        move(0, 40);
        connect(parent->verticalScrollBar(), &QScrollBar::valueChanged, this, &Ruler::setOffset);
    }
    virtual void paintEvent(QPaintEvent* event)
    {
        QPainter painter(this);
        painter.translate(0, -offset);
        int const heightMM = height() * toMM();
        painter.setFont(font());
        QFontMetrics fm(font());
        for (int position = 0; position < heightMM; ++position)
        {
            int const positionInPix = int(position / toMM());
            if (position % 10 == 0)
            {
                if (position != 0)
                {
                    QString const txt = QString::number(position);
                    QRect txtRect = fm.boundingRect(txt).translated(0, positionInPix);
                    txtRect.translate(0, txtRect.height()/2);
                    painter.drawText(txtRect, txt);
                }
                painter.drawLine(width() - 15, positionInPix, width(), positionInPix);
            }
            else {
                painter.drawLine(width() - 10, positionInPix, width(), positionInPix);
            }
        }
    }

    virtual void resizeEvent(QResizeEvent* event)
    {

        int const maximumMM = event->size().height() * toMM();
        QFontMetrics fm(font());
        int w = fm.horizontalAdvance(QString::number(maximumMM)) + 20;
        if (w != event->size().width())
        {
            QSize const newSize(w, event->size().height());
            sizeChanged(newSize);
            return setFixedSize(newSize);
        }
        return QWidget::resizeEvent(event);
    }

    void setOffset(int value)
    {
        offset = value;
        update();
    }
signals:
    void sizeChanged(QSize const&);
private:
    int offset;

    static qreal toMM()
        {
            // Ottieni il display primario
            QScreen *primaryScreen = QGuiApplication::primaryScreen();

            // Verifica se il display primario è valido
            if (primaryScreen) {
                // Ottieni la densità dei pixel verticale del display primario
                qreal dpiY = primaryScreen->logicalDotsPerInchY();

                // Calcola la conversione da pixel a millimetri
                return 25.4 / dpiY;
            } else {
                // Gestisci il caso in cui il display primario non è disponibile
                // Restituisci un valore predefinito o lancia un'eccezione, a seconda delle tue esigenze
                return 0.0;  // Modifica questo valore di default a seconda delle tue esigenze
            }
        }
};

#endif // RULER_H
