#-------------------------------------------------
#
# Project created by QtCreator 2017-06-26T21:11:23
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets



TARGET = AwBitRate
TEMPLATE = app

SOURCES += main.cpp\
        awbitrate.cpp \
    dcrbsthread.cpp \
    decoder/mpeg2frame.cpp \
    decoder/avsframe.cpp \
    decoder/hevcframe.cpp

HEADERS  += awbitrate.h \
    dcrbsthread.h \
    include/infor.h \
    decoder/mpeg2frame.h \
    decoder/avsframe.h \
    decoder/hevcframe.h

FORMS    += awbitrate.ui
CONFIG   += qaxcontainer

RC_FILE += myico.rc
