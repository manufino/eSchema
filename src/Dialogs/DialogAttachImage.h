/*
 * DialogAttachImage.h
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

#ifndef DIALOGATTACHIMAGE_H
#define DIALOGATTACHIMAGE_H

#include <QDialog>
#include <QString>
#include <QPointF>
#include <QImage>

namespace Ui {
class DialogAttachImage;
}

class Sheet;

// Lets the user attach, reposition/resize, or remove the sheet's background
// tracing image (Tools > Tracing image...). Only collects/validates the
// choice - MainWindow::clickAttachImageAction() applies it to the Sheet,
// since only it can trigger the actual repaint.
class DialogAttachImage : public QDialog
{
    Q_OBJECT

public:
    // `sheet` supplies the currently-attached image (if any) to prefill the
    // fields with, so reopening the dialog on an existing tracing image lets
    // the user tweak it rather than starting over.
    explicit DialogAttachImage(const Sheet *sheet, QWidget *parent = nullptr);
    ~DialogAttachImage();

    // True if the user picked "Remove image" - the caller should clear the
    // background image and ignore every other getter below.
    bool imageRemoved() const { return m_imageRemoved; }

    // Empty mimeSubtype()/base64Data() means the user accepted without ever
    // loading an image (e.g. opened the dialog and immediately hit OK on an
    // already-imageless sheet) - the caller should treat that as a no-op.
    QString mimeSubtype() const { return m_mimeSubtype; }
    QString base64Data() const { return m_base64Data; }
    double resolution() const;
    QPointF corner() const;

private slots:
    // Opens the image file picker and loads the chosen file into the fields.
    void browseForImage();
    // Bidirectional with updateHeightFromWidth(): whichever field the user
    // actually edited recomputes the other, with the image's own aspect
    // ratio always kept (only the height label, never height itself, is
    // ever directly editable).
    void updateSizeFromResolution();
    void updateHeightFromWidth();
    // Flags the removal (imageRemoved()) and accepts the dialog.
    void removeImage();

private:
    // Reads/encodes the file into m_mimeSubtype/m_base64Data/m_image and
    // refreshes the size fields.
    void loadImageFile(const QString &path);
    void updateHeightLabel();

    Ui::DialogAttachImage *ui;
    QString m_mimeSubtype;
    QString m_base64Data;
    QImage m_image; // decoded, just for pixel dimensions - not re-saved
    bool m_imageRemoved = false;
};

#endif // DIALOGATTACHIMAGE_H
