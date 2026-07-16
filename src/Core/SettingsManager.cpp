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
    // A fresh install (no ini/registry entries yet) has no way to reach
    // restoreDefaultSettings() before the GUI starts reading settings -
    // it's normally only invoked from the Options dialog's "Restore
    // defaults" button. Without this, every unwritten key read via
    // loadSetting() comes back as an invalid QVariant (an empty string
    // becomes an invalid, effectively black QColor; toInt()/toDouble() on
    // it silently yield 0), producing a solid black canvas, black toolbar
    // icons and a divide-by-zero crash rather than the intended defaults.
    // Fills only keys that are missing, so it is safe to run on every
    // startup: existing installs keep all their customizations, partial
    // stores (e.g. one aborted by a first-launch crash) are healed, and
    // settings introduced by an upgrade get their defaults filled in.
    writeMissingDefaults();
    m_settings.sync();
}

void SettingsManager::restoreDefaultSettings()
{
    m_settings.clear(); // Clear existing settings
    m_cache.clear();    // ...and the read cache built over them
    m_settings.sync(); // Make sure the changes are saved to disk

    // The store was just cleared, so this writes every default.
    writeMissingDefaults();

    m_settings.sync();
}

void SettingsManager::writeMissingDefaults()
{
    const auto def = [this](const QString &key, const QVariant &value) {
        if (!m_settings.contains(key))
            saveSetting(key, value);
    };

    // Default settings

    // GENERAL - "language" is deliberately absent: main.cpp's
    // resolveLanguageCode() derives it from the OS locale when unset, and
    // writing one here would suppress that detection on first launch.
    def("gui_style", "nord");
    def("stylesheet_path", "");
    def("lib_path", "");
    def("macro_icon_size", 32);
    // Whether the library tree also renders a small preview icon per macro
    // row (LibraryManager::icon()) or just its name - independent of the
    // large MacroPreviewWidget panel below the tree, which always shows the
    // currently-clicked macro regardless of this setting.
    def("macro_tree_icons_enabled", true);
    // Matches the reference FidoCadJ editor's own compiled-in default
    // (Globals.lineWidthDefault) - the single width every schematic
    // primitive (Line/Bezier/Rectangle/Ellipse/Polygon/ComplexCurve) draws
    // with, including inside macro bodies (GraphicsPrimitive::
    // effectiveLineWidth()). PL/PA (PCB track/pad) are unaffected - they
    // carry their own explicit width/size token per FIDOSPECS.
    def("line_width", 0.5);
    // Extra hit-test padding (scene units) added around a primitive's actual
    // drawn geometry for click/rubber-band selection - see
    // GraphicsPrimitive::selectionTolerance(). Without this, Qt's default
    // shape() (a plain bounding rect) made every primitive selectable
    // anywhere inside its bounding box, including empty space far from what
    // was actually drawn.
    def("selection_tolerance", 3.0);
    // Matches the reference FidoCadJ editor's own compiled-in default
    // (Globals.diameterConnection) - the connection-dot size every SA
    // primitive draws with, unless overridden per-document by an "FJC C" line
    // (Sheet::connectionDiameter()/PrimitiveConnection::effectiveDiameter()).
    def("connection_diameter", 2.0);

    // GRID - matches the reference FidoCadJ editor's own compiled-in default
    // (GRID_SIZE = 5). The bundled libraries are drawn on that 5-unit pitch:
    // many macros have pin offsets at odd multiples of 5 (e.g. +15/-10 from
    // the anchor), so a coarser grid/snap step leaves some pins off-grid
    // after placement. One logical unit is 5 mils (0.127 mm), hence a 5-unit
    // grid step spans 0.635 mm.
    def("grid_step", 5);
    def("mm_step", 0.635);
    // Utils::GridType::Dots - the classic dotted schematic grid, matching
    // the reference FidoCadJ editor's look out of the box.
    def("grid_type", 1);
    def("grid_step_mark", 50);
    def("grid_line_width", 0.20);
    def("grid_line_mark_width", 0.20);
    // Whether the grid is actually drawn - toggled via the main toolbar's
    // grid button (actionShowGrid), independent of the grid's own appearance
    // settings above.
    def("grid_visible", true);

    // SNAP - defaults to the same spacing as the visible grid (grid_step) so
    // clicks visibly snap to grid intersections. Any multiple of 1 keeps
    // coordinates integers, satisfying FidoCadJ's grid-unit requirement
    // (FIDOSPECS.md 3), so this can be changed independently of grid_step.
    def("snap_enabled", true);
    def("snap_step", 5);

    // RULERS - whether the top/left rulers are shown around the drawing
    // area (RulerWidget), toggled from the Options dialog.
    def("rulers_visible", true);

    // AUTOSAVE - periodically writes the open drawing to a recovery sidecar
    // file so a crash doesn't lose unsaved work (see MainWindow::
    // autosaveTick()). Does not touch the document's own save path/undo
    // clean state.
    def("autosave_enabled", true);
    def("autosave_interval_minutes", 5);

    // UPDATE CHECK - a single HTTPS request to GitHub's releases API at
    // startup (see UpdateChecker), silent unless a newer version is found.
    // The manual "Check for updates" menu action ignores this setting.
    def("check_updates_on_startup", true);

    // OBJECT SNAP - see ObjectSnap.h/Sheet::snapPosition().
    def("snap_objects", true);
    def("object_snap_radius", 12);
    def("object_snap_endpoints", true);
    def("object_snap_midpoints", true);
    def("object_snap_centers", true);
    def("object_snap_intersections", true);
    def("snap_indicator_color", QColor(255, 128, 0).name());
    def("handle_color", QColor(Qt::red).name());
    def("snap_to_guides", true);
    def("guide_color", QColor(0, 170, 255).name());

    // BEHAVIOR/INTERFACE extras (see DialogOptions' pages).
    def("save_backup", false);
    def("undo_limit", 0);
    def("startup_reopen_last", false);
    def("recent_files_max", 10);
    def("toolbar_icon_size", 25);
    def("render_antialias", true);
    def("units_display", 0);
    def("wheel_zoom_direct", false);
    def("regular_polygon_sides", 6);
    def("curve_sampling_step", 5.0);
    def("boolean_smooth_results", false);
    def("text_default_font", "Courier New");
    def("nudge_step_multiplier", 1);
    def("dimension_text_size", 4);

    // COLORS
    def("background_color", QColor("white").name());
    def("grid_dot_color", QColor("blue").name());
    def("grid_line_color", QColor(100, 100, 100).name());
    def("grid_line_mark_color", QColor(100, 100, 100).name());
}
