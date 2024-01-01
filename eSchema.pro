VERSION = 0.6.2

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

win32 {
DEFINES += BUILDDATE=\\\"$$system('echo %date%')\\\"
} else {
DEFINES += BUILDDATE=\\\"$$system(date '+%d/%m/%y')\\\"
}

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	src/ColorSelector.cpp \
	src/ComboBoxPenStyle.cpp \
	src/SettingsManager.cpp \
	src/AboutDialog.cpp \
	src/main.cpp \
	src/MainWindow.cpp \
	src/OptionsDialog.cpp \
	src/Sheet.cpp \
	src/SheetView.cpp \
    src/StatusBar.cpp \
	src/ToolBarPrimitive.cpp

HEADERS += \
	src/ColorSelector.h \
	src/ComboBoxPenStyle.h \
	src/SettingsManager.h \
	src/LayerComboBox.h \
	src/AboutDialog.h \
	src/MainWindow.h \
    src/OptionsDialog.h \
    src/Sheet.h \
    src/SheetView.h \
	src/StatusBar.h \
	src/ToolBarPrimitive.h

FORMS += \
	gui/AboutDialog.ui \
    gui/MainWindow.ui \
	gui/OptionsDialog.ui

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



