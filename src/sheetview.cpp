#include "SheetView.h"

SheetView::SheetView(QWidget *parent) : QGraphicsView(parent)
{
    /* TODO: per ora lo lascio cosi ..
     * ma sara' da salvare nel file dei settings.*/
    gridEnabled=true;

    rubberBand = new QRubberBand(QRubberBand::Rectangle, this);

    zoomLevel=100;

    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &SheetView::settingChanged);

    loadSettings();

    setMouseTracking(true);
    setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing); //TODO: da valutare
    setTransformationAnchor(QGraphicsView::NoAnchor);
}

void SheetView::setGrid(int size, QColor clr)
{
    gridSize = size;
    dotsGridColor = clr;

    this->update();
}

void SheetView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // se la griglia e' disabilitata
    // non disegno niente
    if(!gridEnabled)
        return;

    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);

    QPen myPen(Qt::NoPen);
    painter->setBrush(QBrush(backgroundColor));
    painter->setPen(myPen);
    painter->drawRect(rect);

    // LINEE+PUNTI o LINEE
    if(gridType == Utils::GridType::LinesAndDots || gridType == Utils::GridType::Lines)
    {
        QVarLengthArray<QLineF, 100> lines;

        for (qreal x = left; x < rect.right(); x += gridSize)
            lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (qreal y = top; y < rect.bottom(); y += gridSize)
            lines.append(QLineF(rect.left(), y, rect.right(), y));

        QVarLengthArray<QLineF, 100> thickLines;

        for (qreal x = left; x < rect.right(); x += gridSize * (gridMarkSize/10))
            thickLines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (qreal y = top; y < rect.bottom(); y += gridSize * (gridMarkSize/10))
            thickLines.append(QLineF(rect.left(), y, rect.right(), y));

        QPen penHLines(lineGridColor, lineGridWidth, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
        painter->setPen(penHLines);
        painter->drawLines(lines.data(), lines.size());

        painter->setPen(QPen(lineThickGridColor, lineThickGridWidth, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter->drawLines(thickLines.data(), thickLines.size());
    }

    // PUNTI o LINEE+PUNTI
    if(gridType == Utils::GridType::Dots || gridType == Utils::GridType::LinesAndDots)
    {
        painter->setPen(dotsGridColor);

        QVector<QPointF> points;
        for (qreal x = left; x < rect.right(); x += gridSize) {
            for (qreal y = top; y < rect.bottom(); y += gridSize) {
                points.append(QPointF(x, y));
            }
        }
        painter->drawPoints(points.data(), points.size());
    }

}


void SheetView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // zoom
        const ViewportAnchor anchor = transformationAnchor();
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        int angle = event->angleDelta().y();
        qreal factor;
        if (angle > 0) {
            factor = 1.1;
        } else {
            factor = 0.9;
        }

        qreal currentScale = transform().m11(); // Ottiene la scala corrente sull'asse x
        qreal newScale = currentScale * factor; // Calcola la nuova scala

        // Limiti del fattore di zoom
        if (newScale < ZOOM_SCALE_MIN) {
            factor = ZOOM_SCALE_MIN / currentScale;
        } else if (newScale > ZOOM_SCALE_MAX) {
            factor = ZOOM_SCALE_MAX / currentScale;
        }

        scale(factor, factor);
        setTransformationAnchor(anchor);

        zoomUpdate();

    } else {
        QGraphicsView::wheelEvent(event);
    }
}


void SheetView::mouseMoveEvent(QMouseEvent *event)
{
    // mappo le coordinate della vista sulla scena
    QPoint origin = mapFromGlobal(QCursor::pos());
    QPointF relativeOrigin = mapToScene(origin);

    if (event->buttons() & Qt::MiddleButton)
    {
        setCursor(Qt::ClosedHandCursor);
        QPointF oldp = mapToScene(m_originX, m_originY);
        QPointF newp = mapToScene(event->pos());
        QPointF translation = newp - oldp;

        translate(translation.x(), translation.y());

        m_originX = event->position().x();
        m_originY = event->position().y();
    } else {
        setCursor(Qt::ArrowCursor);
    }
    if (rubberBand->isVisible())
        rubberBand->setGeometry(QRect(originSelection, event->pos()).normalized());
    emit mouseMoved(relativeOrigin);
    QGraphicsView::mouseMoveEvent(event);
}

void SheetView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        // Store original position.
        m_originX = event->position().x();
        m_originY = event->position().y();
    }
    else
        if(event->button() == Qt::LeftButton)
        {
            originSelection = event->pos();

            //rubberBand->setGeometry(QRect(originSelection, QSize()));
            //rubberBand->show();
        }
    QGraphicsView::mousePressEvent(event);
}

void SheetView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
   // rubberBand->hide();
    //update();
    //scene()->update();
}

void SheetView::loadSettings()
{
    QVariant val = SettingsManager::getInstance().loadSetting("grid_step");
    gridSize = val.toInt();

    val = SettingsManager::getInstance().loadSetting("grid_line_width");
    lineGridWidth = val.toDouble();

    val = SettingsManager::getInstance().loadSetting("grid_line_mark_width");
    lineThickGridWidth = val.toDouble();

    val = SettingsManager::getInstance().loadSetting("grid_line_color");
    lineGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_line_mark_color");
    lineThickGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_dot_color");
    dotsGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("background_color");
    backgroundColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_step_mark");
    gridMarkSize = val.toInt();

    val = SettingsManager::getInstance().loadSetting("grid_type");
    gridType = static_cast<Utils::GridType>(val.toInt());
}

void SheetView::zoomUpdate()
{
    // Calcola la percentuale di zoom
    qreal zoomPercentage = (transform().m11() * 100) / ZOOM_SCALE_MAX;
    zoomLevel = qRound(zoomPercentage);

    emit zoomScaleIsChanged(zoomLevel);
}


void SheetView::settingChanged()
{
    loadSettings();
    update();
}

void SheetView::adjustView()
{
    fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    zoomUpdate();
}


