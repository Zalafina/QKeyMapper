TEMPLATE = app
TARGET = QKeyMapperCrashReporter
DESTDIR = release
CONFIG -= qt debug debug_and_release
CONFIG += release c++17 windows force_debug_info
DEFINES += UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601
SOURCES += crash_reporter.cpp
HEADERS += crash_protocol.h crash_files.h hang_monitor.h
LIBS += Shell32.lib Version.lib
contains(QMAKE_TARGET.arch, x86_64) {
    LIBS += $$quote($$PWD/../win_lib/x64/DbgHelp.Lib)
} else:contains(QMAKE_TARGET.arch, x86) {
    LIBS += $$quote($$PWD/../win_lib/x86/DbgHelp.Lib)
} else {
    error(Unsupported crash reporter architecture: $$QMAKE_TARGET.arch)
}
QMAKE_CFLAGS += /utf-8
QMAKE_CXXFLAGS += /utf-8
# The native helper must not add a Qt or redistributable CRT deployment variant.
QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO -= -MD /MD
QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO -= -MD /MD
QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO += -MT
QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO += -MT
# Qt 5's generated /DEBUG link line lacks ICF; Qt 6 already supplies it.
!contains(QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO, /OPT:ICF): QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO += /OPT:ICF
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'asInvoker\' uiAccess=\'false\'\"
