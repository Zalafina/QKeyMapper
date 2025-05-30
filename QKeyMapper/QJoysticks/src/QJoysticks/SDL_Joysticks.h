/*
 * Copyright (c) 2015-2017 Alex Spataru <alex_spataru@outlook.com>
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

#ifndef _QJOYSTICKS_SDL_JOYSTICK_H
#define _QJOYSTICKS_SDL_JOYSTICK_H

#include <SDL.h>
#include <QObject>
#include <QMap>
#include <QJoysticks/JoysticksCommon.h>

/**
 * \brief Translates SDL events into \c QJoysticks events
 *
 * This class is in charge of managing and operating real joysticks through the
 * SDL API. The implementation procedure is the same for every operating system.
 *
 * The only thing that differs from each operating system is the backup mapping
 * applied in the case that we do not know what mapping to apply to a joystick.
 *
 * \note The joystick values are refreshed every 20 milliseconds through a
 *       simple event loop.
 */
class SDL_Joysticks : public QObject
{
   Q_OBJECT

signals:
   void countChanged();
   void joystickAdded(QJoystickDevice *joystick);
   void joystickRemoved(const QJoystickDevice joystick);
   void POVEvent(const QJoystickPOVEvent &event);
   void axisEvent(const QJoystickAxisEvent &event);
   void buttonEvent(const QJoystickButtonEvent &event);
   void sensorEvent(const QJoystickSensorEvent &event);

public:
   SDL_Joysticks(QObject *parent = Q_NULLPTR);
   ~SDL_Joysticks();

   QMap<int, QJoystickDevice *> joysticks();

public slots:
   void rumble(const QJoystickRumble &request);

private slots:
   void update();
   void configureJoystick(const SDL_Event *event);

private:
   QJoystickDevice *getJoystick(int id);
   QJoystickPOVEvent getPOVEvent(const SDL_Event *sdl_event);
   QJoystickAxisEvent getAxisEvent(const SDL_Event *sdl_event);
   QJoystickButtonEvent getButtonEvent(const SDL_Event *sdl_event);
   QJoystickSensorEvent getSensorEvent(const SDL_Event *sdl_event);

   QMap<int, QJoystickDevice *> m_joysticks;
};

#endif
