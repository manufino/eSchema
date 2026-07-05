#ifndef FIDOCADREADER_H
#define FIDOCADREADER_H

#include <QString>

class Sheet;

// Parses FidoCadJ (.fcd) text into a Sheet (FIDOSPECS.md 4-7). Per the format's
// robustness contract, malformed or unrecognized lines are skipped rather than
// aborting the parse - reading never "fails" except for an unreadable file.
namespace FidoCadReader {

// Clears `sheet` and populates it from `text`.
void read(const QString &text, Sheet *sheet);

// Reads from disk. Returns false and sets *errorMessage if the file can't be
// opened; parsing itself (via read()) never fails.
bool readFile(const QString &filePath, Sheet *sheet, QString *errorMessage = nullptr);

} // namespace FidoCadReader

#endif // FIDOCADREADER_H
