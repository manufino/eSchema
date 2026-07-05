#include "CreatePrimitiveCommand.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include <QObject>

CreatePrimitiveCommand::CreatePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive)
    : m_sheet(sheet), m_primitive(primitive)
{
    setText(QObject::tr("Crea"));
}

CreatePrimitiveCommand::~CreatePrimitiveCommand()
{
    // If undo() was the last thing applied, the sheet no longer owns the
    // primitive - we do, and must clean it up ourselves.
    if (!m_sheet->primitives().contains(m_primitive))
        delete m_primitive;
}

void CreatePrimitiveCommand::undo()
{
    m_sheet->takePrimitive(m_primitive);
}

void CreatePrimitiveCommand::redo()
{
    m_sheet->addPrimitive(m_primitive);
}
