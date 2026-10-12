TEMPLATE = app
TARGET = QKeyMapperDiagnosticsTest
CONFIG -= qt debug debug_and_release app_bundle
CONFIG += release c++17 console force_debug_info
DESTDIR = release
DEFINES += UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0601
SOURCES += diagnostics_test.cpp ../../diagnostics/crash_client.cpp
HEADERS += ../../diagnostics/crash_client.h ../../diagnostics/crash_protocol.h ../../diagnostics/crash_files.h ../../diagnostics/hang_monitor.h
include(../../diagnostics/build_identity.pri)
QMAKE_CXXFLAGS += /utf-8
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'asInvoker\' uiAccess=\'false\'\"
