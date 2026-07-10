# Vendored libdxfrw (see README-eschema.md) - qmake include, not a standalone
# project. $$PWD here is this file's own directory regardless of who
# include()s it, so eSchema.pro just needs one line to pull this in.

INCLUDEPATH += $$PWD/src $$PWD/src/intern

HEADERS += \
    $$PWD/src/drw_base.h \
    $$PWD/src/drw_classes.h \
    $$PWD/src/drw_entities.h \
    $$PWD/src/drw_header.h \
    $$PWD/src/drw_interface.h \
    $$PWD/src/drw_objects.h \
    $$PWD/src/libdwgr.h \
    $$PWD/src/libdxfrw.h \
    $$PWD/src/main_doc.h \
    $$PWD/src/intern/drw_cptable932.h \
    $$PWD/src/intern/drw_cptable936.h \
    $$PWD/src/intern/drw_cptable949.h \
    $$PWD/src/intern/drw_cptable950.h \
    $$PWD/src/intern/drw_cptables.h \
    $$PWD/src/intern/drw_dbg.h \
    $$PWD/src/intern/drw_reserve.h \
    $$PWD/src/intern/drw_textcodec.h \
    $$PWD/src/intern/dwgbuffer.h \
    $$PWD/src/intern/dwgreader.h \
    $$PWD/src/intern/dwgreader15.h \
    $$PWD/src/intern/dwgreader18.h \
    $$PWD/src/intern/dwgreader21.h \
    $$PWD/src/intern/dwgreader24.h \
    $$PWD/src/intern/dwgreader27.h \
    $$PWD/src/intern/dwgreader32.h \
    $$PWD/src/intern/dwgutil.h \
    $$PWD/src/intern/dxfreader.h \
    $$PWD/src/intern/dxfwriter.h \
    $$PWD/src/intern/rscodec.h

SOURCES += \
    $$PWD/src/drw_base.cpp \
    $$PWD/src/drw_classes.cpp \
    $$PWD/src/drw_entities.cpp \
    $$PWD/src/drw_header.cpp \
    $$PWD/src/drw_objects.cpp \
    $$PWD/src/libdwgr.cpp \
    $$PWD/src/libdxfrw.cpp \
    $$PWD/src/intern/drw_dbg.cpp \
    $$PWD/src/intern/drw_textcodec.cpp \
    $$PWD/src/intern/dwgbuffer.cpp \
    $$PWD/src/intern/dwgreader.cpp \
    $$PWD/src/intern/dwgreader15.cpp \
    $$PWD/src/intern/dwgreader18.cpp \
    $$PWD/src/intern/dwgreader21.cpp \
    $$PWD/src/intern/dwgreader24.cpp \
    $$PWD/src/intern/dwgreader27.cpp \
    $$PWD/src/intern/dwgutil.cpp \
    $$PWD/src/intern/dxfreader.cpp \
    $$PWD/src/intern/dxfwriter.cpp \
    $$PWD/src/intern/rscodec.cpp
