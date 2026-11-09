QT = core
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = QKeyMapperIbInputClientTest
DEFINES += IBINPUT_SUPPORT IB_INPUT_STATIC NOMINMAX QKM_IBINPUT_CLIENT_TEST
!contains(CONFIG, ibinput_production_capabilities): DEFINES += QKM_IBINPUT_VALIDATION_PROBE
INCLUDEPATH += $$PWD/../.. $$PWD/../../IbInputSimulator/include
SOURCES += $$PWD/client_test.cpp $$PWD/../../qkm_ibinput_client.cpp
HEADERS += $$PWD/../../qkm_ibinput_client.h
QMAKE_CXXFLAGS += /utf-8

SOURCES += $$PWD/../../automation/qkm_demo_mode.cpp
LIBS += user32.lib
