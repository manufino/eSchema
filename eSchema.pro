QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    optionsdialog.cpp \
    sheet.cpp \
    sheetview.cpp \
    statusbar.cpp \
    toolbarprimitive.cpp

HEADERS += \
    aboutdialog.h \
    mainwindow.h \
    optionsdialog.h \
    sheet.h \
    sheetview.h \
    statusbar.h \
    toolbarprimitive.h

FORMS += \
    aboutdialog.ui \
    mainwindow.ui \
    optionsdialog.ui

TRANSLATIONS += \
    eSchema_it_IT.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
	resources.qrc

DISTFILES += \
	LICENSE
