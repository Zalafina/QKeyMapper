#-------------------------------------------------
#
# Project created by QtCreator 2017-09-18T08:52:14
#
#-------------------------------------------------

QT  += core gui
QT  += winextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

CONFIG(debug, debug|release){
    DEFINES += DEBUG_LOGOUT_ON
    message("Debug Build")
}

CONFIG(release, debug|release){
    message("Release Build")
}

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Win x86 libs
LIBS        += -L$$PWD/win_lib/x86
LIBS        += User32.lib Psapi.lib
#  Kernel32.Lib AdvAPI32.Lib


SOURCES     += \
    main.cpp \
    qkeymapper.cpp

HEADERS     += \
    qkeymapper.h

FORMS       += \
    qkeymapper.ui

RESOURCES   += \
    image.qrc \
    font.qrc

RC_FILE     += \
    QKeyMapper.rc

DISTFILES   += \
    QKeyMapper.rc
