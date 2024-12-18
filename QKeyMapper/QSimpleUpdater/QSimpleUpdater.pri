#
# Copyright (c) 2014-2021 Alex Spataru <https://github.com/alex-spataru>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

QT += gui
QT += core
QT += network
QT += widgets

DEFINES += QSU_INCLUDE_MOC=1
INCLUDEPATH += $$PWD/include

SOURCES += \
    $$PWD/src/Updater.cpp \
    $$PWD/src/Downloader.cpp \
    $$PWD/src/AuthenticateDialog.cpp \
    $$PWD/src/QSimpleUpdater.cpp

HEADERS += \
    $$PWD/include/QSimpleUpdater.h \
    $$PWD/src/Updater.h \
    $$PWD/src/AuthenticateDialog.h \
    $$PWD/src/Downloader.h

FORMS += \
    $$PWD/src/Downloader.ui \
    $$PWD/src/AuthenticateDialog.ui
RESOURCES += $$PWD/etc/resources/qsimpleupdater.qrc
