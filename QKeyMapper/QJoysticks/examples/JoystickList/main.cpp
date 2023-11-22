/*
 * Copyright (c) 2015-2016 Alex Spataru <alex_spataru@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <QPalette>
#include <QQmlContext>
#include <QApplication>
#include <QStyleFactory>
#include <QQmlApplicationEngine>

#include <QJoysticks.h>

#ifdef Q_OS_WIN
#   ifdef main
#      undef main
#   endif
#endif

void configureDarkStyle()
{
   qApp->setStyle(QStyleFactory::create("Fusion"));
   QPalette darkPalette;
   darkPalette.setColor(QPalette::BrightText, Qt::red);
   darkPalette.setColor(QPalette::WindowText, Qt::white);
   darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
   darkPalette.setColor(QPalette::ToolTipText, Qt::white);
   darkPalette.setColor(QPalette::Text, Qt::white);
   darkPalette.setColor(QPalette::ButtonText, Qt::white);
   darkPalette.setColor(QPalette::HighlightedText, Qt::black);
   darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
   darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
   darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
   darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
   darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
   darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
   qApp->setPalette(darkPalette);
}

int main(int argc, char *argv[])
{
   QApplication app(argc, argv);
   QQmlApplicationEngine qmlEngine;

   /*
    * Make the application use the Fusion style + dark palette
    */
   configureDarkStyle();

   /*
    * QJoysticks is single instance, you can use the "getInstance()" function
    * directly if you want, or you can create a pointer to it to make code
    * easier to read;
    */
   QJoysticks *instance = QJoysticks::getInstance();

   /* Enable the virtual joystick */
   instance->setVirtualJoystickRange(1);
   instance->setVirtualJoystickEnabled(true);
   instance->setVirtualJoystickAxisSensibility(0.7);

   /*
    * Register the QJoysticks with the QML engine, so that the QML interface
    * can easilly use it.
    */
   qmlEngine.rootContext()->setContextProperty("QJoysticks", instance);

   /*
    * Load main.qml and run the application.
    */
   qmlEngine.load(QUrl(QStringLiteral("qrc:/main.qml")));

   return app.exec();
}
