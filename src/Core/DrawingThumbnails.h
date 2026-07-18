/*
 * DrawingThumbnails.h
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

#ifndef DRAWINGTHUMBNAILS_H
#define DRAWINGTHUMBNAILS_H

#include <QPixmap>
#include <QSize>
#include <QString>

// Small preview renderings of .fcd files on disk, for the recent-files UI
// (the Open Recent menu's hover preview and the welcome card's rows).
// Rendering goes through FidoCadReader::parse() into a hidden reusable
// Sheet - parse(), not read()/readFile(), on purpose: the whole-document
// read path applies the file's FJC document configuration (layer locks,
// widths) to the GLOBAL LayerList, which a mere preview must never touch.
// Colors therefore follow the session's current layer palette.
namespace DrawingThumbnails {

// The drawing at `path` rendered fitted-and-centered on a white `size`
// canvas, or a null pixmap for an unreadable file. Cached per (path, size)
// and invalidated by the file's modification time, so repeated menu hovers
// cost one hash lookup.
QPixmap thumbnail(const QString &path, const QSize &size);

} // namespace DrawingThumbnails

#endif // DRAWINGTHUMBNAILS_H
