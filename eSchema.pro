QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	src/aboutdialog.cpp \
	src/main.cpp \
	src/mainwindow.cpp \
	src/optionsdialog.cpp \
	src/sheet.cpp \
	src/sheetview.cpp \
    src/statusbar.cpp \
    src/toolbarprimitive.cpp \
	src/statusbar.cpp

HEADERS += \
	src/aboutdialog.h \
	src/mainwindow.h \
    src/optionsdialog.h \
    src/sheet.h \
    src/sheetview.h \
	src/statusbar.h

FORMS += \
	gui/aboutdialog.ui \
    gui/mainwindow.ui \
	gui/optionsdialog.ui


RC_ICONS = "resources/main.ico"

TRANSLATIONS += \
	language/eSchema_it_IT.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
	resources.qrc

DISTFILES += \
	LICENSE \
	language/eSchema_it_IT.ts
