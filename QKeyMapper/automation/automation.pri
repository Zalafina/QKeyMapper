SOURCES += $$PWD/qkm_ipc_frame.cpp $$PWD/qkm_local_identity.cpp \
    $$PWD/qkm_local_listener.cpp $$PWD/qkm_auth_proof.cpp \
    $$PWD/qkm_pair_store.cpp $$PWD/qkm_session_server.cpp $$PWD/qkm_control_dialog.cpp \
    $$PWD/qkm_private_storage.cpp $$PWD/qkm_instance_discovery.cpp $$PWD/qkm_source_snapshot.cpp \
    $$PWD/qkm_mapping_help.cpp
HEADERS += $$PWD/qkm_ipc_frame.h $$PWD/qkm_local_identity.h \
    $$PWD/qkm_local_listener.h $$PWD/qkm_auth_proof.h \
    $$PWD/qkm_pair_store.h $$PWD/qkm_session_server.h $$PWD/qkm_control_dialog.h \
    $$PWD/qkm_private_storage.h $$PWD/qkm_instance_discovery.h $$PWD/qkm_source_snapshot.h \
    $$PWD/../qkm_mapping_source_access.h $$PWD/qkm_mapping_help.h
LIBS += -ladvapi32 -lbcrypt -lcrypt32
