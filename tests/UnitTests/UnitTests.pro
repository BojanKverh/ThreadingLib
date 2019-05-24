#-------------------------------------------------
#
# Project created by QtCreator 2018-11-13T12:46:08
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = UnitTests
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    unittests.cpp \
    sessionmanager.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../src/release/ -lThreadingLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../src/debug/ -lThreadingLib
else:unix: LIBS += -L$$PWD/../../src/ -lThreadingLib

INCLUDEPATH += $$PWD/../../src
DEPENDPATH += $$PWD/../../src

HEADERS += \
    sessionmanager.h
