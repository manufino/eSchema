# eSchema Version
VERSION = 1.0.5

QT += core gui printsupport svg network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 # lrelease
# Mirrors each source's directory under the build dir for its .o file,
# instead of just its basename - needed because two vendored libdxfrw
# filenames (intern/dxfreader.cpp, intern/dxfwriter.cpp) only differ in case
# from this project's own DxfReader.cpp/DxfWriter.cpp, which collide on
# Windows' case-insensitive filesystem otherwise.
CONFIG += object_parallel_to_source

INCLUDEPATH += src src/App src/Core src/Primitives src/Commands src/IO src/Dialogs src/Widgets

# Vendored DXF read/write library (see third_party/libdxfrw/README-eschema.md).
# Must stay AFTER the INCLUDEPATH line above: this app's own DxfReader.h/
# DxfWriter.h only differ in case from libdxfrw's intern/dxfreader.h/
# dxfwriter.h, and on Windows' case-insensitive filesystem, whichever
# INCLUDEPATH entry appears first wins an unqualified #include lookup from a
# file outside src/IO/ itself (e.g. MainWindow.cpp) - moving this above the
# INCLUDEPATH line would silently start pulling in the wrong header.
include(third_party/libdxfrw/libdxfrw.pri)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# APP_VERSION used to be a bare -D compiler define, but make rebuilds
# nothing when a define changes - after a version bump every
# not-otherwise-touched object kept the old string baked in (stale About
# box, update checker comparing GitHub against the wrong version). Writing
# it to a generated header instead turns the bump into an ordinary file
# change that dependency tracking picks up. The Makefile re-runs qmake by
# itself whenever this .pro is newer, so the header follows VERSION
# automatically.
APP_VERSION_LINE = '$${LITERAL_HASH}define APP_VERSION "$$VERSION"'
write_file($$OUT_PWD/appversion.h, APP_VERSION_LINE)
INCLUDEPATH += $$OUT_PWD

SOURCES += \
	src/main.cpp \
	src/App/CommandLine.cpp \
	src/App/MainWindow.cpp \
	src/App/MainWindowLibraryPanel.cpp \
	src/App/MainWindowPropertiesPanel.cpp \
	src/App/MainWindowEditActions.cpp \
	src/Core/BooleanOps.cpp \
	src/Core/DecoratedText.cpp \
	src/Core/GraphicsPrimitive.cpp \
	src/Core/LayerList.cpp \
	src/Core/LibraryManager.cpp \
	src/Core/ObjectSnap.cpp \
	src/Core/SettingsManager.cpp \
	src/Core/Sheet.cpp \
	src/Core/SheetView.cpp \
	src/Core/ThemeManager.cpp \
	src/Core/UpdateChecker.cpp \
	src/Primitives/PrimitiveBezier.cpp \
	src/Primitives/PrimitiveComplexCurve.cpp \
	src/Primitives/PrimitiveConnection.cpp \
	src/Primitives/PrimitiveEllipse.cpp \
	src/Primitives/PrimitiveHandleItem.cpp \
	src/Primitives/PrimitiveImage.cpp \
	src/Primitives/PrimitiveLine.cpp \
	src/Primitives/PrimitiveMacro.cpp \
	src/Primitives/PrimitivePad.cpp \
	src/Primitives/PrimitivePcbTrack.cpp \
	src/Primitives/PrimitivePlacementController.cpp \
	src/Primitives/PrimitivePolygon.cpp \
	src/Primitives/PrimitiveRectangle.cpp \
	src/Primitives/PrimitiveText.cpp \
	src/Primitives/SelectionHandleController.cpp \
	src/Commands/CreatePrimitiveCommand.cpp \
	src/Commands/DeletePrimitiveCommand.cpp \
	src/Commands/InsertNodeCommand.cpp \
	src/Commands/MirrorPrimitiveCommand.cpp \
	src/Commands/MovePrimitiveCommand.cpp \
	src/Commands/RemoveNodeCommand.cpp \
	src/Commands/ResizePrimitiveCommand.cpp \
	src/Commands/RotatePrimitiveCommand.cpp \
	src/IO/DxfCommon.cpp \
	src/IO/DxfReader.cpp \
	src/IO/DxfWriter.cpp \
	src/IO/EpsGenerator.cpp \
	src/IO/FidoCadReader.cpp \
	src/IO/FidoCadTokenUtils.cpp \
	src/IO/FidoCadWriter.cpp \
	src/IO/GraphicExporter.cpp \
	src/Dialogs/DialogAbout.cpp \
	src/Dialogs/DialogArray.cpp \
	src/Dialogs/DialogAttachImage.cpp \
	src/Dialogs/DialogCustomizeToolbars.cpp \
	src/Dialogs/DialogCreateMacro.cpp \
	src/Dialogs/DialogExport.cpp \
	src/Dialogs/DialogFind.cpp \
	src/Dialogs/DialogPrintOptions.cpp \
	src/Dialogs/DialogLayerList.cpp \
	src/Dialogs/DialogOptions.cpp \
	src/Dialogs/DialogShortcuts.cpp \
	src/Widgets/ColorPicker.cpp \
	src/Widgets/GridPreviewWidget.cpp \
	src/Widgets/LayerColorPicker.cpp \
	src/Widgets/LayerComboBox.cpp \
	src/Widgets/LayerIcons.cpp \
	src/Widgets/LayerItemDelegate.cpp \
	src/Widgets/LayerLabelName.cpp \
	src/Widgets/LayerListView.cpp \
	src/Widgets/LayerListWidgetItem.cpp \
	src/Widgets/LayerLockButton.cpp \
	src/Widgets/LayerToolBarWidget.cpp \
	src/Widgets/LayerVisibilityButton.cpp \
	src/Widgets/MacroPreviewWidget.cpp \
	src/Widgets/PenStyleComboBox.cpp \
	src/Widgets/RulerWidget.cpp \
	src/Widgets/StatusBar.cpp \
	src/Widgets/ToolBarPrimitive.cpp

HEADERS += \
	src/App/CommandLine.h \
	src/App/MainWindow.h \
	src/Core/BooleanOps.h \
	src/Core/DecoratedText.h \
	src/Core/GlobalUtils.h \
	src/Core/GraphicsPrimitive.h \
	src/Core/Layer.h \
	src/Core/LayerList.h \
	src/Core/LibraryManager.h \
	src/Core/MacroDescriptor.h \
	src/Core/ObjectSnap.h \
	src/Core/SettingsManager.h \
	src/Core/Sheet.h \
	src/Core/SheetView.h \
	src/Core/ThemeManager.h \
	src/Core/UpdateChecker.h \
	src/Primitives/PrimitiveArrowUtils.h \
	src/Primitives/PrimitiveBezier.h \
	src/Primitives/PrimitiveComplexCurve.h \
	src/Primitives/PrimitiveConnection.h \
	src/Primitives/PrimitiveEllipse.h \
	src/Primitives/PrimitiveHandleItem.h \
	src/Primitives/PrimitiveImage.h \
	src/Primitives/PrimitiveLine.h \
	src/Primitives/PrimitiveMacro.h \
	src/Primitives/PrimitivePad.h \
	src/Primitives/PrimitivePcbTrack.h \
	src/Primitives/PrimitivePlacementController.h \
	src/Primitives/PrimitivePolygon.h \
	src/Primitives/PrimitiveRectangle.h \
	src/Primitives/PrimitiveText.h \
	src/Primitives/SelectionHandleController.h \
	src/Commands/CreatePrimitiveCommand.h \
	src/Commands/DeletePrimitiveCommand.h \
	src/Commands/InsertNodeCommand.h \
	src/Commands/MirrorPrimitiveCommand.h \
	src/Commands/MovePrimitiveCommand.h \
	src/Commands/RemoveNodeCommand.h \
	src/Commands/ResizePrimitiveCommand.h \
	src/Commands/RotatePrimitiveCommand.h \
	src/IO/DxfCommon.h \
	src/IO/DxfReader.h \
	src/IO/DxfWriter.h \
	src/IO/EpsGenerator.h \
	src/IO/FidoCadReader.h \
	src/IO/FidoCadTokenUtils.h \
	src/IO/FidoCadWriter.h \
	src/IO/GraphicExporter.h \
	src/Dialogs/DialogAbout.h \
	src/Dialogs/DialogArray.h \
	src/Dialogs/DialogAttachImage.h \
	src/Dialogs/DialogCustomizeToolbars.h \
	src/Dialogs/DialogCreateMacro.h \
	src/Dialogs/DialogExport.h \
	src/Dialogs/DialogFind.h \
	src/Dialogs/DialogPrintOptions.h \
	src/Dialogs/DialogLayerList.h \
	src/Dialogs/DialogOptions.h \
	src/Dialogs/DialogShortcuts.h \
	src/Widgets/ColorPicker.h \
	src/Widgets/GridPreviewWidget.h \
	src/Widgets/LayerColorPicker.h \
	src/Widgets/LayerComboBox.h \
	src/Widgets/LayerIcons.h \
	src/Widgets/LayerItemDelegate.h \
	src/Widgets/LayerLabelName.h \
	src/Widgets/LayerListView.h \
	src/Widgets/LayerListWidgetItem.h \
	src/Widgets/LayerLockButton.h \
	src/Widgets/LayerToolBarWidget.h \
	src/Widgets/LayerVisibilityButton.h \
	src/Widgets/LinkLabel.h \
	src/Widgets/MacroPreviewWidget.h \
	src/Widgets/PenStyleComboBox.h \
	src/Widgets/RulerWidget.h \
	src/Widgets/StatusBar.h \
	src/Widgets/ToolBarPrimitive.h

FORMS += \
	gui/DialogAbout.ui \
	gui/DialogArray.ui \
	gui/DialogAttachImage.ui \
	gui/DialogCustomizeToolbars.ui \
	gui/DialogCreateMacro.ui \
	gui/DialogExport.ui \
	gui/DialogFind.ui \
	gui/DialogPrintOptions.ui \
	gui/DialogLayerList.ui \
	gui/DialogOptions.ui \
	gui/DialogShortcuts.ui \
    gui/MainWindow.ui \
	gui/LayerToolBarWidget.ui

RESOURCES += resources.qrc

# English is the source language the UI/tr() strings are written in, so it
# needs no .ts of its own - main.cpp simply installs no QTranslator for "en".
# TRANSLATIONS only drives `lupdate eSchema.pro`, which refreshes these .ts
# files with any new/changed tr() strings (existing translations are kept).
# After editing translations, re-run `lrelease` on each .ts to refresh the
# committed .qm binaries under translations/, which resources.qrc embeds.
TRANSLATIONS += \
	translations/eSchema_it.ts \
	translations/eSchema_de.ts \
	translations/eSchema_fr.ts \
	translations/eSchema_es.ts \
	translations/eSchema_ru.ts \
	translations/eSchema_zh.ts \
	translations/eSchema_pl.ts \
	translations/eSchema_ja.ts \
	translations/eSchema_pt.ts \
	translations/eSchema_ar.ts \
	translations/eSchema_hi.ts

win32 {
	RC_ICONS = "resources/main.ico"
	# $$system() on win32 always shells out through cmd.exe, regardless of
	# which shell qmake itself was launched from - so this has to be a cmd
	# command. Plain "echo %date%" (cmd's own locale-formatted date) is not
	# an option: on a machine/CI runner whose regional short-date format
	# includes the weekday name (e.g. "Fri 07/10/2026"), the embedded space
	# splits DEFINES into two list entries, and the second one
	# ("-D07/10/2026\"") isn't a valid macro name, breaking the build.
	# Get-Date -Format explicitly with no day name and no locale-dependent
	# spacing sidesteps that regardless of the machine's regional settings.
	DEFINES += BUILDDATE=\\\"$$system(powershell -NoProfile -Command Get-Date -Format dd/MM/yyyy)\\\"
} else {
	DEFINES += BUILDDATE=\\\"$$system(date '+%d/%m/%y')\\\"
}

macx {
	# Xcode 15+'s Clang promotes -Wimplicit-function-declaration from a
	# warning to a hard error by default. Qt 6.9.2's own qyieldcpu.h header
	# calls the __yield() intrinsic without declaring it first, which then
	# fails our build the moment any of our sources transitively include
	# that Qt header - nothing to do with our own code, so downgrade just
	# this one diagnostic back to a warning.
	QMAKE_CXXFLAGS += -Wno-error=implicit-function-declaration
	QMAKE_CFLAGS += -Wno-error=implicit-function-declaration
}

# Copies the FidoCadJ macro libraries next to the built executable after every
# link, so LibraryManager (which resolves "lib/" relative to
# QCoreApplication::applicationDirPath()) finds them without needing an
# absolute path or Qt resource embedding - matching the reference FidoCadJ
# editor's own external DIR_LIBS convention, which lets users drop in their
# own .fcl files without rebuilding. The win32-g++ mkspec puts the actual
# executable one level below $$OUT_PWD, in "release/" or "debug/" depending on
# CONFIG - copying into both keeps this correct regardless of which variant
# was just built.
win32 {
	# QMAKE_POST_LINK runs under whatever shell "make" resolves (sh.exe from
	# Git for Windows here, not cmd) - $$system_path() (unlike $$shell_path())
	# always yields a native "C:\..." path regardless of that shell, which the
	# native xcopy.exe binary needs. The flags use double slashes (//s //y //i,
	# not /s /y /i) because MSYS2/Git-Bash's sh auto-translates any argument
	# starting with a single "/" into a bogus Windows path before xcopy ever
	# sees it - "//" is MSYS's documented escape to suppress that conversion.
	QMAKE_POST_LINK += $$quote(xcopy //s //y //i \"$$system_path($$PWD/lib)\" \"$$system_path($$OUT_PWD/release/lib)\" $$escape_expand(\\n\\t))
	QMAKE_POST_LINK += $$quote(xcopy //s //y //i \"$$system_path($$PWD/lib)\" \"$$system_path($$OUT_PWD/debug/lib)\" $$escape_expand(\\n\\t))
} else:macx {
	# CONFIG+=app_bundle is the default on mac, so the actual executable lands
	# inside $$TARGET.app/Contents/MacOS/ - not directly under $$OUT_PWD like
	# on Linux - and that's the directory applicationDirPath() resolves to, so
	# "lib" has to be copied one level deeper here than the unix branch below.
	QMAKE_POST_LINK += $$quote(mkdir -p \"$$OUT_PWD/$${TARGET}.app/Contents/MacOS/lib\" && cp -r \"$$PWD/lib/.\" \"$$OUT_PWD/$${TARGET}.app/Contents/MacOS/lib\" $$escape_expand(\\n\\t))
} else {
	QMAKE_POST_LINK += $$quote(mkdir -p \"$$OUT_PWD/lib\" && cp -r \"$$PWD/lib/.\" \"$$OUT_PWD/lib\" $$escape_expand(\\n\\t))
}
