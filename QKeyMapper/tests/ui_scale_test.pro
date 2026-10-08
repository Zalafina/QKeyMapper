TEMPLATE = app
TARGET = QKeyMapperUiScaleTest
QT += widgets
CONFIG += console c++17 release force_debug_info
CONFIG -= debug debug_and_release app_bundle
DESTDIR = release
SOURCES += qkm_ui_scale_test.cpp ../qkm_ui_scale.cpp
HEADERS += ../qkm_ui_scale.h ../qkeymapper_qt_compat.h
QMAKE_CXXFLAGS += /utf-8
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'asInvoker\' uiAccess=\'false\'\"
