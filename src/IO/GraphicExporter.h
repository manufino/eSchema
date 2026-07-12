/*
 * GraphicExporter.h
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

#ifndef GRAPHICEXPORTER_H
#define GRAPHICEXPORTER_H

#include <QString>

class Sheet;

// The one shared graphic-export engine, used by both the GUI's export dialog
// (DialogExport) and the FidoCadJ-compatible command line (CommandLine's -c),
// so the two can never drift apart on sizing, background, or option
// semantics - mirroring how every export path in the reference FidoCadJ
// editor funnels through its single ExportGraphic.export().
namespace GraphicExporter {

struct Options {
    // "png", "jpg"/"jpeg" (bitmap), "svg", "pdf" (vector), "dxf".
    QString format;
    // Resolution-based sizing (the image size follows the drawing at
    // `resolution` pixels per logical unit) vs exact-size sizing (an image of
    // exactly totX x totY with the drawing fitted and centered inside,
    // preserving its aspect ratio) - the same two modes as FidoCadJ's
    // ExportGraphic.export()/exportSize().
    bool resolutionBased = true;
    double resolution = 4.0;
    int totX = 800;
    int totY = 600;
    // Bitmap formats only; vector output has no rasterization to smooth.
    bool antialias = true;
    // Renders every layer in black (all formats, DXF included) - matches
    // FidoCadJ's export dialog's own "black & white" option.
    bool blackWhite = false;
    // One output file per layer actually carrying primitives (out_0.ext,
    // out_1.ext, ...), every file sharing the whole drawing's bounding box so
    // the per-layer images overlay perfectly - the CLI's -m. Not supported
    // for DXF (DxfWriter always writes the whole sheet).
    bool splitLayers = false;
};

// "out.svg" + layer 3 -> "out_3.svg", matching the file naming FidoCadJ's -m
// help text documents (test_0.svg, test_1.svg, ...).
QString splitLayerFileName(const QString &fileName, int layer);

// Exports `sheet` to `path` per `options`. Returns false on failure with the
// reason in `error`. The caller is responsible for editor-chrome concerns
// (clearing the on-screen selection first - a headless CLI sheet has none).
bool exportSheet(Sheet *sheet, const QString &path, const Options &options,
                 QString *error);

} // namespace GraphicExporter

#endif // GRAPHICEXPORTER_H
