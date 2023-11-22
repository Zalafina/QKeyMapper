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

#include <math.h>
#include <QJoysticks/VirtualJoystick.h>

VirtualJoystick::VirtualJoystick(QObject *parent)
   : QObject(parent)
{
   m_axisRange = 1;
   m_joystickEnabled = false;
   m_joystick.blacklisted = false;
   m_joystick.name = tr("Virtual Joystick");

   /* Initialize POVs */
   m_joystick.povs.append(0);

   /* Initialize axes */
   m_axisStatus = QVector<AxisState>(NUMBER_OF_AXES, AxisState::STILL);
   m_axisValue = QVector<qint16>(NUMBER_OF_AXES, 0);
   for (int i = 0; i < NUMBER_OF_AXES; ++i)
   {
      m_joystick.axes.append(0);
   }

   /* Initialize buttons */
   for (int i = 0; i < NUMBER_OF_BUTTONS; ++i)
      m_joystick.buttons.append(false);

   m_timerUpdateAxis.reset(new QTimer());
   connect(m_timerUpdateAxis.get(), &QTimer::timeout, this, &VirtualJoystick::updateAxis);
}

/**
 * Returns the current axis range of the joystick.
 * The axis range is an absolute value that represents the maximum value that
 * the joystick can have.
 *
 * For example, the \c QJoystick system supports an axis range of 1 (-1 to 1).
 * If you set an axis range of 0.8 to the virtual joystick, then it will report
 * values ranging from -0.8 to 0.8.
 */
qreal VirtualJoystick::axisRange() const
{
   return m_axisRange;
}

/**
 * Returns \c true if the virtual joystick is enabled.
 */
bool VirtualJoystick::joystickEnabled() const
{
   return m_joystickEnabled;
}

/**
 * Returns a pointer to the virtual josytick device. This can be used if you
 * need to customize the virtual joystick (e.g. add new axes or buttons).
 */
QJoystickDevice *VirtualJoystick::joystick()
{
   return &m_joystick;
}

/**
 * Sets the ID of the virtual joystick device. The \c QJoysticks will
 * automatically change the \a ID of the virtual joystick when it scans for
 * new joysticks.
 *
 * The virtual joystick will ALWAYS be the last joystick to be registered.
 */
void VirtualJoystick::setJoystickID(int id)
{
   m_joystick.id = id;
}

/**
 * Changes the axis range that the joystick can use. For example, if you set
 * an axis range of 0.8, then axis values will be reported from -0.8 to 0.8.
 *
 * If you set an axis range of 1 (maximum), then the joystick will report axis
 * values ranging from -1 to 1.
 */
void VirtualJoystick::setAxisRange(qreal range)
{
   range = fabs(range);

   if (range > 1)
      range = 1;

   m_axisRange = range;
}

/**
 * Enables or disables the virtual joystick device.
 */
void VirtualJoystick::setJoystickEnabled(bool enabled)
{
   if (enabled)
   {
      // That means that the axes will be updated each 10 ms
      m_timerUpdateAxis->start(10);
      qApp->installEventFilter(this);
   }

   else
   {
      m_timerUpdateAxis->stop();
      // Removing the event filter since the joystick is no longer active
      qApp->removeEventFilter(this);
   }

   m_joystickEnabled = enabled;
   emit enabledChanged();
}

void VirtualJoystick::setAxisSensibility(qreal sensibility)
{
   if (sensibility > 1 || sensibility < 0)
   {
      qFatal("Fatal: VirtualJoystick Axis sensibility must be a value between 0 and 1");
   }

   const qint16 stepMinimum = 50;
   const qint16 stepMaximum = 1000;

   m_axisStep = static_cast<qint16>((-stepMaximum + stepMinimum) * sensibility) + stepMaximum;
}

/**
 * Polls the keyboard events and if required, reports a change in the axis
 * values of the virtual joystick device.
 */
void VirtualJoystick::readAxes(int key, bool pressed)
{

   /* Horizontal axis on thumb 1 */
   if (key == Qt::Key_A)
   {
      if (pressed)
         m_axisStatus[AXIS_AD] = DECREASE;
      else
         m_axisStatus[AXIS_AD] = STILL;
   }
   else if (key == Qt::Key_D)
   {
      if (pressed)
         m_axisStatus[AXIS_AD] = INCREASE;
      else
         m_axisStatus[AXIS_AD] = STILL;
   }

   /* Vertical axis on thumb 1 */
   if (key == Qt::Key_S)
   {
      if (pressed)
         m_axisStatus[AXIS_SW] = DECREASE;
      else
         m_axisStatus[AXIS_SW] = STILL;
   }
   else if (key == Qt::Key_W)
   {
      if (pressed)
         m_axisStatus[AXIS_SW] = INCREASE;
      else
         m_axisStatus[AXIS_SW] = STILL;
   }

   /* Trigger 1 */
   if (key == Qt::Key_Q)
   {
      if (pressed)
         m_axisStatus[AXIS_QE] = DECREASE;
      else
         m_axisStatus[AXIS_QE] = STILL;
   }
   else if (key == Qt::Key_E)
   {
      if (pressed)
         m_axisStatus[AXIS_QE] = INCREASE;
      else
         m_axisStatus[AXIS_QE] = STILL;
   }

   /* Trigger 2 */
   if (key == Qt::Key_U)
   {
      if (pressed)
         m_axisStatus[AXIS_UO] = DECREASE;
      else
         m_axisStatus[AXIS_UO] = STILL;
   }
   else if (key == Qt::Key_O)
   {
      if (pressed)
         m_axisStatus[AXIS_UO] = INCREASE;
      else
         m_axisStatus[AXIS_UO] = STILL;
   }

   /* Horizontal axis on thumb 2 */
   if (key == Qt::Key_J)
   {
      if (pressed)
         m_axisStatus[AXIS_JL] = DECREASE;
      else
         m_axisStatus[AXIS_JL] = STILL;
   }
   else if (key == Qt::Key_L)
   {
      if (pressed)
         m_axisStatus[AXIS_JL] = INCREASE;
      else
         m_axisStatus[AXIS_JL] = STILL;
   }

   /* Vertical axis on thumb 2 */
   if (key == Qt::Key_K)
   {
      if (pressed)
         m_axisStatus[AXIS_KI] = DECREASE;
      else
         m_axisStatus[AXIS_KI] = STILL;
   }
   else if (key == Qt::Key_I)
   {
      if (pressed)
         m_axisStatus[AXIS_KI] = INCREASE;
      else
         m_axisStatus[AXIS_KI] = STILL;
   }
}

void VirtualJoystick::updateAxis()
{
   for (quint8 i = 0; i < NUMBER_OF_AXES; i++)
   {
      changeAxisValue(i);
   }
}

/**
 * Polls the keyboard events and if required, reports a change in the POV/hat
 * values of the virtual joystick device.
 */
void VirtualJoystick::readPOVs(int key, bool pressed)
{
   int angle = 0;

   if (key == Qt::Key_Up)
      angle = 360;
   else if (key == Qt::Key_Right)
      angle = 90;
   else if (key == Qt::Key_Left)
      angle = 270;
   else if (key == Qt::Key_Down)
      angle = 180;

   if (!pressed)
      angle = 0;

   if (joystickEnabled())
   {
      QJoystickPOVEvent event;
      event.pov = 0;
      event.angle = angle;
      event.joystick = joystick();

      emit povEvent(event);
   }
}

/**
 * Polls the keyboard events and if required, reports a change in the button
 * values of the virtual joystick device.
 */
void VirtualJoystick::readButtons(int key, bool pressed)
{
   int button = -1;

   if (key == Qt::Key_0)
   { // Special key that reset all axes
      button = 0;
      resetAllAxes();
   }
   else if (key == Qt::Key_1)
      button = 1;
   else if (key == Qt::Key_2)
      button = 2;
   else if (key == Qt::Key_3)
      button = 3;
   else if (key == Qt::Key_4)
      button = 4;
   else if (key == Qt::Key_5)
      button = 5;
   else if (key == Qt::Key_6)
      button = 6;
   else if (key == Qt::Key_7)
      button = 7;
   else if (key == Qt::Key_8)
      button = 8;
   else if (key == Qt::Key_9)
      button = 9;

   if (button != -1 && joystickEnabled())
   {
      QJoystickButtonEvent event;
      event.button = button;
      event.pressed = pressed;
      event.joystick = joystick();

      emit buttonEvent(event);
   }
}

/**
 * Called when the event filter detects a keyboard event.
 *
 * This function prompts the joystick to update its axis, button and POV values
 * based on the keys that have been pressed or released.
 */
void VirtualJoystick::processKeyEvent(QKeyEvent *event, bool pressed)
{
   if (joystickEnabled())
   {
      readPOVs(event->key(), pressed);
      readAxes(event->key(), pressed);
      readButtons(event->key(), pressed);
   }
}

/**
 * "Listens" for keyboard presses or releases while any window or widget of the
 * application is focused.
 *
 * \note This function may or may not detect keyboard events when the
 *       application is not focused. This depends on the operating system and
 *       the window manager that is being used.
 */
bool VirtualJoystick::eventFilter(QObject *object, QEvent *event)
{
   Q_UNUSED(object);

   switch (event->type())
   {
      case QEvent::KeyPress:
         processKeyEvent(static_cast<QKeyEvent *>(event), true);
         break;
      case QEvent::KeyRelease:
         processKeyEvent(static_cast<QKeyEvent *>(event), false);
         break;
      default:
         break;
   }

   return false;
}

void VirtualJoystick::changeAxisValue(quint8 axis)
{
   switch (m_axisStatus[axis])
   {
      case STILL:
         return;

      case INCREASE:
         if (m_axisValue[axis] > AXIS_MAXIMUM_VIRTUAL_JOYSTICK - m_axisStep)
            m_axisValue[axis] = AXIS_MAXIMUM_VIRTUAL_JOYSTICK;
         else
            m_axisValue[axis] += m_axisStep;
         break;

      case DECREASE:
         if (m_axisValue[axis] < AXIS_MINIMUM_VIRTUAL_JOYSTICK + m_axisStep)
            m_axisValue[axis] = AXIS_MINIMUM_VIRTUAL_JOYSTICK;
         else
            m_axisValue[axis] -= m_axisStep;
         break;
   }

   QJoystickAxisEvent event;

   event.axis = axis;
   event.value = m_axisRange * static_cast<qreal>(m_axisValue[axis]) / AXIS_MAXIMUM_VIRTUAL_JOYSTICK;
   event.joystick = joystick();

   emit axisEvent(event);
}

void VirtualJoystick::resetAllAxes()
{
   for (quint8 i = 0; i < NUMBER_OF_AXES; i++)
   {
      m_axisValue[i] = 0;

      QJoystickAxisEvent event;

      event.axis = i;
      event.value = 0;
      event.joystick = joystick();

      emit axisEvent(event);
   }
}
