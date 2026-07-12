/*
 * CommandLine.cpp
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

#include "CommandLine.h"
#include "Sheet.h"
#include "LayerList.h"
#include "LibraryManager.h"
#include "GraphicsPrimitive.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "DxfWriter.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QLocale>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSet>
#include <QSvgGenerator>
#include <QtMath>

#include <algorithm>
#include <cstdio>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

// eSchema.exe is linked as a Windows GUI-subsystem binary, so it starts with
// no console attached and printf goes nowhere. Re-attaching to the parent
// process' console (the cmd/PowerShell the user typed the command into)
// makes stdout/stderr land there like a normal command line tool. A stream
// that is already valid was redirected by the caller (a pipe or a file) and
// must be left alone - rewiring it to CONOUT$ would steal the output away
// from the redirection. A no-op when there is no parent console
// (double-click launch) or off Windows.
void attachParentConsole()
{
#ifdef Q_OS_WIN
    auto streamNeedsConsole = [](DWORD stdHandleId) {
        const HANDLE handle = GetStdHandle(stdHandleId);
        return handle == nullptr || handle == INVALID_HANDLE_VALUE;
    };
    const bool wireStdout = streamNeedsConsole(STD_OUTPUT_HANDLE);
    const bool wireStderr = streamNeedsConsole(STD_ERROR_HANDLE);
    if ((wireStdout || wireStderr) && AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (wireStdout) {
            freopen("CONOUT$", "w", stdout);
            // The shell already printed its prompt before we attached -
            // start on a fresh line instead of appending to the prompt.
            fputs("\n", stdout);
        }
        if (wireStderr)
            freopen("CONOUT$", "w", stderr);
    }
#endif
}

// True when the output file's extension is coherent with the export format
// (FidoCadJ's Globals.checkExtension: it refuses "-c ... png out.gif" unless
// -f is given). jpg/jpeg are the one interchangeable pair.
bool extensionCoherent(const QString &fileName, const QString &format)
{
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (format == QLatin1String("jpg") || format == QLatin1String("jpeg"))
        return suffix == QLatin1String("jpg") || suffix == QLatin1String("jpeg");
    return suffix == format;
}

// "out.svg" + layer 3 -> "out_3.svg", matching the file naming FidoCadJ's -m
// help text documents (test_0.svg, test_1.svg, ...).
QString splitLayerFileName(const QString &fileName, int layer)
{
    const QFileInfo info(fileName);
    const QString suffix = info.suffix();
    QString name = info.completeBaseName() + QLatin1Char('_') + QString::number(layer);
    if (!suffix.isEmpty())
        name += QLatin1Char('.') + suffix;
    return info.dir().filePath(name);
}

// Fits `source` inside `target` preserving the aspect ratio and centering it,
// same logic as MainWindow::renderForPrint() - stretching a schematic unevenly
// would distort right angles and text.
QRectF fitCentered(const QRectF &source, const QRectF &target)
{
    const qreal scale = qMin(target.width() / source.width(),
                              target.height() / source.height());
    const QSizeF scaledSize = source.size() * scale;
    return QRectF(target.left() + (target.width() - scaledSize.width()) / 2.0,
                  target.top() + (target.height() - scaledSize.height()) / 2.0,
                  scaledSize.width(), scaledSize.height());
}

} // namespace

int CommandLine::process(const QStringList &arguments)
{
    const QStringList args = arguments.mid(1); // drop the executable path
    if (!args.isEmpty())
        attachParentConsole();

    bool nextIsLibDir = false;
    bool fileLoaded = false;

    for (int i = 0; i < args.size(); ++i) {
        const QString arg = args.at(i);
        if (arg.startsWith(QLatin1Char('-'))) {
            if (arg == QLatin1String("-n")) {
                // Command line only: never open the GUI.
                m_commandLineOnly = true;
            } else if (arg == QLatin1String("-d")) {
                nextIsLibDir = true;
            } else if (arg == QLatin1String("-f")) {
                m_force = true;
            } else if (arg == QLatin1String("-m")) {
                m_splitLayers = true;
            } else if (arg == QLatin1String("-c")) {
                // -c [r<res> | sx sy] format outfile
                bool ok = false;
                if (++i >= args.size()) {
                    fputs("Unable to read the parameters given to -c\n", stderr);
                    return 1;
                }
                if (args.at(i).startsWith(QLatin1Char('r'))) {
                    m_resolution = args.at(i).mid(1).toDouble(&ok);
                    m_resolutionBased = true;
                    if (!ok || m_resolution <= 0) {
                        fputs("Resolution should be a positive real number\n", stderr);
                        return 1;
                    }
                } else {
                    m_totX = args.at(i).toInt(&ok);
                    if (ok && ++i < args.size())
                        m_totY = args.at(i).toInt(&ok);
                    if (!ok || m_totX <= 0 || m_totY <= 0) {
                        fputs("Unable to read the parameters given to -c\n", stderr);
                        return 1;
                    }
                }
                if (i + 2 >= args.size()) {
                    fputs("Unable to read the parameters given to -c\n", stderr);
                    return 1;
                }
                m_exportFormat = args.at(++i).toLower();
                m_outputFile = args.at(++i);
                m_convert = true;
                m_headless = true;
            } else if (arg == QLatin1String("-h") || arg == QLatin1String("--help")) {
                printHelp();
                return 0;
            } else if (arg == QLatin1String("-s")) {
                m_headless = true;
                m_printSize = true;
            } else if (arg == QLatin1String("-t")) {
                m_printTime = true;
            } else if (arg == QLatin1String("-p")) {
                // FidoCadJ's "strip Java platform optimizations" switch -
                // accepted for script compatibility, nothing to strip here.
            } else if (arg == QLatin1String("-k")) {
                fprintf(stdout, "Detected locale: %s\n",
                        qPrintable(QLocale::system().name().section(QLatin1Char('_'), 0, 0)));
            } else if (arg.startsWith(QLatin1String("-l"))) {
                // "-l xx" or "-lxx", like FidoCadJ.
                if (arg.length() == 2) {
                    if (i == args.size() - 1 || args.at(i + 1).startsWith(QLatin1Char('-'))) {
                        fputs("-l option requires a locale language code.\n", stderr);
                        return 1;
                    }
                    m_language = args.at(++i);
                } else {
                    m_language = arg.mid(2);
                }
            } else {
                fprintf(stderr, "Unrecognized option: %s\n", qPrintable(arg));
                printHelp();
                return 1;
            }
        } else if (nextIsLibDir) {
            m_libDirectory = arg;
            nextIsLibDir = false;
            fprintf(stdout, "Changed the library directory: %s\n", qPrintable(arg));
        } else {
            if (fileLoaded)
                fputs("Only one file can be specified in the command line\n", stderr);
            m_loadFile = arg;
            fileLoaded = true;
        }
    }
    return -1;
}

int CommandLine::runHeadlessTasks()
{
    QElapsedTimer timer;
    timer.start();

    if (m_loadFile.isEmpty()) {
        fputs("You should specify a FidoCadJ file to read\n", stderr);
        return 1;
    }

    // Same environment the GUI sets up before opening a file: the 16
    // standard layers (FidoCadReader maps layer indices onto them) and the
    // macro libraries ("MC" lines resolve against LibraryManager).
    LayerList::getInstance().createDefaultLayers();
    if (!m_libDirectory.isEmpty())
        LibraryManager::getInstance().setExternalLibraryDirectory(m_libDirectory);
    LibraryManager::getInstance().loadLibraries();

    Sheet sheet;
    QString error;
    if (!FidoCadReader::readFile(m_loadFile, &sheet, &error)) {
        fprintf(stderr, "Unable to process: %s\n", qPrintable(error));
        return 1;
    }

    int result = 0;
    if (m_convert)
        result = doConvert(&sheet);

    if (m_printSize) {
        const QRectF bounds = sheet.itemsBoundingRect();
        fprintf(stdout, "%d %d\n", qCeil(bounds.width()), qCeil(bounds.height()));
    }

    if (m_printTime)
        fprintf(stdout, "Elapsed time: %lld ms.\n",
                static_cast<long long>(timer.elapsed()));

    fflush(stdout);
    return result;
}

int CommandLine::doConvert(Sheet *sheet)
{
    static const QStringList supportedFormats = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("svg"), QStringLiteral("pdf"), QStringLiteral("dxf"),
        QStringLiteral("fcd")
    };
    if (!supportedFormats.contains(m_exportFormat)) {
        fprintf(stderr, "Unsupported format: %s (use png|jpg|svg|pdf|dxf|fcd)\n",
                qPrintable(m_exportFormat));
        return 1;
    }

    if (!m_force && !extensionCoherent(m_outputFile, m_exportFormat)) {
        fputs("File extension is not coherent with the export output format!"
              " Use -f to skip this test.\n", stderr);
        return 1;
    }

    const bool graphicFormat = m_exportFormat != QLatin1String("fcd");
    if (graphicFormat && sheet->itemsBoundingRect().isEmpty()) {
        fputs("The drawing is empty, nothing to export.\n", stderr);
        return 1;
    }

    QString error;
    if (m_splitLayers && graphicFormat) {
        // One output file per layer actually carrying primitives, named
        // out_0.ext, out_1.ext, ... - every file shares the whole drawing's
        // bounding box, so the per-layer images overlay perfectly.
        const QList<GraphicsPrimitive *> &primitives = sheet->primitives();
        QSet<int> usedSet;
        for (GraphicsPrimitive *primitive : primitives)
            usedSet.insert(primitive->layerIndex());
        QList<int> usedLayers(usedSet.begin(), usedSet.end());
        std::sort(usedLayers.begin(), usedLayers.end());

        bool ok = true;
        for (int layer : std::as_const(usedLayers)) {
            for (GraphicsPrimitive *primitive : primitives)
                primitive->setVisible(primitive->layerIndex() == layer);
            if (!exportOne(sheet, splitLayerFileName(m_outputFile, layer),
                           m_exportFormat, &error)) {
                ok = false;
                break;
            }
        }
        for (GraphicsPrimitive *primitive : primitives)
            primitive->setVisible(true);
        if (!ok) {
            fprintf(stderr, "Export error: %s\n", qPrintable(error));
            return 1;
        }
    } else if (!exportOne(sheet, m_outputFile, m_exportFormat, &error)) {
        fprintf(stderr, "Export error: %s\n", qPrintable(error));
        return 1;
    }

    fputs("Export completed\n", stdout);
    return 0;
}

bool CommandLine::exportOne(Sheet *sheet, const QString &path,
                            const QString &format, QString *error)
{
    if (format == QLatin1String("fcd"))
        return FidoCadWriter::writeFile(sheet, path, error);
    if (format == QLatin1String("dxf"))
        return DxfWriter::writeFile(sheet, path, error);

    const QRectF source = sheet->itemsBoundingRect();

    // r<res> means res pixels per logical unit (the image size follows the
    // drawing); sx sy means an image of exactly that size with the drawing
    // fitted and centered inside, preserving its aspect ratio - the same two
    // sizing modes as FidoCadJ's ExportGraphic.export()/exportSize().
    QSizeF targetSize;
    if (m_resolutionBased)
        targetSize = source.size() * m_resolution;
    else
        targetSize = QSizeF(m_totX, m_totY);
    targetSize = targetSize.expandedTo(QSizeF(1, 1));

    const QRectF target(QPointF(0, 0), targetSize);
    const QRectF drawTarget = fitCentered(source, target);

    if (format == QLatin1String("png") || format == QLatin1String("jpg")
            || format == QLatin1String("jpeg")) {
        // White background (not transparent), like the GUI's own PNG export:
        // the scene itself paints none.
        QImage image(qCeil(targetSize.width()), qCeil(targetSize.height()),
                     QImage::Format_ARGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        sheet->render(&painter, drawTarget, source);
        painter.end();
        // Explicit format: with -f the extension may not match it.
        const bool saved = image.save(path,
                format == QLatin1String("png") ? "PNG" : "JPG");
        if (!saved && error)
            *error = QStringLiteral("unable to save the image to \"%1\"").arg(path);
        return saved;
    }

    if (format == QLatin1String("svg")) {
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setSize(targetSize.toSize());
        generator.setViewBox(target);
        generator.setTitle(QStringLiteral("eSchema drawing"));
        QPainter painter;
        if (!painter.begin(&generator)) {
            if (error)
                *error = QStringLiteral("unable to write \"%1\"").arg(path);
            return false;
        }
        sheet->render(&painter, drawTarget, source);
        painter.end();
        return true;
    }

    // pdf: a single page cut exactly to the requested size (1 pt per pixel
    // at 72 dpi), so both sizing modes are honored - unlike the GUI export,
    // which fits the drawing onto a fixed printer page.
    QPdfWriter writer(path);
    writer.setResolution(72);
    writer.setPageSize(QPageSize(targetSize, QPageSize::Point, QString(),
                                 QPageSize::ExactMatch));
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    QPainter painter;
    if (!painter.begin(&writer)) {
        if (error)
            *error = QStringLiteral("unable to write \"%1\"").arg(path);
        return false;
    }
    painter.setRenderHint(QPainter::Antialiasing);
    sheet->render(&painter, drawTarget, source);
    painter.end();
    return true;
}

void CommandLine::printHelp() const
{
    fputs("\nThis is eSchema, version " APP_VERSION ".\n"
        "FidoCadJ-compatible command line.\n\n"

        "Use: eSchema [-options] [file]\n"
        "where options include:\n\n"

        " -n     Do not start the graphical user interface (headless mode)\n\n"

        " -d     Set the extern library directory\n"
        "        Usage: -d dir\n"
        "        where 'dir' is the path of the directory you want to use.\n\n"

        " -c     Convert the given file to a graphical format.\n"
        "        Usage: -c sx sy png|jpg|svg|pdf|dxf|fcd outfile\n"
        "        If you use this command line option, you *must* specify an eSchema/\n"
        "        FidoCadJ (.fcd) file to convert.\n"
        "        An alternative is to specify the resolution in pixels per logical unit\n"
        "        by preceding it by the letter 'r' (without spaces), instead of giving\n"
        "        sx and sy.\n"
        "        NOTE: the correctness of the file extension is checked, unless the -f\n"
        "        option is specified.\n\n"

        " -m     if a file export is done, split the layers and write one file for\n"
        "        each layer. The file name will be obtained by appending _ followed\n"
        "        by the layer number to the specified file name. For example, the\n"
        "        following command will create files test_0.svg, test_1.svg ... from\n"
        "        the drawing contained in test.fcd:\n\n"
        "           eSchema -n -m -c r2 svg test.svg test.fcd\n\n"

        " -s     Print the size of the specified file in logical units.\n\n"

        " -h     Print this help and exit.\n\n"

        " -t     Print the time used by eSchema for the specified operation.\n\n"

        " -l     Force eSchema to use a certain locale (the code might follow\n"
        "        immediately or be separated by an optional space).\n\n"

        " -k     Show the current locale.\n\n"

        " -f     Force eSchema to skip some sanity tests on the input data.\n\n"

        " [file] The optional (except if you use the -c or -s options) eSchema file to\n"
        "        load at startup time.\n\n"

        "Example: load and convert a drawing to a 800x600 pixel png file\n"
        "        without using the GUI.\n"
        "  eSchema -n -c 800 600 png out1.png test1.fcd\n\n"
        "Example: convert a drawing to a png file in headless mode.\n"
        "        Each logical unit will be converted in 2 pixels on the image.\n"
        "  eSchema -n -c r2 png out2.png test2.fcd\n\n", stdout);
    fflush(stdout);
}
