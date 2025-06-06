QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

UI_DIR=./build
OBJECTS_DIR=./build
MOC_DIR=./build
RCC_DIR=./build
DESTDIR=./bin

RC_FILE = MClipboard.rc

SOURCES += \
    src/DBOperate.cpp \
    src/Dialog.cpp \
    src/main.cpp

HEADERS += \
    src/DBOperate.h \
    src/Dialog.h

FORMS += \
    src/Dialog.ui

RESOURCES += \
    MClipboard.qrc

DISTFILES += \
    bin/MClipboard.qss
