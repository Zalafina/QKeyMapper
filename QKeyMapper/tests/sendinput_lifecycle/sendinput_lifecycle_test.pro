QKM_PRODUCT_ROOT = $$clean_path($$PWD/../..)
include($$QKM_PRODUCT_ROOT/QKeyMapper.pro)
TARGET = QKeyMapperSendInputLifecycleTest
SOURCES -= main.cpp

# Resolve inherited product files before adding this test's own source.
for(qkmFileVariable, $$list(SOURCES HEADERS FORMS RESOURCES RC_FILE DISTFILES TRANSLATIONS)) {
    qkmFiles = $$eval($$qkmFileVariable)
    qkmAbsoluteFiles =
    for(qkmFile, qkmFiles) {
        qkmAbsoluteFiles += $$quote($$absolute_path($$qkmFile, $$QKM_PRODUCT_ROOT))
    }
    eval($$qkmFileVariable = $$qkmAbsoluteFiles)
}
INCLUDEPATH += $$QKM_PRODUCT_ROOT
SOURCES += $$PWD/qkm_sendinput_lifecycle_test.cpp
