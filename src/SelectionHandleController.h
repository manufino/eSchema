#ifndef SELECTIONHANDLECONTROLLER_H
#define SELECTIONHANDLECONTROLLER_H

#include <QObject>
#include <QList>

class Sheet;
class PrimitiveHandleItem;

// Shows resize-grip handles for the currently selected primitive. Scoped to a
// single-primitive selection - resizing multiple primitives at once has no
// single sensible meaning (each primitive type has a different control-point
// count/role), so a multi-selection simply shows no handles.
class SelectionHandleController : public QObject
{
    Q_OBJECT
public:
    explicit SelectionHandleController(Sheet *sheet, QObject *parent = nullptr);

private slots:
    void onSelectionChanged();
    // Re-syncs handle positions from their primitive's live control points.
    // Needed because dragging the primitive body itself (not a handle) also
    // moves its control points (see GraphicsPrimitive::itemChange), which the
    // handles must follow.
    void refreshHandlePositions();

private:
    void clearHandles();

    Sheet *m_sheet;
    QList<PrimitiveHandleItem *> m_handles;
};

#endif // SELECTIONHANDLECONTROLLER_H
