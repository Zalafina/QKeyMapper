QT = core gui widgets
CONFIG += c++17
CONFIG -= console app_bundle
TEMPLATE = app
TARGET = QKeyMapperIbInputKeyboardReceiver
SOURCES += $$PWD/keyboard_receiver.cpp
QMAKE_CXXFLAGS += /utf-8
LIBS += user32.lib
