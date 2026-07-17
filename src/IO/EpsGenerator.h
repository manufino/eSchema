/*
 * EpsGenerator.h
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

#ifndef EPSGENERATOR_H
#define EPSGENERATOR_H

#include <QPaintDevice>
#include <QScopedPointer>
#include <QSize>
#include <QString>

class EpsPaintEngine;

// A QPaintDevice producing Encapsulated PostScript, with the same
// setFileName()/setSize()/setTitle()-then-paint usage as QSvgGenerator - so
// GraphicExporter's EPS branch reads exactly like its SVG one, and the
// existing Sheet::render() path needs no per-primitive export code at all
// (unlike the reference FidoCadJ editor's ExportEPS, which reimplements
// every primitive). Painter calls are translated to PostScript operators;
// text arrives already converted to filled outlines by QPaintEngine's own
// drawTextItem() fallback, so no font embedding is needed. Sizes are in
// PostScript points (72 dpi), matching the PDF export's 1-pt-per-pixel
// convention. Limitations (same class of trade-off as QSvgGenerator):
// clipping is ignored and alpha is flattened - EPS has neither.
class EpsGenerator : public QPaintDevice
{
public:
    EpsGenerator();
    ~EpsGenerator() override;

    // Output path - the file is created when a QPainter begins on this
    // device and finalized when it ends. Set all three before painting.
    void setFileName(const QString &fileName);
    // The EPS BoundingBox, in points.
    void setSize(const QSize &size);
    // Written as the EPS "%%Title:" DSC comment.
    void setTitle(const QString &title);

    // The custom EpsPaintEngine translating QPainter calls to PostScript.
    QPaintEngine *paintEngine() const override;

protected:
    // Reports size/dpi to QPainter using the 72-dpi points convention.
    int metric(PaintDeviceMetric metric) const override;

private:
    QScopedPointer<EpsPaintEngine> m_engine;
    QString m_fileName;
    QSize m_size;
    QString m_title;
};

#endif // EPSGENERATOR_H
