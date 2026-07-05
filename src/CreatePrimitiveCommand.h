#ifndef CREATEPRIMITIVECOMMAND_H
#define CREATEPRIMITIVECOMMAND_H

#include <QUndoCommand>

class Sheet;
class GraphicsPrimitive;

// Undo/redo for finishing a drawing-tool placement. The primitive is already
// added to the sheet by the time this command is constructed and pushed (see
// PrimitivePlacementController::finishPlacement()) - Sheet::addPrimitive() is
// idempotent, so QUndoStack::push()'s automatic redo() call is a harmless
// no-op the first time.
//
// Ownership toggles between the Sheet and this command: undo() hands the
// primitive to takePrimitive() (removes it from the scene without deleting
// it), so this command becomes the sole owner until either redo() gives it
// back or this command itself is destroyed while still holding it.
class CreatePrimitiveCommand : public QUndoCommand
{
public:
    CreatePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive);
    ~CreatePrimitiveCommand() override;

    void undo() override;
    void redo() override;

private:
    Sheet *m_sheet;
    GraphicsPrimitive *m_primitive;
};

#endif // CREATEPRIMITIVECOMMAND_H
