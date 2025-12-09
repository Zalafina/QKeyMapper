#-------------------------------------------------
#
# Project created by QtCreator 2017-09-18T08:52:14
#
#-------------------------------------------------

QT  += core gui network
CONFIG += c++17
lessThan(QT_MAJOR_VERSION, 6): QT += winextras
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

# include(qhotkey.pri)
include(QJoysticks/QJoysticks.pri)
include(QSimpleUpdater/QSimpleUpdater.pri)

DEFINES += SINGLE_APPLICATION

# DEFINES += LOGOUT_TOFILE
# DEFINES += USE_SAOFONT
# DEFINES += DINPUT_TEST
# DEFINES += VLD_DETECT
# DEFINES += JOYSTICK_VERBOSE_LOG
# DEFINES += EXTRACTICON_VERBOSE_LOG
# DEFINES += GAMECONTROLLER_SENSOR_VERBOSE_LOG
# DEFINES += MOUSE_VERBOSE_LOG
# DEFINES += GRIP_VERBOSE_LOG
# DEFINES += MOUSEBUTTON_CONVERT
# DEFINES += USE_CUSTOMSTYLE
DEFINES += DISPLAY_SWITCHKEY_MINIMIZED
# DEFINES += ENABLE_SYSTEMFILTERKEYS_DEFAULT
DEFINES += CLOSE_SETUPDIALOG_ONDATACHANGED
DEFINES += USE_QTRANSLATOR
# DEFINES += SETTINGSFILE_CONVERT
# DEFINES += SEPARATOR_CONVERT
DEFINES += VIGEM_CLIENT_SUPPORT
DEFINES += FAKERINPUT_SUPPORT
DEFINES += HOOKSTART_ONSTARTUP
DEFINES += CYCLECHECKTIMER_ENABLED
DEFINES += AUTO_REFRESH_PROCESSINFOLIST
DEFINES += PASTETEXT_RESTORE_CLIPBOARD
# DEFINES += USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
# DEFINES += SDL_VIRTUALGAMEPAD_IGNORE
# DEFINES += INTERCEPTION_VERBOSE_LOG

lessThan(QT_MAJOR_VERSION, 6) {
    message("Qt5 Version")
    win32-msvc*: {
        QMAKE_CFLAGS *= /utf-8
        QMAKE_CXXFLAGS *= /utf-8
    }
}
else {
    message("Qt6 Version")
}

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

contains(DEFINES, WIN64) {
# Win x64 libs
LIBS        += -L$$PWD/win_lib/x64
} else {
# Win x86 libs
LIBS        += -L$$PWD/win_lib/x86
}
LIBS        += User32.lib
# LIBS        += Psapi.lib
# LIBS        += dxguid.lib
LIBS        += Gdi32.Lib
LIBS        += WinMM.Lib
LIBS        += dwmapi.lib
LIBS        += Version.Lib
LIBS        += SDL2.lib
LIBS        += AdvAPI32.Lib
LIBS        += powrprof.lib
LIBS        += SetupAPI.Lib
LIBS        += WtsApi32.Lib
LIBS        += gdiplus.lib
LIBS        += Ole32.lib
contains( DEFINES, DINPUT_TEST ) {
    LIBS    += dinput8.lib
}

contains( DEFINES, VIGEM_CLIENT_SUPPORT ) {
    contains(DEFINES, WIN64) {
    # ViGEmClient x64 dll library
    LIBS        += -L$$PWD/ViGEm/lib/x64
    } else {
    # ViGEmClient x86 dll library
    LIBS        += -L$$PWD/ViGEm/lib/x86
    }

    LIBS    += ViGEmClient.lib

    INCLUDEPATH += $$PWD/ViGEm/include

    HEADERS     += \
        ViGEm\include\ViGEm\Client.h \
        ViGEm\include\ViGEm\Common.h \
        ViGEm\include\ViGEm\Util.h
}


# Interception Driver Support >>>
contains(DEFINES, WIN64) {
# Interception x64 dll library
LIBS        += -L$$PWD/Interception/lib/x64
LIBS        += -L$$PWD/libusb/lib/x64
LIBS        += -L$$PWD/FakerInput/lib/x64
} else {
# Interception x86 dll library
LIBS        += -L$$PWD/Interception/lib/x86
LIBS        += -L$$PWD/libusb/lib/x86
LIBS        += -L$$PWD/FakerInput/lib/x86
}
LIBS        += interception.lib
LIBS        += libusb-1.0.lib
LIBS        += FakerInputDll.lib

INCLUDEPATH += $$PWD/Interception/include
INCLUDEPATH += $$PWD/libusb/include
INCLUDEPATH += $$PWD/FakerInput/include
INCLUDEPATH += $$PWD/QSimpleUpdater/include
INCLUDEPATH += $$PWD/orderedmap
INCLUDEPATH += $$PWD/GamepadMotion

HEADERS     += \
    GamepadMotion/GamepadMotion.hpp \
    Interception/include/interception.h \
    colorpickerwidget.h \
    libusb/include/libusb.h \
    FakerInput/include/fakerinputclient.h \
    orderedmap/orderedmap.h \
    qcrosshairsetupdialog.h \
    qfloatingwindowsetupdialog.h \
    qgyro2mouseoptiondialog.h \
    qignorewindowinfolistdialog.h \
    qitemsetupdialog.h \
    qkeyrecord.h \
    qmacrolistdialog.h \
    qmappingadvanceddialog.h \
    qnotificationsetupdialog.h \
    qstartuppositiondialog.h \
    qtablesetupdialog.h \
    qtrayiconselectdialog.h \
    volumecontroller.h
# Interception Driver Support <<<


# UAC for Administrator
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

SOURCES     += \
    colorpickerwidget.cpp \
    interception_worker.cpp \
    main.cpp \
    qcrosshairsetupdialog.cpp \
    qfloatingwindowsetupdialog.cpp \
    qgyro2mouseoptiondialog.cpp \
    qignorewindowinfolistdialog.cpp \
    qinputdevicelistwindow.cpp \
    qitemsetupdialog.cpp \
    qkeymapper.cpp \
    qkeymapper_worker.cpp \
    qkeyrecord.cpp \
    qmacrolistdialog.cpp \
    qmappingadvanceddialog.cpp \
    qnotificationsetupdialog.cpp \
    qstartuppositiondialog.cpp \
    qtablesetupdialog.cpp \
    qtrayiconselectdialog.cpp \
    volumecontroller.cpp

HEADERS     += \
    interception_worker.h \
    qinputdevicelistwindow.h \
    qkeymapper.h \
    qkeymapper_worker.h \
    qkeymapper_constants.h

contains( DEFINES, SINGLE_APPLICATION ) {
    message("SINGLE_APPLICATION Defined!")

    SOURCES +=  \
        singleapp/singleapplication.cpp

    HEADERS +=  \
        singleapp/singleapplication.h
}

FORMS       += \
    qcrosshairsetupdialog.ui \
    qfloatingwindowsetupdialog.ui \
    qgyro2mouseoptiondialog.ui \
    qignorewindowinfolistdialog.ui \
    qinputdevicelistwindow.ui \
    qitemsetupdialog.ui \
    qkeymapper.ui \
    qkeyrecord.ui \
    qmacrolistdialog.ui \
    qmappingadvanceddialog.ui \
    qnotificationsetupdialog.ui \
    qstartuppositiondialog.ui \
    qtablesetupdialog.ui \
    qtrayiconselectdialog.ui

RESOURCES   += \
    image.qrc \
    sound.qrc \
    usb-ids.qrc

contains( DEFINES, USE_QTRANSLATOR ) {
RESOURCES   += translations.qrc
}

contains( DEFINES, USE_SAOFONT ) {
RESOURCES   += font.qrc
}

RC_FILE     += \
    QKeyMapper.rc

DISTFILES   += \
    QKeyMapper.rc \
    ../README*.md

contains( DEFINES, USE_QTRANSLATOR ) {
TRANSLATIONS += \
    translations/QKeyMapper_en_US.ts \
    translations/QKeyMapper_zh_CN.ts \
    translations/QKeyMapper_ja_JP.ts
}
