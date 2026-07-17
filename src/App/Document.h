/*
 * Document.h
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

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QColor>
#include <QList>
#include <QString>

class Sheet;
class DocumentView;
class PrimitivePlacementController;
class SelectionHandleController;
class QDockWidget;

// One open drawing: the unit a MainWindow tab represents. Bundles what used
// to be scattered single-instance MainWindow members - the Sheet, the file
// path, the placement/selection controllers and the tab's view widget - so
// any number of drawings can be open at once.
//
// The document owns a LIST of sheets, not a single one: today every
// document has exactly one, but the planned "multiple pages per drawing"
// feature only has to add UI around addSheet()/setActiveSheetIndex() - the
// model is already shaped for it.
//
// Layers: LayerList stays the process-wide singleton every widget and the
// file readers/writers already talk to (FidoCadJ itself shares layer
// definitions globally too - loading a file mutates them for the session).
// Per-document isolation is layered on top via snapshots: switching tabs
// captures the outgoing document's layer definitions (captureLayerState())
// and applies the incoming one's (applyLayerState()) onto the same global
// Layer objects, so every document keeps its own names/colors/visibility/
// locks across switches while every existing LayerList call site and signal
// connection keeps working unchanged. The Layer objects themselves are
// never deleted by a switch, so primitives in background documents never
// hold a dangling Layer*.
class Document : public QObject
{
    Q_OBJECT

public:
    explicit Document(QObject *parent = nullptr);
    ~Document() override;

    // The active sheet (the only one, until multi-page lands).
    Sheet *sheet() const { return m_sheets.at(m_activeSheet); }
    const QList<Sheet *> &sheets() const { return m_sheets; }
    int activeSheetIndex() const { return m_activeSheet; }
    void setActiveSheetIndex(int index);
    // Appends a fresh empty sheet (future multi-page support).
    Sheet *addSheet();

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path) { m_filePath = path; }
    // The sequential number an untitled document shows in its tab
    // ("New drawing 1", ...) - assigned by MainWindow::createDocument().
    int untitledNumber() const { return m_untitledNumber; }
    void setUntitledNumber(int number) { m_untitledNumber = number; }
    // The tab caption: the file name, or "New drawing N" while untitled.
    QString displayName() const;

    // True when no sheet has unsaved changes.
    bool isClean() const;

    // --- Per-document layer definitions (see the class comment) ----------
    void captureLayerState();
    void applyLayerState();

    // Owned by this document (QObject children), created and wired by
    // MainWindow::createDocument() - plain public pointers rather than
    // getters/setters, mirroring how MainWindow already treats its own
    // controller members. `dock` is the QDockWidget in the document area
    // that hosts `viewWidget`; it is parented to that area, not the
    // document, so Qt manages its lifetime with the dock hierarchy.
    DocumentView *viewWidget = nullptr;
    QDockWidget *dock = nullptr;
    PrimitivePlacementController *placement = nullptr;
    SelectionHandleController *handles = nullptr;
    // The sidecar autosaveTick() last wrote for this document (runtime
    // only) - lets clearAutosaveFor() delete the right file even after a
    // Save As changed where future sidecars would go.
    QString lastAutosavePath;

private:
    struct LayerState {
        QString name;
        QColor color;
        bool visible = true;
        bool locked = false;
        bool master = false;
    };

    QList<Sheet *> m_sheets;
    int m_activeSheet = 0;
    QString m_filePath; // empty = new/unsaved drawing
    int m_untitledNumber = 0; // shown in the tab while untitled
    QList<LayerState> m_layerStates;
    bool m_hasLayerState = false;
};

#endif // DOCUMENT_H
