QKM_PRODUCT_ROOT = $$clean_path($$PWD/../..)
include($$QKM_PRODUCT_ROOT/QKeyMapper.pro)
TARGET = QKeyMapperForzaAutoStateTest
CONFIG += console
LIBS += Shell32.lib
SOURCES -= main.cpp qkeymapper_worker.cpp
RC_FILE =
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'asInvoker\' uiAccess=\'false\'\"

# Compile the real worker in the test translation unit to reach its private state.
for(qkmFileVariable, $$list(SOURCES HEADERS FORMS RESOURCES DISTFILES TRANSLATIONS)) {
    qkmFiles = $$eval($$qkmFileVariable)
    qkmAbsoluteFiles =
    for(qkmFile, qkmFiles) {
        qkmAbsoluteFiles += $$quote($$absolute_path($$qkmFile, $$QKM_PRODUCT_ROOT))
    }
    eval($$qkmFileVariable = $$qkmAbsoluteFiles)
}
INCLUDEPATH += $$QKM_PRODUCT_ROOT
SOURCES += $$PWD/qkm_forza_auto_state_test.cpp
