#ifndef FIDOCADWRITER_H
#define FIDOCADWRITER_H

#include <QString>
#include <QList>

class Sheet;
class GraphicsPrimitive;

// Serializes a Sheet's primitives to FidoCadJ (.fcd) text (FIDOSPECS.md 9).
// Writing is the mirror image of FidoCadReader: each primitive's own
// toTokens() plus, where applicable, an FCJ line (arrow/dash/text-flag) and
// TY name/value lines.
namespace FidoCadWriter {

// Returns the full file contents (including the "[FIDOCAD]" header line).
QString write(const Sheet *sheet);

// Writes to disk. Returns false and sets *errorMessage on failure to open
// the file for writing.
bool writeFile(const Sheet *sheet, const QString &filePath, QString *errorMessage = nullptr);

// Serializes just the given primitives (in the given order), with no
// "[FIDOCAD]" header and no document-wide FJC config line - used by Copy/Cut
// to put a FidoCadJ-text fragment on the system clipboard, mirroring the
// reference FidoCadJ editor's CopyPasteActions.copySelected(), which copies
// only the selected elements' own lines rather than a whole document.
QString writeSelection(const QList<GraphicsPrimitive *> &primitives);

} // namespace FidoCadWriter

#endif // FIDOCADWRITER_H
