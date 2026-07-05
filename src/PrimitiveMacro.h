#ifndef PRIMITIVEMACRO_H
#define PRIMITIVEMACRO_H

#include "GraphicsPrimitive.h"

// FCD "MC" - an instance of a library macro (FIDOSPECS.md 5.10). Per current
// project scope this is round-tripped as raw data only: the macro key,
// position, orientation and mirror flag are preserved exactly, but the macro
// body is not loaded/expanded from any .fcl library, so it paints as a dashed
// placeholder box labeled with its key. Expanding real library macros is a
// separate future task.
class PrimitiveMacro : public GraphicsPrimitive
{
public:
    explicit PrimitiveMacro(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 1; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    // A macro's "rotation"/"mirror" are the discrete orientation/mirror fields
    // FidoCadJ stores, not a geometric transform of its single reference point.
    void mirror(Qt::Orientation axis, const QPointF &pivot) override;
    void rotate90(const QPointF &pivot) override;

    QString macroName() const { return m_macroName; }
    void setMacroName(const QString &name) { m_macroName = name; }
    int orientation() const { return m_orientation; }
    void setOrientation(int orientation) { m_orientation = orientation % 4; }
    bool isMirrored() const { return m_mirrored; }
    void setMirrored(bool mirrored) { m_mirrored = mirrored; }

    bool isDegenerate() const override { return false; } // a macro instance is always meaningful
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }
    // Matches FIDOSPECS.md's worked example (11), where a macro's name/value
    // labels sit further from the anchor than other primitives'.
    QPointF labelOffset(int labelIndex) const override { return QPointF(10, labelIndex == 0 ? 5 : 10); }

private:
    QPointF m_pos;
    int m_orientation = 0; // 0-3, each step = 90 degrees
    bool m_mirrored = false;
    QString m_macroName;
};

#endif // PRIMITIVEMACRO_H
