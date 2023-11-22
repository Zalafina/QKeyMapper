/*
 * Copyright (c) 2016 Alex Spataru <alex_spataru@outlook.com>
 *
 * This file is part of the LibDS, which is released under the MIT license.
 * For more information, please read the LICENSE file in the root directory
 * of this project.
 */

#ifndef QJOYSTICKS_TEST_H
#define QJOYSTICKS_TEST_H

#include <QtTest>
#include <QJoysticks.h>

class Test_QJoysticks : public QObject
{
   Q_OBJECT

private slots:
   void initTestCase()
   {
      /* Get the QJoysticks instance */
      joysticks = QJoysticks::getInstance();

      /* Configure our test device */
      QJoystickDevice device;
      device.name = "Test device";
//      device.numAxes = 6;
//      device.numPOVs = 0;
//      device.numButtons = 12;
      device.blacklisted = false;
   }

   void checkJoystickCount()
   {
      /* Clear joysticks & register only 1 joystick */
      joysticks->resetJoysticks();
      joysticks->addInputDevice(&device);

      /* Verify that joystick count is 1 */
      QVERIFY(joysticks->count() == 1);
   }

   void checkJoystickProperties()
   {
      joysticks->resetJoysticks();
      joysticks->addInputDevice(&device);

      /* Joystick properties should remain the same */
      QVERIFY(joysticks->getName(0) == device.name);
      QVERIFY(joysticks->getNumAxes(0) == device.axes.count());
      QVERIFY(joysticks->getNumPOVs(0) == device.povs.count());
      QVERIFY(joysticks->getNumButtons(0) == device.buttons.count());
   }

   void checkBlacklistedCount()
   {
      /* Add virtual joystick & blacklist it */
      joysticks->resetJoysticks();
      joysticks->setVirtualJoystickEnabled(true);
      joysticks->setBlacklisted(0, true);

      /* Check that blacklisted joysticks have been updated */
      QVERIFY(joysticks->nonBlacklistedCount() < joysticks->count());
   }

   void checkDiagnosticFunctions()
   {
      joysticks->resetJoysticks();
      joysticks->addInputDevice(&device);

      /* Joystick 0 must exist */
      QVERIFY(joysticks->joystickExists(0) == true);

      /* Joystick 1 should not exist */
      QVERIFY(joysticks->joystickExists(1) == false);
   }

   void checkPointers()
   {
      /* Check pointers to interfaces */
      QVERIFY(joysticks->sdlJoysticks());
      QVERIFY(joysticks->virtualJoystick());

      /* Add a joystick */
      joysticks->resetJoysticks();
      joysticks->addInputDevice(&device);

      /* Check that pointers returned by QJoysticks are appropiate */
      QVERIFY(joysticks->getInputDevice(0) != Q_NULLPTR);
      QVERIFY(joysticks->getInputDevice(1) == Q_NULLPTR);

      /* Verify that joystick pointers still remain the same */
      QVERIFY(joysticks->getInputDevice(0) == &device);
   }

   void verifyCrashAvoidance()
   {
      joysticks->resetJoysticks();

      /* Try to access invalid joystick and see what happens */
      qDebug() << joysticks->getName(2);
      qDebug() << joysticks->getNumAxes(2);
      qDebug() << joysticks->getNumPOVs(2);
      qDebug() << joysticks->getNumButtons(2);
      qDebug() << joysticks->isBlacklisted(2);
      qDebug() << joysticks->getInputDevice(2);
   }

private:
   QJoysticks *joysticks;
   QJoystickDevice device;
};

#endif
