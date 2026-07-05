# eSchema Version
VERSION = 0.6.6

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 # lrelease

INCLUDEPATH += src

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
	src/ColorPicker.cpp \
	src/DialogAbout.cpp \
	src/DialogLayerList.cpp \
	src/DialogOptions.cpp \
	src/DialogShortcuts.cpp \
	src/FidoCadTokenUtils.cpp \
	src/GraphicsPrimitive.cpp \
	src/LayerColorPicker.cpp \
	src/LayerComboBox.cpp \
	src/LayerItemDelegate.cpp \
	src/LayerLabelName.cpp \
	src/LayerList.cpp \
	src/LayerListView.cpp \
	src/LayerListWidgetItem.cpp \
	src/LayerToolBarWidget.cpp \
	src/LayerVisibilityButton.cpp \
	src/ObjectProperties.cpp \
	src/PenStyleComboBox.cpp \
	src/PrimitiveBezier.cpp \
	src/PrimitiveComplexCurve.cpp \
	src/PrimitiveConnection.cpp \
	src/PrimitiveEllipse.cpp \
	src/PrimitiveImage.cpp \
	src/PrimitiveLine.cpp \
	src/PrimitiveMacro.cpp \
	src/PrimitivePad.cpp \
	src/PrimitivePcbTrack.cpp \
	src/PrimitivePolygon.cpp \
	src/PrimitiveRectangle.cpp \
	src/PrimitiveText.cpp \
	src/SettingsManager.cpp \
	src/main.cpp \
	src/MainWindow.cpp \
	src/Sheet.cpp \
	src/SheetView.cpp \
    src/StatusBar.cpp \
	src/ToolBarPrimitive.cpp

HEADERS += \
	src/ColorPicker.h \
	src/DialogAbout.h \
	src/DialogLayerList.h \
	src/DialogOptions.h \
	src/DialogShortcuts.h \
	src/FidoCadTokenUtils.h \
	src/GlobalUtils.h \
	src/GraphicsPrimitive.h \
	src/Layer.h \
	src/LayerColorPicker.h \
	src/LayerItemDelegate.h \
	src/LayerLabelName.h \
	src/LayerList.h \
	src/LayerListView.h \
	src/LayerListWidgetItem.h \
	src/LayerToolBarWidget.h \
	src/LayerVisibilityButton.h \
	src/LinkLabel.h \
	src/ObjectProperties.h \
	src/PenStyleComboBox.h \
	src/PrimitiveArrowUtils.h \
	src/PrimitiveBezier.h \
	src/PrimitiveComplexCurve.h \
	src/PrimitiveConnection.h \
	src/PrimitiveEllipse.h \
	src/PrimitiveImage.h \
	src/PrimitiveLine.h \
	src/PrimitiveMacro.h \
	src/PrimitivePad.h \
	src/PrimitivePcbTrack.h \
	src/PrimitivePolygon.h \
	src/PrimitiveRectangle.h \
	src/PrimitiveText.h \
	src/SettingsManager.h \
	src/LayerComboBox.h \
	src/MainWindow.h \
    src/Sheet.h \
    src/SheetView.h \
	src/StatusBar.h \
	src/ToolBarPrimitive.h

FORMS += \
	gui/DialogAbout.ui \
	gui/DialogLayerList.ui \
	gui/DialogOptions.ui \
	gui/DialogShortcuts.ui \
    gui/MainWindow.ui \
	gui/LayerToolBarWidget.ui

RESOURCES += resources.qrc

win32 {
	RC_ICONS = "resources/main.ico"
	DEFINES += BUILDDATE=\\\"$$system('echo %date%')\\\"
} else {
	DEFINES += BUILDDATE=\\\"$$system(date '+%d/%m/%y')\\\"
}
