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
    m_cache.insert(key, value);
    m_settings.setValue(key, value);
    // Coalesce change notifications: the Options dialog's Apply saves tens
    // of settings back to back, and emitting synchronously per key made
    // every receiver (live-settings refresh, grid reload, ruler sync, ...)
    // re-run tens of times in a row - a visible multi-second freeze. One
    // queued emission per event-loop turn notifies everyone exactly once
    // for the whole batch; receivers read whatever they need back from the
    // already-saved settings, so nothing observes a stale value.
    if (!m_changeSignalPending) {
        m_changeSignalPending = true;
        QMetaObject::invokeMethod(this, [this]() {
            m_changeSignalPending = false;
            emit settingIsChanged();
        }, Qt::QueuedConnection);
    }
}

QVariant SettingsManager::loadSetting(const QString& key) const
{
    // Hot settings (line width, snap radii, colors) are read per paint/
    // mouse move - serve them from the in-memory cache after the first hit.
    const auto cached = m_cache.constFind(key);
    if (cached != m_cache.constEnd())
        return cached.value();
    const QVariant value = m_settings.value(key);
    m_cache.insert(key, value);
    return value;
}

void SettingsManager::ensureDefaults()
{
    // A genuinely fresh install (no ini/registry entries at all yet) has no
    // way to reach restoreDefaultSettings() before the GUI starts reading
    // settings - it's normally only invoked from the Options dialog's
    // "Restore defaults" button. Without it, every color/number read via
    // loadSetting() comes back as an invalid QVariant (an empty string
    // becomes an invalid, effectively black QColor; toInt()/toDouble() on
    // it silently yield 0), producing a solid black canvas and black
    // toolbar icons rather than the intended defaults - this call fills
    // every key in exactly once, on a store that has none, so it never
    // touches settings an existing install has already customized.
    if (m_settings.allKeys().isEmpty())
        restoreDefaultSettings();
}

void SettingsManager::restoreDefaultSettings()
{
    m_settings.clear(); // Clear existing settings
    m_cache.clear();    // ...and the read cache built over them
    m_settings.sync(); // Make sure the changes are saved to disk

    // Default settings

    // GENERAL
    saveSetting("language", "it");
    saveSetting("gui_style", "nord");
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

    // OBJECT SNAP - see ObjectSnap.h/Sheet::snapPosition().
    saveSetting("snap_objects", true);
    saveSetting("object_snap_radius", 12);
    saveSetting("object_snap_endpoints", true);
    saveSetting("object_snap_midpoints", true);
    saveSetting("object_snap_centers", true);
    saveSetting("object_snap_intersections", true);
    saveSetting("snap_indicator_color", QColor(255, 128, 0).name());
    saveSetting("handle_color", QColor(Qt::red).name());
    saveSetting("snap_to_guides", true);
    saveSetting("guide_color", QColor(0, 170, 255).name());

    // BEHAVIOR/INTERFACE extras (see DialogOptions' pages).
    saveSetting("save_backup", false);
    saveSetting("undo_limit", 0);
    saveSetting("startup_reopen_last", false);
    saveSetting("recent_files_max", 10);
    saveSetting("toolbar_icon_size", 25);
    saveSetting("render_antialias", true);
    saveSetting("units_display", 0);
    saveSetting("wheel_zoom_direct", false);
    saveSetting("regular_polygon_sides", 6);
    saveSetting("curve_sampling_step", 5.0);
    saveSetting("boolean_smooth_results", false);
    saveSetting("text_default_font", "Courier New");
    saveSetting("nudge_step_multiplier", 1);
    saveSetting("dimension_text_size", 4);

    // COLORS
    saveSetting("background_color", QColor("white").name());
    saveSetting("grid_dot_color", QColor("blue").name());
    saveSetting("grid_line_color", QColor(100, 100, 100).name());
    saveSetting("grid_line_mark_color", QColor(100, 100, 100).name());

    m_settings.sync();
}
