#ifndef PRIMITIVETEXT_H
#define PRIMITIVETEXT_H

#include "GraphicsPrimitive.h"

// FCD "TY" (advanced) / "TE" (legacy, always upgraded to TY on save) - a
// standalone text primitive (FIDOSPECS.md 5.11). Note: when a TY line instead
// serves as another primitive's attached name/value label (the "dual role of
// TY" in 5.11), it is NOT represented as a PrimitiveText - FidoCadReader writes
// that text straight into the owning primitive's name()/value() instead. This
// class only models genuinely standalone text.
class PrimitiveText : public GraphicsPrimitive
{
public:
    // FCJ-independent style bits, matching the FCD "style" field (5.11).
    enum StyleFlag { Bold = 1, Italic = 2, Mirrored = 4 };

    explicit PrimitiveText(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 1; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    // A text's own reading direction/orientation is a scalar field, not
    // something derived from moving its single anchor point - like
    // PrimitiveMacro, mirror/rotate must additionally update that field
    // (matches FidoCadJ's PrimitiveAdvText.rotatePrimitive/mirrorPrimitive).
    void mirror(Qt::Orientation axis, const QPointF &pivot) override;
    void rotate90(const QPointF &pivot) override;

    QString text() const { return m_text; }
    void setText(const QString &text) { m_text = text; }
    int sizeY() const { return m_sizeY; }
    int sizeX() const { return m_sizeX; }
    void setSize(int sizeY, int sizeX) { m_sizeY = sizeY; m_sizeX = sizeX; }
    int orientationDeg() const { return m_orientationDeg; }
    void setOrientationDeg(int deg) { m_orientationDeg = deg; }
    int styleFlags() const { return m_styleFlags; }
    void setStyleFlags(int flags) { m_styleFlags = flags; }
    QString fontName() const { return m_fontName; }
    void setFontName(const QString &font) { m_fontName = font; }

    bool isDegenerate() const override { return m_text.isEmpty(); }
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }

private:
    QPointF m_pos;
    int m_sizeY = 4, m_sizeX = 3;
    int m_orientationDeg = 0;
    int m_styleFlags = 0;
    QString m_fontName = QStringLiteral("Courier New");
    QString m_text;
};

#endif // PRIMITIVETEXT_H
