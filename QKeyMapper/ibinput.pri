SOURCES += $$PWD/qkm_ibinput_client.cpp $$PWD/qkm_ibinput_popup.cpp
HEADERS += $$PWD/qkm_ibinput_client.h $$PWD/qkm_ibinput_popup.h
!contains(CONFIG, ibinput_disabled) {
    DEFINES += IBINPUT_SUPPORT
    INCLUDEPATH += $$PWD/IbInputSimulator/include
    equals(QT_ARCH, x86_64): QKM_IB_ARCH = x64
    else:equals(QT_ARCH, i386): QKM_IB_ARCH = x86
    else: error("Unsupported IbInputSimulator target architecture")
    QKM_IB_LIBDIR = $$PWD/IbInputSimulator/lib/$$QKM_IB_ARCH
    !exists($$QKM_IB_LIBDIR/IbInputSimulator.lib): error("Missing IbInputSimulator import library; run scripts/build_ibinput_dependency.py")
    LIBS += $$quote($$QKM_IB_LIBDIR/IbInputSimulator.lib)
    isEmpty(DESTDIR) {
        CONFIG(release, debug|release): QKM_IB_DEST = $$OUT_PWD/release
        else: QKM_IB_DEST = $$OUT_PWD/debug
    } else: QKM_IB_DEST = $$absolute_path($$DESTDIR, $$OUT_PWD)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$QKM_IB_LIBDIR/IbInputSimulator.dll)) $$shell_quote($$shell_path($$QKM_IB_DEST/IbInputSimulator.dll)) $$escape_expand(\n\t)
}
