include(build_identity.pri)
# Confirmed with the generated Qt 5.15.2 Release link line: /DEBUG otherwise drops implicit ICF.
lessThan(QT_MAJOR_VERSION, 6):win32-msvc*:CONFIG(release, debug|release) {
    !contains(QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO, /OPT:ICF): QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO += /OPT:ICF
}
SOURCES += $$PWD/crash_client.cpp $$PWD/crash_monitor.cpp
HEADERS += $$PWD/crash_client.h $$PWD/crash_monitor.h $$PWD/crash_protocol.h

# This checked helper dependency also runs for builds launched directly by Qt Creator.
CONFIG(debug, debug|release): QKM_OUTPUT_CONFIG = debug
else: QKM_OUTPUT_CONFIG = release
QKM_DEPLOY_DIRECTORY = $$OUT_PWD/$$QKM_OUTPUT_CONFIG
!isEmpty(DESTDIR): QKM_DEPLOY_DIRECTORY = $$absolute_path($$DESTDIR, $$OUT_PWD)
qkm_crash_reporter.target = qkm_crash_reporter
qkm_crash_reporter.depends = FORCE
qkm_crash_reporter.commands = powershell.exe -NoProfile -ExecutionPolicy Bypass -File $$shell_quote($$system_path($$PWD/build_crash_reporter.ps1)) -QMakePath $$shell_quote($$system_path($$QMAKE_QMAKE)) -MakePath \"$(MAKE)\" -BuildDirectory $$shell_quote($$system_path($$OUT_PWD/crash-reporter)) -DeployDirectory $$shell_quote($$system_path($$QKM_DEPLOY_DIRECTORY))
QMAKE_EXTRA_TARGETS += qkm_crash_reporter
PRE_TARGETDEPS += qkm_crash_reporter
