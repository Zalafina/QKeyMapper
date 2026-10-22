SOURCES += $$PWD/qkm_ipc_frame.cpp $$PWD/qkm_local_identity.cpp \
    $$PWD/qkm_local_listener.cpp $$PWD/qkm_auth_proof.cpp \
    $$PWD/qkm_pair_store.cpp $$PWD/qkm_session_server.cpp $$PWD/qkm_control_dialog.cpp
HEADERS += $$PWD/qkm_ipc_frame.h $$PWD/qkm_local_identity.h \
    $$PWD/qkm_local_listener.h $$PWD/qkm_auth_proof.h \
    $$PWD/qkm_pair_store.h $$PWD/qkm_session_server.h $$PWD/qkm_control_dialog.h
LIBS += -ladvapi32 -lbcrypt -lcrypt32
