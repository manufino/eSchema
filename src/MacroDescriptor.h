/*
 * MacroDescriptor.h
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

#ifndef MACRODESCRIPTOR_H
#define MACRODESCRIPTOR_H

#include <QString>

// One macro entry read from a ".fcl" library file (the reference FidoCadJ
// editor's own macro library format - see LibUtils.java/ParserActions.
// readLibraryBufferedReader()). Mirrors Java's MacroDesc.
struct MacroDescriptor
{
    QString key;      // lookup key: "prefix.name" lowercased, or bare "name" for the
                      // unprefixed standard library (FCDstdlib) - matches how "MC" lines
                      // reference macros (FIDOSPECS.md 5.10)
    QString name;     // display name shown to the user, e.g. "Terminale"
    QString category; // the "{...}" group this macro was declared under
    QString library;  // the library's display name (the "[FIDOLIB ...]" header text)
    QString filename; // the file-derived prefix ("", "pcb", "ihram", ...)
    QString body;     // the macro's FCD lines (LI/EV/BE/...), newline-joined, in the
                      // library's own (100,100)-centered local coordinate space
};

#endif // MACRODESCRIPTOR_H
