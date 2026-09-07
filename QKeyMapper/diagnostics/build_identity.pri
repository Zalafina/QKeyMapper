QKM_REVISION = $$system(git -C $$system_quote($$PWD/../..) rev-parse HEAD)
isEmpty(QKM_REVISION): QKM_REVISION = unknown
QKM_DIRTY = $$system(git -C $$system_quote($$PWD/../..) status --porcelain)
!isEmpty(QKM_DIRTY): QKM_REVISION = $${QKM_REVISION}-dirty
QKM_IDENTITY_LINES = "$${LITERAL_HASH}ifndef QKM_BUILD_IDENTITY_H"
QKM_IDENTITY_LINES += "$${LITERAL_HASH}define QKM_BUILD_IDENTITY_H"
QKM_IDENTITY_LINES += "$${LITERAL_HASH}define QKM_SOURCE_REVISION \"$${QKM_REVISION}\""
QKM_IDENTITY_LINES += "$${LITERAL_HASH}endif"
!write_file($$OUT_PWD/qkm_build_identity.h, QKM_IDENTITY_LINES): error(Cannot write diagnostic build identity)
INCLUDEPATH += $$OUT_PWD
