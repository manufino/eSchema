#ifndef PRIMITIVEIMAGE_H
#define PRIMITIVEIMAGE_H

#include "GraphicsPrimitive.h"
#include <QPixmap>

// FCD "IM" - an embedded raster image occupying a bounding box, storing the
// original file bytes as base64 so the drawing round-trips on its own
// (FIDOSPECS.md 5.12). No FCJ; name/value TY labels may follow directly.
class PrimitiveImage : public GraphicsPrimitive
{
public:
    explicit PrimitiveImage(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 2; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity) { m_opacity = opacity; }
    int hue() const { return m_hue; }
    void setHue(int hue) { m_hue = hue; }
    QString mimeSubtype() const { return m_mimeSubtype; }
    QString base64Data() const { return m_base64Data; }

    // Decodes and caches the pixmap from `data:image/<mime>;base64,<data>`.
    void setImageData(const QString &mimeSubtype, const QString &base64Data);

    bool isDegenerate() const override;
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }

private:
    QPointF m_p1, m_p2;
    qreal m_opacity = 1.0;
    int m_hue = 0;
    QString m_mimeSubtype;
    QString m_base64Data;
    QPixmap m_pixmap; // decoded once in setImageData(), reused on every paint()
};

#endif // PRIMITIVEIMAGE_H
