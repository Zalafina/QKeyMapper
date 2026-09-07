TEMPLATE = app
TARGET = QKeyMapperDiagnosticsTest
CONFIG -= qt debug debug_and_release app_bundle
CONFIG += release c++17 console force_debug_info
DESTDIR = release
DEFINES += UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0601
SOURCES += diagnostics_test.cpp ../crash_client.cpp
HEADERS += ../crash_client.h ../crash_protocol.h ../crash_files.h ../hang_monitor.h
include(../build_identity.pri)
QMAKE_CXXFLAGS += /utf-8
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'asInvoker\' uiAccess=\'false\'\"
