#-------------------------------------------------
#
# Project created by QtCreator 2015-09-19T08:24:54
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = KlimaLoggReader
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

#/opt/embedded/bbb/rootfs/usr/local/lib/

SOURCES += main.cpp \
    ../../../Qt/KlimaLoggProOnBBB/bitconverter.cpp

LIBS += -L/opt/embedded/bbb/rootfs/usr/local/lib/ -lts

#LIBS += -L$$OUT_PWD/../KlimaLoggBitConverter/ -lKlimaLoggBitConverter

INCLUDEPATH += $$PWD/../KlimaLoggBitConverter
INCLUDEPATH += ../../../Qt/KlimaLoggProOnBBB
#DEPENDPATH += $$PWD/../KlimaLoggBitConverter

HEADERS += \
    ../../../Qt/KlimaLoggProOnBBB/bitconverter.h \
    ../../../Qt/KlimaLoggProOnBBB/definitions.h
