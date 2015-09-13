#-------------------------------------------------
#
# Project created by QtCreator 2015-09-13T12:38:28
#
#-------------------------------------------------

QT       -= gui
QT       += core
TARGET = KlimaLoggBitConverter
TEMPLATE = lib

DEFINES += KLIMALOGGBITCONVERTER_LIBRARY

SOURCES += \
    bitconverter.cpp

HEADERS +=\
    bitconverter.h \
    bitconverter_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
