#ifndef DELETEPRIMITIVECOMMAND_H
#define DELETEPRIMITIVECOMMAND_H

#include <QUndoCommand>

class Sheet;
class GraphicsPrimitive;

// Undo/redo for Edit > Delete. Mirrors CreatePrimitiveCommand: constructed
// while the primitive is still in the sheet, redo() (auto-called once by
// QUndoStack::push()) removes it via takePrimitive() without deleting it, and
// undo() hands it back via addPrimitive(). Whichever state the command isn't
// currently "holding" the primitive in when destroyed determines whether the
// destructor needs to delete it.
class DeletePrimitiveCommand : public QUndoCommand
{
public:
    DeletePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive);
    ~DeletePrimitiveCommand() override;

    void undo() override;
    void redo() override;

private:
    Sheet *m_sheet;
    GraphicsPrimitive *m_primitive;
};

#endif // DELETEPRIMITIVECOMMAND_H
