#-------------------------------------------------
#
# Project created by QtCreator 2017-09-18T08:52:14
#
#-------------------------------------------------

QT  += core gui
lessThan(QT_MAJOR_VERSION, 6): QT += winextras
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

include(qhotkey.pri)

DEFINES += SINGLE_APPLICATION

#DEFINES += LOGOUT_TOFILE

QMAKE_CXXFLAGS_WARN_ON += -wd4819

contains(DEFINES, SINGLE_APPLICATION) {
    DEFINES += QAPPLICATION_CLASS=QApplication
    QT += network
}

CONFIG(debug, debug|release){
    DEFINES += DEBUG_LOGOUT_ON

    VLD_PATH = $$PWD/../vld-2.5.1
    INCLUDEPATH += $$VLD_PATH/include
    contains(DEFINES, WIN64) {
        message("WIN64 Debug Build")
        LIBS += -L$$VLD_PATH/lib/Win64 -lvld
        LIBS += -L$$VLD_PATH/bin/Win64
    } else {
        message("WIN32 Debug Build")
        LIBS += -L$$VLD_PATH/lib/Win32 -lvld
        LIBS += -L$$VLD_PATH/bin/Win32
    }
}

CONFIG(release, debug|release){
    contains(DEFINES, WIN64) {
        message("WIN64 Release Build")
    } else {
        message("WIN32 Release Build")
    }
}

contains(DEFINES, LOGOUT_TOFILE) {
    DEFINES += DEBUG_LOGOUT_ON
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

#DEFINES += ADJUST_PRIVILEGES

contains(DEFINES, WIN64) {
# Win x64 libs
LIBS        += -L$$PWD/win_lib/x64
} else {
# Win x86 libs
LIBS        += -L$$PWD/win_lib/x86
}
LIBS        += User32.lib Psapi.lib dinput8.lib dxguid.lib
LIBS        += Gdi32.Lib
LIBS        += WinMM.Lib

contains( DEFINES, ADJUST_PRIVILEGES ) {
    LIBS    += AdvAPI32.Lib
}


# UAC for Administrator
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

SOURCES     += \
    main.cpp \
    qkeymapper.cpp \
    qkeymapper_worker.cpp

HEADERS     += \
    qkeymapper.h \
    qkeymapper_worker.h

contains( DEFINES, SINGLE_APPLICATION ) {
    message("SINGLE_APPLICATION Defined!")

    SOURCES +=  \
        singleapp/singleapplication.cpp

    HEADERS +=  \
        singleapp/singleapplication.h
}

FORMS       += \
    qkeymapper.ui

RESOURCES   += \
    image.qrc \
    font.qrc \
    sound.qrc

RC_FILE     += \
    QKeyMapper.rc

DISTFILES   += \
    QKeyMapper.rc
