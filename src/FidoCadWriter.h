#ifndef FIDOCADWRITER_H
#define FIDOCADWRITER_H

#include <QString>

class Sheet;

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

} // namespace FidoCadWriter

#endif // FIDOCADWRITER_H
