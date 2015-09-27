#-------------------------------------------------
#
# Project created by QtCreator 2015-09-12T21:14:58
#
#-------------------------------------------------

QT       += core gui sql

CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = KlimaLoggProOnBBB
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        qcustomplot.cpp \
        kldatabase.cpp \
        bitconverter.cpp \
        readdataworker.cpp

HEADERS  += mainwindow.h \
        qcustomplot.h \
        kldatabase.h \
        bitconverter.h \
        definitions.h \
        readdataworker.h

FORMS    += mainwindow.ui

INCLUDEPATH += /opt/crosstools/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/arm-linux-gnueabihf/libc/usr/include
INCLUDEPATH += /opt/embedded/bbb/rootfs/usr/include
INCLUDEPATH += /opt/embedded/bbb/rootfs/usr/local/include


LIBS += -L/opt/embedded/bbb/rootfs/lib
LIBS += -L/opt/embedded/bbb/rootfs/usr/lib
LIBS += -L/opt/embedded/bbb/rootfs/usr/local/lib

LIBS += -L/opt/embedded/bbb/rootfs/usr/local/qt-5.3/lib -lz -lpthread -lm -lqwt -lQt5Gui -lGLES_CM -lGLESv2 -lusc

target.path = /usr/local/bin
INSTALLS += target

