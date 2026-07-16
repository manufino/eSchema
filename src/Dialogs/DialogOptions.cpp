/*
 * DialogOptions.cpp
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

#include "DialogOptions.h"
#include "ui_DialogOptions.h"
#include "ThemeManager.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>

namespace {
// Matches cboxLanguage's item order 1:1 (see DialogOptions.ui) - index into
// this to get/set the "language" setting's value from/to the combo box.
const QStringList LanguageCodes = {
    "it", "en", "de", "fr", "es", "ru", "zh", "pl", "ja", "pt", "ar", "hi"
};

// Matches cboxStyle's item order 1:1 (Light/Dark/the five built-in extra
// themes/System/Stylesheet, see DialogOptions.ui) - index into this to
// get/set the "gui_style" setting's value from/to the combo box. Also
// consumed by ThemeManager::apply().
const QStringList GuiStyleCodes = {
    "light", "dark", "nord", "midnight", "graphite", "sepia", "ocean", "system", "custom"
};

// listPages/stackPages order.
enum Page { PageGeneral = 0, PageInterface, PageGrid, PageSnap, PageDrawing };
}

DialogOptions::DialogOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogOptions)
{
    ui->setupUi(this);
    this->setModal(true);

    connect(ui->btnOK, &QPushButton::clicked, this, &DialogOptions::accept);
    connect(ui->btnCancel, &QPushButton::clicked, this, &DialogOptions::cancel);
    connect(ui->btnApply, &QPushButton::clicked, this, &DialogOptions::apply);
    connect(ui->btnRestore, &QPushButton::clicked, this, &DialogOptions::restore);
    connect(ui->listPages, &QListWidget::currentRowChanged,
            ui->stackPages, &QStackedWidget::setCurrentIndex);
    connect(ui->txtOptionsSearch, &QLineEdit::textChanged,
            this, [this](const QString &text) { filterPages(text); });
    connect(ui->cboxStyle, &QComboBox::currentIndexChanged, this, &DialogOptions::updateStylesheetPathEnabled);
    connect(ui->btnOpenStylesheetPath, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, tr("Select stylesheet"),
                                                            ui->txtStylesheetPath->text(),
                                                            tr("Stylesheet (*.qss)"));
        if (!path.isEmpty())
            ui->txtStylesheetPath->setText(path);
    });
    connect(ui->btnOpenLibPath, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getExistingDirectory(this, tr("Library path"),
                                                                 ui->txtLibPath->text());
        if (!path.isEmpty())
            ui->txtLibPath->setText(path);
    });

    // The Grid page's live preview follows its controls as they change.
    connect(ui->cboxGridType, &QComboBox::currentIndexChanged, this, [this]() { syncGridPreview(); });
    connect(ui->spinGridStep, &QSpinBox::valueChanged, this, [this]() { syncGridPreview(); });
    connect(ui->spinGridLineMarkStep, &QSpinBox::valueChanged, this, [this]() { syncGridPreview(); });
    connect(ui->doubleSpinGridLineWidth, &QDoubleSpinBox::valueChanged, this, [this]() { syncGridPreview(); });
    connect(ui->doubleSpinGridLineMarkWidth, &QDoubleSpinBox::valueChanged, this, [this]() { syncGridPreview(); });
    connect(ui->lblGridColor, &ColorPicker::colorChanged, this, [this]() { syncGridPreview(); });
    connect(ui->lblGridLineNormalColor, &ColorPicker::colorChanged, this, [this]() { syncGridPreview(); });
    connect(ui->lblGridLineMarkColor, &ColorPicker::colorChanged, this, [this]() { syncGridPreview(); });
    connect(ui->lblBackColor, &ColorPicker::colorChanged, this, [this]() { syncGridPreview(); });

    loadSettings();
    ui->listPages->setCurrentRow(PageGeneral);
}

DialogOptions::~DialogOptions()
{
    delete ui;
}

void DialogOptions::loadSettings()
{
    auto &settings = SettingsManager::getInstance();
    QVariant val;

    // --- General ---
    val = settings.loadSetting("library_directory");
    ui->txtLibPath->setText(val.toString());
    val = settings.loadSetting("autosave_enabled");
    ui->chkAutosaveEnabled->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("autosave_interval_minutes");
    ui->spinAutosaveInterval->setValue(val.toInt() > 0 ? val.toInt() : 5);
    val = settings.loadSetting("save_backup");
    ui->chkSaveBackup->setChecked(val.toBool());
    val = settings.loadSetting("undo_limit");
    ui->spinUndoLimit->setValue(qMax(0, val.toInt()));
    val = settings.loadSetting("check_updates_on_startup");
    ui->chkCheckUpdatesOnStartup->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("startup_reopen_last");
    ui->chkStartupReopenLast->setChecked(val.toBool());
    val = settings.loadSetting("recent_files_max");
    ui->spinRecentFilesMax->setValue(val.toInt() > 0 ? val.toInt() : 10);

    // --- Interface ---
    val = settings.loadSetting("language");
    const int languageIndex = LanguageCodes.indexOf(val.toString());
    ui->cboxLanguage->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
    m_initialLanguageIndex = ui->cboxLanguage->currentIndex();
    val = settings.loadSetting("gui_style");
    const int styleIndex = GuiStyleCodes.indexOf(val.toString());
    ui->cboxStyle->setCurrentIndex(styleIndex >= 0 ? styleIndex : 0);
    val = settings.loadSetting("stylesheet_path");
    ui->txtStylesheetPath->setText(val.toString());
    updateStylesheetPathEnabled();
    val = settings.loadSetting("toolbar_icon_size");
    ui->spinToolbarIconSize->setValue(val.toInt() > 0 ? val.toInt() : 25);
    val = settings.loadSetting("render_antialias");
    ui->chkRenderAntialias->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("units_display");
    ui->cboxUnitsDisplay->setCurrentIndex(qBound(0, val.toInt(), 2));
    val = settings.loadSetting("wheel_zoom_direct");
    ui->chkWheelZoomDirect->setChecked(val.toBool());
    val = settings.loadSetting("macro_icon_size");
    ui->spinMacroIconSize->setValue(val.toInt() > 0 ? val.toInt() : 32);
    val = settings.loadSetting("macro_tree_icons_enabled");
    ui->chkMacroTreeIcons->setChecked(val.isValid() ? val.toBool() : true);

    // --- Grid ---
    val = settings.loadSetting("grid_type");
    ui->cboxGridType->setCurrentIndex(val.toInt());
    val = settings.loadSetting("grid_step");
    ui->spinGridStep->setValue(val.toInt());
    val = settings.loadSetting("grid_step_mark");
    ui->spinGridLineMarkStep->setValue(val.toInt());
    val = settings.loadSetting("mm_step");
    ui->doubleSpinStep_mm->setValue(val.toDouble());
    val = settings.loadSetting("snap_enabled");
    ui->chkSnapEnabled->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("snap_step");
    ui->spinSnapStep->setValue(val.toInt());
    val = settings.loadSetting("grid_dot_color");
    ui->lblGridColor->setColor(QColor(val.toString()));
    val = settings.loadSetting("grid_line_color");
    ui->lblGridLineNormalColor->setColor(QColor(val.toString()));
    val = settings.loadSetting("grid_line_mark_color");
    ui->lblGridLineMarkColor->setColor(QColor(val.toString()));
    val = settings.loadSetting("background_color");
    ui->lblBackColor->setColor(QColor(val.toString()));
    val = settings.loadSetting("grid_line_width");
    ui->doubleSpinGridLineWidth->setValue(val.toDouble());
    val = settings.loadSetting("grid_line_mark_width");
    ui->doubleSpinGridLineMarkWidth->setValue(val.toDouble());
    syncGridPreview();

    // --- Snap ---
    val = settings.loadSetting("snap_objects");
    ui->chkObjectSnapEnabled->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("object_snap_radius");
    ui->spinObjectSnapRadius->setValue(val.toInt() > 0 ? val.toInt() : 12);
    val = settings.loadSetting("object_snap_endpoints");
    ui->chkSnapEndpoints->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("object_snap_midpoints");
    ui->chkSnapMidpoints->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("object_snap_centers");
    ui->chkSnapCenters->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("object_snap_intersections");
    ui->chkSnapIntersections->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("snap_indicator_color");
    ui->pickSnapIndicatorColor->setColor(QColor(val.isValid() && QColor(val.toString()).isValid()
                                                 ? val.toString() : QStringLiteral("#ff8000")));
    val = settings.loadSetting("handle_color");
    ui->pickHandleColor->setColor(QColor(val.isValid() && QColor(val.toString()).isValid()
                                          ? val.toString() : QStringLiteral("#ff0000")));
    val = settings.loadSetting("snap_to_guides");
    ui->chkSnapGuides->setChecked(val.isValid() ? val.toBool() : true);
    val = settings.loadSetting("guide_color");
    ui->pickGuideColor->setColor(QColor(val.isValid() && QColor(val.toString()).isValid()
                                         ? val.toString() : QStringLiteral("#00aaff")));

    // --- Drawing ---
    val = settings.loadSetting("line_width");
    ui->spinLineWidth->setValue(val.toDouble() > 0 ? val.toDouble() : 0.5);
    val = settings.loadSetting("selection_tolerance");
    ui->spinSelectionTolerance->setValue(val.toDouble() > 0 ? val.toDouble() : 3.0);
    val = settings.loadSetting("connection_diameter");
    ui->spinConnectionDiameter->setValue(val.toDouble() > 0 ? val.toDouble() : 2.0);
    val = settings.loadSetting("regular_polygon_sides");
    ui->spinPolygonSides->setValue(val.toInt() >= 3 ? val.toInt() : 6);
    val = settings.loadSetting("curve_sampling_step");
    ui->spinCurveSampling->setValue(val.toDouble() > 0 ? val.toDouble() : 5.0);
    val = settings.loadSetting("boolean_smooth_results");
    ui->chkBooleanSmooth->setChecked(val.toBool());
    val = settings.loadSetting("text_default_font");
    ui->fontComboDefaultText->setCurrentFont(QFont(
            val.isValid() && !val.toString().isEmpty() ? val.toString()
                                                       : QStringLiteral("Courier New")));
    val = settings.loadSetting("nudge_step_multiplier");
    ui->spinNudgeMultiplier->setValue(val.toInt() > 0 ? val.toInt() : 1);
    val = settings.loadSetting("dimension_text_size");
    ui->spinDimensionTextSize->setValue(val.toInt() > 0 ? val.toInt() : 4);
}

void DialogOptions::saveSettings()
{
    auto &settings = SettingsManager::getInstance();

    // --- General ---
    settings.saveSetting("library_directory", ui->txtLibPath->text());
    settings.saveSetting("autosave_enabled", ui->chkAutosaveEnabled->isChecked());
    settings.saveSetting("autosave_interval_minutes", ui->spinAutosaveInterval->value());
    settings.saveSetting("save_backup", ui->chkSaveBackup->isChecked());
    settings.saveSetting("undo_limit", ui->spinUndoLimit->value());
    settings.saveSetting("check_updates_on_startup", ui->chkCheckUpdatesOnStartup->isChecked());
    settings.saveSetting("startup_reopen_last", ui->chkStartupReopenLast->isChecked());
    settings.saveSetting("recent_files_max", ui->spinRecentFilesMax->value());

    // --- Interface ---
    settings.saveSetting("language", LanguageCodes.at(ui->cboxLanguage->currentIndex()));
    settings.saveSetting("gui_style", GuiStyleCodes.at(ui->cboxStyle->currentIndex()));
    settings.saveSetting("stylesheet_path", ui->txtStylesheetPath->text());
    settings.saveSetting("toolbar_icon_size", ui->spinToolbarIconSize->value());
    settings.saveSetting("render_antialias", ui->chkRenderAntialias->isChecked());
    settings.saveSetting("units_display", ui->cboxUnitsDisplay->currentIndex());
    settings.saveSetting("wheel_zoom_direct", ui->chkWheelZoomDirect->isChecked());
    settings.saveSetting("macro_icon_size", ui->spinMacroIconSize->value());
    settings.saveSetting("macro_tree_icons_enabled", ui->chkMacroTreeIcons->isChecked());

    // --- Grid ---
    settings.saveSetting("grid_type", ui->cboxGridType->currentIndex());
    settings.saveSetting("grid_step", ui->spinGridStep->value());
    settings.saveSetting("grid_step_mark", ui->spinGridLineMarkStep->value());
    settings.saveSetting("mm_step", ui->doubleSpinStep_mm->value());
    settings.saveSetting("snap_enabled", ui->chkSnapEnabled->isChecked());
    settings.saveSetting("snap_step", ui->spinSnapStep->value());
    settings.saveSetting("grid_dot_color", ui->lblGridColor->getColor().name());
    settings.saveSetting("grid_line_color", ui->lblGridLineNormalColor->getColor().name());
    settings.saveSetting("grid_line_mark_color", ui->lblGridLineMarkColor->getColor().name());
    settings.saveSetting("background_color", ui->lblBackColor->getColor().name());
    settings.saveSetting("grid_line_width", ui->doubleSpinGridLineWidth->value());
    settings.saveSetting("grid_line_mark_width", ui->doubleSpinGridLineMarkWidth->value());

    // --- Snap ---
    settings.saveSetting("snap_objects", ui->chkObjectSnapEnabled->isChecked());
    settings.saveSetting("object_snap_radius", ui->spinObjectSnapRadius->value());
    settings.saveSetting("object_snap_endpoints", ui->chkSnapEndpoints->isChecked());
    settings.saveSetting("object_snap_midpoints", ui->chkSnapMidpoints->isChecked());
    settings.saveSetting("object_snap_centers", ui->chkSnapCenters->isChecked());
    settings.saveSetting("object_snap_intersections", ui->chkSnapIntersections->isChecked());
    settings.saveSetting("snap_indicator_color", ui->pickSnapIndicatorColor->getColor().name());
    settings.saveSetting("handle_color", ui->pickHandleColor->getColor().name());
    settings.saveSetting("snap_to_guides", ui->chkSnapGuides->isChecked());
    settings.saveSetting("guide_color", ui->pickGuideColor->getColor().name());

    // --- Drawing ---
    settings.saveSetting("line_width", ui->spinLineWidth->value());
    settings.saveSetting("selection_tolerance", ui->spinSelectionTolerance->value());
    settings.saveSetting("connection_diameter", ui->spinConnectionDiameter->value());
    settings.saveSetting("regular_polygon_sides", ui->spinPolygonSides->value());
    settings.saveSetting("curve_sampling_step", ui->spinCurveSampling->value());
    settings.saveSetting("boolean_smooth_results", ui->chkBooleanSmooth->isChecked());
    settings.saveSetting("text_default_font", ui->fontComboDefaultText->currentFont().family());
    settings.saveSetting("nudge_step_multiplier", ui->spinNudgeMultiplier->value());
    settings.saveSetting("dimension_text_size", ui->spinDimensionTextSize->value());
}

void DialogOptions::syncGridPreview()
{
    ui->gridPreview->setGridType(ui->cboxGridType->currentIndex());
    ui->gridPreview->setStep(ui->spinGridStep->value());
    ui->gridPreview->setMarkStep(ui->spinGridLineMarkStep->value());
    ui->gridPreview->setDotColor(ui->lblGridColor->getColor());
    ui->gridPreview->setLineColor(ui->lblGridLineNormalColor->getColor());
    ui->gridPreview->setMarkColor(ui->lblGridLineMarkColor->getColor());
    ui->gridPreview->setBackgroundColor(ui->lblBackColor->getColor());
    ui->gridPreview->setLineWidth(ui->doubleSpinGridLineWidth->value());
    ui->gridPreview->setMarkWidth(ui->doubleSpinGridLineMarkWidth->value());
}

// A page matches when any of its labels, checkboxes, or group titles
// contains the text - crude, but enough to find where an option lives.
void DialogOptions::filterPages(const QString &text)
{
    const QString needle = text.trimmed();
    for (int row = 0; row < ui->listPages->count(); ++row) {
        QWidget *page = ui->stackPages->widget(row);
        bool matches = needle.isEmpty()
                || ui->listPages->item(row)->text().contains(needle, Qt::CaseInsensitive);
        if (!matches) {
            for (const QLabel *label : page->findChildren<QLabel *>()) {
                if (label->text().contains(needle, Qt::CaseInsensitive)) {
                    matches = true;
                    break;
                }
            }
        }
        if (!matches) {
            for (const QCheckBox *box : page->findChildren<QCheckBox *>()) {
                if (box->text().contains(needle, Qt::CaseInsensitive)) {
                    matches = true;
                    break;
                }
            }
        }
        if (!matches) {
            for (const QGroupBox *group : page->findChildren<QGroupBox *>()) {
                if (group->title().contains(needle, Qt::CaseInsensitive)) {
                    matches = true;
                    break;
                }
            }
        }
        ui->listPages->item(row)->setHidden(!matches);
    }
    // Keep the selection on a visible page.
    if (ui->listPages->currentItem() && ui->listPages->currentItem()->isHidden()) {
        for (int row = 0; row < ui->listPages->count(); ++row) {
            if (!ui->listPages->item(row)->isHidden()) {
                ui->listPages->setCurrentRow(row);
                break;
            }
        }
    }
}

void DialogOptions::restorePageDefaults(int pageIndex)
{
    switch (pageIndex) {
    case PageGeneral:
        ui->txtLibPath->setText(QString());
        ui->chkAutosaveEnabled->setChecked(true);
        ui->spinAutosaveInterval->setValue(5);
        ui->chkSaveBackup->setChecked(false);
        ui->spinUndoLimit->setValue(0);
        ui->chkCheckUpdatesOnStartup->setChecked(true);
        ui->chkStartupReopenLast->setChecked(false);
        ui->spinRecentFilesMax->setValue(10);
        break;
    case PageInterface:
        // Language deliberately untouched - "reset" silently switching the
        // UI language would be hostile.
        ui->cboxStyle->setCurrentIndex(0);
        ui->txtStylesheetPath->clear();
        ui->spinToolbarIconSize->setValue(25);
        ui->chkRenderAntialias->setChecked(true);
        ui->cboxUnitsDisplay->setCurrentIndex(0);
        ui->chkWheelZoomDirect->setChecked(false);
        ui->spinMacroIconSize->setValue(32);
        ui->chkMacroTreeIcons->setChecked(true);
        break;
    case PageGrid:
        // Mirrors SettingsManager::restoreDefaultSettings()'s grid values.
        ui->cboxGridType->setCurrentIndex(0);
        ui->spinGridStep->setValue(5);
        ui->spinGridLineMarkStep->setValue(50);
        ui->doubleSpinStep_mm->setValue(0.635);
        ui->chkSnapEnabled->setChecked(true);
        ui->spinSnapStep->setValue(5);
        ui->lblGridColor->setColor(QColor(Qt::blue));
        ui->lblGridLineNormalColor->setColor(QColor(100, 100, 100));
        ui->lblGridLineMarkColor->setColor(QColor(100, 100, 100));
        ui->lblBackColor->setColor(QColor(Qt::white));
        ui->doubleSpinGridLineWidth->setValue(0.20);
        ui->doubleSpinGridLineMarkWidth->setValue(0.20);
        break;
    case PageSnap:
        ui->chkObjectSnapEnabled->setChecked(true);
        ui->spinObjectSnapRadius->setValue(12);
        ui->chkSnapEndpoints->setChecked(true);
        ui->chkSnapMidpoints->setChecked(true);
        ui->chkSnapCenters->setChecked(true);
        ui->chkSnapIntersections->setChecked(true);
        ui->pickSnapIndicatorColor->setColor(QColor(QStringLiteral("#ff8000")));
        ui->pickHandleColor->setColor(QColor(QStringLiteral("#ff0000")));
        ui->chkSnapGuides->setChecked(true);
        ui->pickGuideColor->setColor(QColor(QStringLiteral("#00aaff")));
        break;
    case PageDrawing:
        ui->spinLineWidth->setValue(0.5);
        ui->spinSelectionTolerance->setValue(3.0);
        ui->spinConnectionDiameter->setValue(2.0);
        ui->spinPolygonSides->setValue(6);
        ui->spinCurveSampling->setValue(5.0);
        ui->chkBooleanSmooth->setChecked(false);
        ui->fontComboDefaultText->setCurrentFont(QFont(QStringLiteral("Courier New")));
        ui->spinNudgeMultiplier->setValue(1);
        ui->spinDimensionTextSize->setValue(4);
        break;
    default:
        break;
    }
    syncGridPreview();
}

void DialogOptions::accept()
{
    apply();
    close();
}

void DialogOptions::cancel()
{
    close();
}

void DialogOptions::apply()
{
    saveSettings();
    ThemeManager::apply();

    // The UI isn't retranslated live - the change is saved and takes effect
    // on the next launch, so tell the user rather than leaving them thinking
    // the combo box selection did nothing.
    if (ui->cboxLanguage->currentIndex() != m_initialLanguageIndex) {
        m_initialLanguageIndex = ui->cboxLanguage->currentIndex();
        QMessageBox::information(this, tr("Language"),
                                  tr("The new language will take effect the next time eSchema starts."));
    }
}

void DialogOptions::updateStylesheetPathEnabled()
{
    const bool isCustom = ui->cboxStyle->currentIndex() == GuiStyleCodes.indexOf("custom");
    ui->txtStylesheetPath->setEnabled(isCustom);
    ui->btnOpenStylesheetPath->setEnabled(isCustom);
}

// Page-scoped, and only touching the widgets: nothing lands in the settings
// until Apply/OK, so a mistaken reset is still cancellable.
void DialogOptions::restore()
{
    restorePageDefaults(ui->stackPages->currentIndex());
}
