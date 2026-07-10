# eSchema Version
VERSION = 0.6.6

QT += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 # lrelease

INCLUDEPATH += src src/App src/Core src/Primitives src/Commands src/IO src/Dialogs src/Widgets

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
	src/main.cpp \
	src/App/MainWindow.cpp \
	src/App/MainWindowLibraryPanel.cpp \
	src/App/MainWindowPropertiesPanel.cpp \
	src/App/MainWindowEditActions.cpp \
	src/Core/GraphicsPrimitive.cpp \
	src/Core/LayerList.cpp \
	src/Core/LibraryManager.cpp \
	src/Core/SettingsManager.cpp \
	src/Core/Sheet.cpp \
	src/Core/SheetView.cpp \
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
	src/Commands/MirrorPrimitiveCommand.cpp \
	src/Commands/MovePrimitiveCommand.cpp \
	src/Commands/ResizePrimitiveCommand.cpp \
	src/Commands/RotatePrimitiveCommand.cpp \
	src/IO/FidoCadReader.cpp \
	src/IO/FidoCadTokenUtils.cpp \
	src/IO/FidoCadWriter.cpp \
	src/Dialogs/DialogAbout.cpp \
	src/Dialogs/DialogCreateMacro.cpp \
	src/Dialogs/DialogLayerList.cpp \
	src/Dialogs/DialogOptions.cpp \
	src/Dialogs/DialogShortcuts.cpp \
	src/Widgets/ColorPicker.cpp \
	src/Widgets/LayerColorPicker.cpp \
	src/Widgets/LayerComboBox.cpp \
	src/Widgets/LayerItemDelegate.cpp \
	src/Widgets/LayerLabelName.cpp \
	src/Widgets/LayerListView.cpp \
	src/Widgets/LayerListWidgetItem.cpp \
	src/Widgets/LayerToolBarWidget.cpp \
	src/Widgets/LayerVisibilityButton.cpp \
	src/Widgets/PenStyleComboBox.cpp \
	src/Widgets/RulerWidget.cpp \
	src/Widgets/StatusBar.cpp \
	src/Widgets/ToolBarPrimitive.cpp

HEADERS += \
	src/App/MainWindow.h \
	src/Core/GlobalUtils.h \
	src/Core/GraphicsPrimitive.h \
	src/Core/Layer.h \
	src/Core/LayerList.h \
	src/Core/LibraryManager.h \
	src/Core/MacroDescriptor.h \
	src/Core/SettingsManager.h \
	src/Core/Sheet.h \
	src/Core/SheetView.h \
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
	src/Commands/MirrorPrimitiveCommand.h \
	src/Commands/MovePrimitiveCommand.h \
	src/Commands/ResizePrimitiveCommand.h \
	src/Commands/RotatePrimitiveCommand.h \
	src/IO/FidoCadReader.h \
	src/IO/FidoCadTokenUtils.h \
	src/IO/FidoCadWriter.h \
	src/Dialogs/DialogAbout.h \
	src/Dialogs/DialogCreateMacro.h \
	src/Dialogs/DialogLayerList.h \
	src/Dialogs/DialogOptions.h \
	src/Dialogs/DialogShortcuts.h \
	src/Widgets/ColorPicker.h \
	src/Widgets/LayerColorPicker.h \
	src/Widgets/LayerComboBox.h \
	src/Widgets/LayerItemDelegate.h \
	src/Widgets/LayerLabelName.h \
	src/Widgets/LayerListView.h \
	src/Widgets/LayerListWidgetItem.h \
	src/Widgets/LayerToolBarWidget.h \
	src/Widgets/LayerVisibilityButton.h \
	src/Widgets/LinkLabel.h \
	src/Widgets/PenStyleComboBox.h \
	src/Widgets/RulerWidget.h \
	src/Widgets/StatusBar.h \
	src/Widgets/ToolBarPrimitive.h

FORMS += \
	gui/DialogAbout.ui \
	gui/DialogCreateMacro.ui \
	gui/DialogLayerList.ui \
	gui/DialogOptions.ui \
	gui/DialogShortcuts.ui \
    gui/MainWindow.ui \
	gui/LayerToolBarWidget.ui

RESOURCES += resources.qrc

# Italian is the source language the UI/tr() strings are written in, so it
# needs no .ts of its own - main.cpp simply installs no QTranslator for "it".
# TRANSLATIONS only drives `lupdate eSchema.pro`, which refreshes these .ts
# files with any new/changed tr() strings (existing translations are kept).
# After editing translations, re-run `lrelease` on each .ts to refresh the
# committed .qm binaries under translations/, which resources.qrc embeds.
TRANSLATIONS += \
	translations/eSchema_en.ts \
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
	DEFINES += BUILDDATE=\\\"$$system('echo %date%')\\\"
} else {
	DEFINES += BUILDDATE=\\\"$$system(date '+%d/%m/%y')\\\"
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
} else {
	QMAKE_POST_LINK += $$quote(mkdir -p \"$$OUT_PWD/lib\" && cp -r \"$$PWD/lib/.\" \"$$OUT_PWD/lib\" $$escape_expand(\\n\\t))
}
