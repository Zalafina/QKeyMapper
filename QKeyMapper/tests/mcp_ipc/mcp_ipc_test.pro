TEMPLATE = app
TARGET = QKeyMapperMcpIpcTest
QT = core network testlib
CONFIG += console c++17 release force_debug_info
CONFIG -= debug debug_and_release app_bundle
DESTDIR = release
SOURCES += test_mcp_ipc.cpp \
    ../../automation/qkm_ipc_frame.cpp \
    ../../automation/qkm_local_identity.cpp \
    ../../automation/qkm_local_listener.cpp
HEADERS += ../../automation/qkm_ipc_frame.h ../../automation/qkm_local_identity.h \
    ../../automation/qkm_local_listener.h
DEFINES += UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00
QMAKE_CXXFLAGS += /utf-8
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'asInvoker\' uiAccess=\'false\'\"
LIBS += -ladvapi32
