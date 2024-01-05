VERSION = 0.6.6

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

win32 {
DEFINES += BUILDDATE=\\\"$$system('echo %date%')\\\"
} else {
DEFINES += BUILDDATE=\\\"$$system(date '+%d/%m/%y')\\\"
}

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
	src/ColorPicker.cpp \
	src/ComboBoxPenStyle.cpp \
	src/GraphicsPrimitive.cpp \
	src/LayerDialog.cpp \
	src/LayerComboBox.cpp \
	src/LayerItemDelegate.cpp \
	src/LayerToolBarWidget.cpp \
	src/SettingsManager.cpp \
	src/AboutDialog.cpp \
	src/ShortcutsDialog.cpp \
	src/main.cpp \
	src/MainWindow.cpp \
	src/OptionsDialog.cpp \
	src/Sheet.cpp \
	src/SheetView.cpp \
    src/StatusBar.cpp \
	src/ToolBarPrimitive.cpp

HEADERS += \
	src/ColorPicker.h \
	src/ComboBoxPenStyle.h \
	src/GraphicsPrimitive.h \
	src/Layer.h \
	src/LayerItemDelegate.h \
	src/LayerListView.h \
	src/LayerToolBarWidget.h \
	src/LinkLabel.h \
	src/LayerDialog.h \
	src/SettingsManager.h \
	src/LayerComboBox.h \
	src/AboutDialog.h \
	src/MainWindow.h \
    src/OptionsDialog.h \
    src/Sheet.h \
    src/SheetView.h \
	src/ShortcutsDialog.h \
	src/StatusBar.h \
	src/ToolBarPrimitive.h

FORMS += \
	gui/LayerDialog.ui \
	gui/AboutDialog.ui \
    gui/MainWindow.ui \
	gui/OptionsDialog.ui \
	gui/LayerToolBarWidget.ui \
	gui/ShortcutsDialog.ui

CONFIG += lrelease

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
	resources.qrc

win32 {
    RC_ICONS = "resources/main.ico"
}



