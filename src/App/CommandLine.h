/*
 * CommandLine.h
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QString>
#include <QStringList>

class Sheet;

// FidoCadJ-compatible command line: same options, same semantics as the
// reference editor's CommandLineParser/FidoMain pair, so scripts written for
// "java -jar fidocadj.jar ..." port over by just swapping the executable:
//
//   eSchema -n -c 800 600 png out.png drawing.fcd
//   eSchema -n -c r2 svg out.svg drawing.fcd
//   eSchema -n -m -c r2 svg layers.svg drawing.fcd   (one file per layer)
//   eSchema -n -s drawing.fcd                        (print size and exit)
//
// -c/-s run headless (no GUI); adding -n prevents the GUI from opening
// afterwards, exactly like FidoCadJ (where -c alone converts AND then opens
// the editor). Formats: png, jpg, svg, pdf, dxf, fcd.
class CommandLine
{
public:
    CommandLine() = default;

    // Parses the full argument list (args[0] = executable, ignored).
    // Returns -1 to continue starting up, or a >= 0 process exit code when
    // the program should stop right away (-h printed its help, or a parse
    // error was already reported on stderr) - mirroring the System.exit()
    // calls sprinkled through FidoCadJ's own parser.
    int process(const QStringList &arguments);

    // True if any headless work (-c conversion and/or -s size) was requested.
    bool headlessMode() const { return m_headless; }
    // True if -n was given: never open the GUI.
    bool commandLineOnly() const { return m_commandLineOnly; }
    // Runs the requested headless work; returns the process exit code.
    int runHeadlessTasks();

    // The trailing [file] argument, empty if none.
    QString loadFileName() const { return m_loadFile; }
    // The -l language code, empty if not given.
    QString wantedLanguage() const { return m_language; }

private:
    // Prints the -h usage text (mirroring FidoCadJ's own) to stdout.
    void printHelp() const;
    // Runs the -c export of the already-loaded sheet through the shared
    // GraphicExporter; returns the process exit code.
    int doConvert(Sheet *sheet);

    bool m_headless = false;
    bool m_commandLineOnly = false;
    bool m_convert = false;
    bool m_printSize = false;
    bool m_printTime = false;
    bool m_force = false;
    bool m_splitLayers = false;
    bool m_resolutionBased = false;
    double m_resolution = 1.0;
    int m_totX = 0;
    int m_totY = 0;
    QString m_exportFormat;
    QString m_outputFile;
    QString m_loadFile;
    QString m_libDirectory;
    QString m_language;
};

#endif // COMMANDLINE_H
