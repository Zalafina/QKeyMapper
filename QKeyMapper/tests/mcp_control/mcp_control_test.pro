TEMPLATE = app
TARGET = QKeyMapperMcpControlTest
QT += widgets network testlib
CONFIG += console c++17 release force_debug_info
CONFIG -= debug debug_and_release app_bundle
DESTDIR = release
SOURCES += test_mcp_control.cpp
include(../../automation/automation.pri)
DEFINES += QKM_AUTOMATION_TESTING UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00
QMAKE_CXXFLAGS += /utf-8
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'asInvoker\' uiAccess=\'false\'\"
LIBS += -luser32
