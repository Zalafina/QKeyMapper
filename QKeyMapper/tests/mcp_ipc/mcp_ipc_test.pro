TEMPLATE = app
TARGET = QKeyMapperMcpIpcTest
QT = core network testlib
CONFIG += console c++17 release force_debug_info
CONFIG -= debug debug_and_release app_bundle
DESTDIR = release
SOURCES += test_mcp_ipc.cpp \
    ../../automation/qkm_ipc_frame.cpp \
    ../../automation/qkm_local_identity.cpp \
    ../../automation/qkm_local_listener.cpp \
    ../../automation/qkm_auth_proof.cpp \
    ../../automation/qkm_pair_store.cpp \
    ../../automation/qkm_session_server.cpp \
    ../../automation/qkm_private_storage.cpp \
    ../../automation/qkm_instance_discovery.cpp
HEADERS += ../../automation/qkm_ipc_frame.h ../../automation/qkm_local_identity.h \
    ../../automation/qkm_local_listener.h ../../automation/qkm_auth_proof.h \
    ../../automation/qkm_pair_store.h ../../automation/qkm_session_server.h \
    ../../automation/qkm_private_storage.h ../../automation/qkm_instance_discovery.h
DEFINES += UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00
QMAKE_CXXFLAGS += /utf-8
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'asInvoker\' uiAccess=\'false\'\"
LIBS += -ladvapi32 -lbcrypt -lcrypt32
contains(CONFIG, mcp_fault_injection): DEFINES += QKM_AUTOMATION_TESTING
CONFIG(asan):win32-msvc* {
    QMAKE_CFLAGS += -fsanitize=address
    QMAKE_CXXFLAGS += -fsanitize=address /bigobj
    QMAKE_LFLAGS += /INCREMENTAL:NO
    DEFINES += QKM_ENABLE_ASAN
}
