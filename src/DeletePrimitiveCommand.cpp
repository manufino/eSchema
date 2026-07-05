#include "DeletePrimitiveCommand.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include <QObject>

DeletePrimitiveCommand::DeletePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive)
    : m_sheet(sheet), m_primitive(primitive)
{
    setText(QObject::tr("Elimina"));
}

DeletePrimitiveCommand::~DeletePrimitiveCommand()
{
    // If redo() was the last thing applied, the primitive is deleted (not in
    // the sheet) - we're the sole owner in that state, so clean it up.
    if (!m_sheet->primitives().contains(m_primitive))
        delete m_primitive;
}

void DeletePrimitiveCommand::undo()
{
    m_sheet->addPrimitive(m_primitive);
}

void DeletePrimitiveCommand::redo()
{
    m_sheet->takePrimitive(m_primitive);
}
