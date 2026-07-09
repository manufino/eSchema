/*
 * DialogCreateMacro.cpp
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

#include "DialogCreateMacro.h"
#include "ui_DialogCreateMacro.h"
#include "LibraryManager.h"
#include <QInputDialog>

namespace {
// Always the combo's last entry - picking it prompts for a new library name
// instead of naming an existing one.
QString newLibrarySentinel()
{
    return QObject::tr("Nuova libreria...");
}
}

DialogCreateMacro::DialogCreateMacro(QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogCreateMacro)
{
    ui->setupUi(this);
    setModal(true);

    ui->cboxLibrary->addItems(LibraryManager::getInstance().userLibraryDisplayNames());
    ui->cboxLibrary->addItem(newLibrarySentinel());
    ui->cboxLibrary->setCurrentIndex(-1); // force an explicit choice

    connect(ui->cboxLibrary, &QComboBox::activated, this, &DialogCreateMacro::handleLibrarySelected);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogCreateMacro::validateAndAccept);
}

DialogCreateMacro::~DialogCreateMacro()
{
    delete ui;
}

QString DialogCreateMacro::macroName() const
{
    return ui->txtName->text().trimmed();
}

QString DialogCreateMacro::category() const
{
    return ui->txtCategory->text().trimmed();
}

QString DialogCreateMacro::libraryDisplayName() const
{
    return ui->cboxLibrary->currentText().trimmed();
}

QString DialogCreateMacro::libraryFilename() const
{
    const QString display = libraryDisplayName();
    const QString existing = LibraryManager::getInstance().userLibraryFilename(display);
    if (!existing.isEmpty())
        return existing;

    // A new library: sanitize the typed display name into a filesystem- and
    // FCD-prefix-safe filename (letters/digits/underscore only - the same
    // string ends up both as "<name>.fcl" on disk and as every macro key's
    // "<name>.key" prefix).
    QString sanitized;
    for (const QChar &c : display) {
        if (c.isLetterOrNumber() || c == QLatin1Char('_'))
            sanitized += c;
    }
    return sanitized;
}

void DialogCreateMacro::handleLibrarySelected(int index)
{
    if (index != ui->cboxLibrary->count() - 1) {
        m_previousLibraryIndex = index;
        return;
    }

    const QString name = QInputDialog::getText(this, tr("Nuova libreria"),
                                                 tr("Nome della nuova libreria:")).trimmed();
    if (name.isEmpty()) {
        ui->cboxLibrary->setCurrentIndex(m_previousLibraryIndex);
        return;
    }

    const int insertAt = ui->cboxLibrary->count() - 1; // just before the sentinel
    ui->cboxLibrary->insertItem(insertAt, name);
    ui->cboxLibrary->setCurrentIndex(insertAt);
    m_previousLibraryIndex = insertAt;
}

void DialogCreateMacro::validateAndAccept()
{
    ui->labelError->clear();

    if (macroName().isEmpty()) {
        ui->labelError->setText(tr("Il nome è obbligatorio."));
        return;
    }
    const QString display = libraryDisplayName();
    if (display.isEmpty()) {
        ui->labelError->setText(tr("Seleziona o crea una libreria."));
        return;
    }
    const QString filename = libraryFilename();
    if (filename.isEmpty()) {
        ui->labelError->setText(tr("Nome libreria non valido: usa lettere, numeri o underscore."));
        return;
    }
    if (LibraryManager::getInstance().isStandardLibraryFilename(filename)) {
        ui->labelError->setText(tr("Non è possibile salvare in una libreria standard."));
        return;
    }

    accept();
}
