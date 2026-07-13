/*
 * SettingsManager.cpp
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

#include "SettingsManager.h"

SettingsManager& SettingsManager::getInstance()
{
    static SettingsManager instance;  // Instantiate the singleton only once
    return instance;
}

SettingsManager::SettingsManager()
    : m_settings(QSettings::IniFormat, QSettings::UserScope, "eSchema", "eSchema")
{
}

void SettingsManager::saveSetting(const QString& key, const QVariant& value)
{
    m_settings.setValue(key, value);
    emit settingIsChanged();
}

QVariant SettingsManager::loadSetting(const QString& key) const
{
    return m_settings.value(key);
}

void SettingsManager::restoreDefaultSettings()
{
    m_settings.clear(); // Clear existing settings
    m_settings.sync(); // Make sure the changes are saved to disk

    // Default settings

    // GENERAL
    saveSetting("language", "it");
    saveSetting("gui_style", "light");
    saveSetting("stylesheet_path", "");
    saveSetting("lib_path", "");
    saveSetting("macro_icon_size", 32);
    // Whether the library tree also renders a small preview icon per macro
    // row (LibraryManager::icon()) or just its name - independent of the
    // large MacroPreviewWidget panel below the tree, which always shows the
    // currently-clicked macro regardless of this setting.
    saveSetting("macro_tree_icons_enabled", true);
    // Matches the reference FidoCadJ editor's own compiled-in default
    // (Globals.lineWidthDefault) - the single width every schematic
    // primitive (Line/Bezier/Rectangle/Ellipse/Polygon/ComplexCurve) draws
    // with, including inside macro bodies (GraphicsPrimitive::
    // effectiveLineWidth()). PL/PA (PCB track/pad) are unaffected - they
    // carry their own explicit width/size token per FIDOSPECS.
    saveSetting("line_width", 0.5);
    // Extra hit-test padding (scene units) added around a primitive's actual
    // drawn geometry for click/rubber-band selection - see
    // GraphicsPrimitive::selectionTolerance(). Without this, Qt's default
    // shape() (a plain bounding rect) made every primitive selectable
    // anywhere inside its bounding box, including empty space far from what
    // was actually drawn.
    saveSetting("selection_tolerance", 3.0);
    // Matches the reference FidoCadJ editor's own compiled-in default
    // (Globals.diameterConnection) - the connection-dot size every SA
    // primitive draws with, unless overridden per-document by an "FJC C" line
    // (Sheet::connectionDiameter()/PrimitiveConnection::effectiveDiameter()).
    saveSetting("connection_diameter", 2.0);

    // GRID - matches the reference FidoCadJ editor's own compiled-in default
    // (GRID_SIZE = 5). The bundled libraries are drawn on that 5-unit pitch:
    // many macros have pin offsets at odd multiples of 5 (e.g. +15/-10 from
    // the anchor), so a coarser grid/snap step leaves some pins off-grid
    // after placement. One logical unit is 5 mils (0.127 mm), hence a 5-unit
    // grid step spans 0.635 mm.
    saveSetting("grid_step", 5);
    saveSetting("mm_step", 0.635);
    saveSetting("grid_type", 0);
    saveSetting("grid_step_mark", 50);
    saveSetting("grid_line_width", 0.20);
    saveSetting("grid_line_mark_width", 0.20);
    // Whether the grid is actually drawn - toggled via the main toolbar's
    // grid button (actionShowGrid), independent of the grid's own appearance
    // settings above.
    saveSetting("grid_visible", true);

    // SNAP - defaults to the same spacing as the visible grid (grid_step) so
    // clicks visibly snap to grid intersections. Any multiple of 1 keeps
    // coordinates integers, satisfying FidoCadJ's grid-unit requirement
    // (FIDOSPECS.md 3), so this can be changed independently of grid_step.
    saveSetting("snap_enabled", true);
    saveSetting("snap_step", 5);

    // RULERS - whether the top/left rulers are shown around the drawing
    // area (RulerWidget), toggled from the Options dialog.
    saveSetting("rulers_visible", true);

    // AUTOSAVE - periodically writes the open drawing to a recovery sidecar
    // file so a crash doesn't lose unsaved work (see MainWindow::
    // autosaveTick()). Does not touch the document's own save path/undo
    // clean state.
    saveSetting("autosave_enabled", true);
    saveSetting("autosave_interval_minutes", 5);

    // UPDATE CHECK - a single HTTPS request to GitHub's releases API at
    // startup (see UpdateChecker), silent unless a newer version is found.
    // The manual "Check for updates" menu action ignores this setting.
    saveSetting("check_updates_on_startup", true);

    // COLORS
    saveSetting("background_color", QColor("white").name());
    saveSetting("grid_dot_color", QColor("blue").name());
    saveSetting("grid_line_color", QColor(100, 100, 100).name());
    saveSetting("grid_line_mark_color", QColor(100, 100, 100).name());

    m_settings.sync();
}
