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

#include <QDebug>
#include <QSettings>
#include <QJoysticks.h>
#include <QJoysticks/SDL_Joysticks.h>
#ifdef VIRTUALJOYSTICK
#include <QJoysticks/VirtualJoystick.h>
#endif

QJoysticks::QJoysticks()
{
   /* Initialize input methods */
   m_sdlJoysticks = new SDL_Joysticks(this);
#ifdef VIRTUALJOYSTICK
   m_virtualJoystick = new VirtualJoystick(this);
#endif

   /* Configure SDL joysticks */
   connect(sdlJoysticks(), &SDL_Joysticks::POVEvent, this, &QJoysticks::POVEvent);
   connect(sdlJoysticks(), &SDL_Joysticks::axisEvent, this, &QJoysticks::axisEvent);
   connect(sdlJoysticks(), &SDL_Joysticks::buttonEvent, this, &QJoysticks::buttonEvent);
   connect(sdlJoysticks(), &SDL_Joysticks::sensorEvent, this, &QJoysticks::sensorEvent);
   connect(sdlJoysticks(), &SDL_Joysticks::countChanged, this, &QJoysticks::updateInterfaces);
   connect(sdlJoysticks(), &SDL_Joysticks::joystickAdded, this, &QJoysticks::onJoystickAdded);
   connect(sdlJoysticks(), &SDL_Joysticks::joystickRemoved, this, &QJoysticks::onJoystickRemoved);

#ifdef VIRTUALJOYSTICK
   /* Configure virtual joysticks */
   connect(virtualJoystick(), &VirtualJoystick::povEvent, this, &QJoysticks::POVEvent);
   connect(virtualJoystick(), &VirtualJoystick::axisEvent, this, &QJoysticks::axisEvent);
   connect(virtualJoystick(), &VirtualJoystick::buttonEvent, this, &QJoysticks::buttonEvent);
   connect(virtualJoystick(), &VirtualJoystick::enabledChanged, this, &QJoysticks::updateInterfaces);
#endif

   /* React to own signals to create QML signals */
   connect(this, &QJoysticks::POVEvent, this, &QJoysticks::onPOVEvent);
   connect(this, &QJoysticks::axisEvent, this, &QJoysticks::onAxisEvent);
   connect(this, &QJoysticks::buttonEvent, this, &QJoysticks::onButtonEvent);

   connect(this, &QJoysticks::setGameControllersSensorEnabled_signal, this, &QJoysticks::setGameControllersSensorEnabled);

   /* Configure the settings */
   m_sortJoyticks = 0;
   // m_settings = new QSettings(qApp->organizationName(), qApp->applicationName());
   // m_settings->beginGroup("Blacklisted Joysticks");
}

QJoysticks::~QJoysticks()
{
   // delete m_settings;
   delete m_sdlJoysticks;
#ifdef VIRTUALJOYSTICK
   delete m_virtualJoystick;
#endif
}

/**
 * Returns the one and only instance of this class
 */
QJoysticks *QJoysticks::getInstance()
{
   static QJoysticks joysticks;
   return &joysticks;
}

/**
 * Returns the number of joysticks that are attached to the computer and/or
 * registered with the \c QJoysticks system.
 *
 * \note This count also includes the virtual joystick (if its enabled)
 */
int QJoysticks::count() const
{
   return inputDevices().count();
}

/**
 * Returns the number of joysticks that are not blacklisted.
 * This can be considered the "effective" number of joysticks.
 */
int QJoysticks::nonBlacklistedCount()
{
   int cnt = count();

   for (int i = 0; i < count(); ++i)
      if (isBlacklisted(i))
         --cnt;

   return cnt;
}

/**
 * Returns a list with the names of all registered joystick.
 *
 * \note This list also includes the blacklisted joysticks
 * \note This list also includes the virtual joystick (if its enabled)
 */
QStringList QJoysticks::deviceNames() const
{
   QStringList names;

   foreach (QJoystickDevice *joystick, inputDevices())
      names.append(joystick->name);

   return names;
}

/**
 * Returns the POV value for the given joystick \a index and \a pov ID
 */
int QJoysticks::getPOV(const int index, const int pov)
{
   if (joystickExists(index))
      return getInputDevice(index)->povs.at(pov);

   return -1;
}

/**
 * Returns the axis value for the given joystick \a index and \a axis ID
 */
double QJoysticks::getAxis(const int index, const int axis)
{
   if (joystickExists(index))
      return getInputDevice(index)->axes.at(axis);

   return 0;
}

/**
 * Returns the button value for the given joystick \a index and \a button ID
 */
bool QJoysticks::getButton(const int index, const int button)
{
   if (joystickExists(index))
      return getInputDevice(index)->buttons.at(button);

   return false;
}

/**
 * Returns the number of axes that the joystick at the given \a index has.
 */
int QJoysticks::getNumAxes(const int index)
{
   if (joystickExists(index))
      return getInputDevice(index)->axes.count();

   return -1;
}

/**
 * Returns the number of POVs that the joystick at the given \a index has.
 */
int QJoysticks::getNumPOVs(const int index)
{
   if (joystickExists(index))
      return getInputDevice(index)->povs.count();

   return -1;
}

/**
 * Returns the number of buttons that the joystick at the given \a index has.
 */
int QJoysticks::getNumButtons(const int index)
{
   if (joystickExists(index))
      return getInputDevice(index)->buttons.count();

   return -1;
}

/**
 * Returns \c true if the joystick at the given \a index is blacklisted.
 */
bool QJoysticks::isBlacklisted(const int index)
{
   if (joystickExists(index))
      return inputDevices().at(index)->blacklisted;

   return true;
}

/**
 * Returns \c true if the joystick at the given \a index is valid, otherwise,
 * the function returns \c false and warns the user through the console.
 */
bool QJoysticks::joystickExists(const int index)
{
   return (index >= 0) && (count() > index);
}

/**
 * Returns the index of the joystick with the given \a instance_id if it exists,
 * otherwise returns -1.
 *
 * \param instance_id The SDL instance ID of the joystick to search for.
 * \return The index of the joystick in the device list if found, otherwise -1.
 */
int QJoysticks::instanceIDExists(const int instance_id)
{
    QList<QJoystickDevice *> devices = inputDevices();
    for (int i = 0; i < devices.size(); ++i) {
        if (devices.at(i)->instanceID == instance_id)
            return i;
    }
    return -1;
}

/**
 * Returns the name of the given joystick
 */
QString QJoysticks::getName(const int index)
{
   if (joystickExists(index))
      return m_devices.at(index)->name;

   return "Invalid Joystick";
}

/**
 * Returns a pointer to the SDL joysticks system.
 * This can be used if you need to get more information regarding the joysticks
 * registered and managed with SDL.
 */
SDL_Joysticks *QJoysticks::sdlJoysticks() const
{
   return m_sdlJoysticks;
}

#ifdef VIRTUALJOYSTICK
/**
 * Returns a pointer to the virtual joystick system.
 * This can be used if you need to get more information regarding the virtual
 * joystick or want to change its properties directly.
 *
 * \note You can also change the properties of the virtual joysticks using the
 *       functions of the \c QJoysticks system class
 */
VirtualJoystick *QJoysticks::virtualJoystick() const
{
   return m_virtualJoystick;
}
#endif

/**
 * Returns a pointer to the device at the given \a index.
 */
QJoystickDevice *QJoysticks::getInputDevice(const int index)
{
   if (joystickExists(index))
      return inputDevices().at(index);

   return Q_NULLPTR;
}

/**
 * Returns a pointer to a list containing all registered joysticks.
 * This can be used for advanced hacks or just to get all properties of each
 * joystick.
 */
QList<QJoystickDevice *> QJoysticks::inputDevices() const
{
   return m_devices;
}

/**
 * If \a sort is set to true, then the device list will put all blacklisted
 * joysticks at the end of the list
 */
void QJoysticks::setSortJoysticksByBlacklistState(bool sort)
{
   if (m_sortJoyticks != sort)
   {
      m_sortJoyticks = sort;
      updateInterfaces();
   }
}

/**
 * Blacklists or whitelists the joystick at the given \a index.
 *
 * \note This function does not have effect if the given joystick does not exist
 * \note Once the joystick is blacklisted, the joystick list will be updated
 */
void QJoysticks::setBlacklisted(const int index, bool blacklisted)
{
   Q_ASSERT(joystickExists(index));

    if (!joystickExists(index)) {
        return;
    }

   /* Netrualize the joystick */
   if (blacklisted)
   {
      for (int i = 0; i < getNumAxes(index); ++i)
         emit axisChanged(index, i, 0);

      for (int i = 0; i < getNumButtons(index); ++i)
         emit buttonChanged(index, i, false);

      for (int i = 0; i < getNumPOVs(index); ++i)
         emit povChanged(index, i, 0);
   }

   /* See if blacklist value was actually changed */
   bool changed = m_devices.at(index)->blacklisted != blacklisted;

   /* Save settings */
   m_devices.at(index)->blacklisted = blacklisted;
   // m_settings->setValue(getName(index), blacklisted);

   /* Re-scan joysticks if blacklist value has changed */
   // if (changed)
   //    updateInterfaces();
   Q_UNUSED(changed);
}

void QJoysticks::setGameControllersSensorEnabled(bool enabled)
{
    foreach (QJoystickDevice *joystick, sdlJoysticks()->joysticks())
    {
        bool sensor_enabled = enabled;
        /* Set all virtual gamepad sensor disabled */
        if (joystick->blacklisted) {
            sensor_enabled = false;
        }

        /* Check sensor disabled flag */
        if (enabled && joystick->sensor_disabled) {
            sensor_enabled = false;
        }

        SDL_bool sdl_enabled = SDL_FALSE;
        if (sensor_enabled) {
            sdl_enabled = SDL_TRUE;
        }
        else {
            sdl_enabled = SDL_FALSE;
        }
        SDL_GameController *gc = SDL_GameControllerFromInstanceID(joystick->instanceID);

        if (gc != nullptr) {
            if (joystick->has_gyro) {
                SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_GYRO, sdl_enabled);
            }
            if (joystick->has_accel) {
                SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_ACCEL, sdl_enabled);
            }

#ifdef DEBUG_LOGOUT_ON
            QString debugmessage = QString("[QJoysticks::setGameControllersSensorEnabled] P[%1], ControllerName[%2], Enabled[%3]")
                                       .arg(joystick->playerindex)
                                       .arg(joystick->name, sensor_enabled ? "True" : "False");
            qDebug().nospace().noquote() << debugmessage;
#endif
        }
    }
}

void QJoysticks::switchSensorDisabled(int instance_id)
{
    int index = instanceIDExists(instance_id);
    Q_ASSERT(index >= 0);

    if (index < 0) {
        return;
    }

    /* Switch sensor disabled value */
    m_devices.at(index)->sensor_disabled = !m_devices.at(index)->sensor_disabled;
}

void QJoysticks::onJoystickAdded(QJoystickDevice *joystick)
{
    emit joystickAdded(joystick);
}

void QJoysticks::onJoystickRemoved(const QJoystickDevice joystick)
{
    emit joystickRemoved(joystick);
}

/**
 * 'Rescans' for new/removed joysticks and registers them again.
 */
void QJoysticks::updateInterfaces()
{
   // Backup the current devices
   QList<QJoystickDevice*> backupDevices = m_devices;
   m_devices.clear();

   /* Put blacklisted joysticks at the bottom of the list */
   if (m_sortJoyticks)
   {
      /* Register non-blacklisted SDL joysticks */
      foreach (QJoystickDevice *joystick, sdlJoysticks()->joysticks())
      {
         // joystick->blacklisted = m_settings->value(joystick->name, false).toBool();
         // Check if the joystick is in the backup list and was blacklisted
         foreach (QJoystickDevice *backupJoystick, backupDevices)
         {
             if (backupJoystick->blacklisted &&
                 backupJoystick->vendorid == joystick->vendorid &&
                 backupJoystick->productid == joystick->productid &&
                 backupJoystick->serial == joystick->serial)
             {
                 joystick->blacklisted = true;
                 break;
             }
         }

         if (!joystick->blacklisted)
            addInputDevice(joystick);
      }

#ifdef VIRTUALJOYSTICK
      /* Register the virtual joystick (if its not blacklisted) */
      if (virtualJoystick()->joystickEnabled())
      {
         QJoystickDevice *joystick = virtualJoystick()->joystick();
         // joystick->blacklisted = m_settings->value(joystick->name, false).toBool();
         // Check if the joystick is in the backup list and was blacklisted
         foreach (QJoystickDevice *backupJoystick, backupDevices)
         {
             if (backupJoystick->blacklisted &&
                 backupJoystick->vendorid == joystick->vendorid &&
                 backupJoystick->productid == joystick->productid &&
                 backupJoystick->serial == joystick->serial)
             {
                 joystick->blacklisted = true;
                 break;
             }
         }

         if (!joystick->blacklisted)
         {
            addInputDevice(joystick);
            virtualJoystick()->setJoystickID(inputDevices().count() - 1);
         }
      }
#endif

      /* Register blacklisted SDL joysticks */
      foreach (QJoystickDevice *joystick, sdlJoysticks()->joysticks())
      {
         // joystick->blacklisted = m_settings->value(joystick->name, false).toBool();
         // Check if the joystick is in the backup list and was blacklisted
         foreach (QJoystickDevice *backupJoystick, backupDevices)
         {
             if (backupJoystick->blacklisted &&
                 backupJoystick->vendorid == joystick->vendorid &&
                 backupJoystick->productid == joystick->productid &&
                 backupJoystick->serial == joystick->serial)
             {
                 joystick->blacklisted = true;
                 break;
             }
         }
         if (joystick->blacklisted)
            addInputDevice(joystick);
      }

#ifdef VIRTUALJOYSTICK
      /* Register the virtual joystick (if its blacklisted) */
      if (virtualJoystick()->joystickEnabled())
      {
         QJoystickDevice *joystick = virtualJoystick()->joystick();
         // joystick->blacklisted = m_settings->value(joystick->name, false).toBool();
         // Check if the joystick is in the backup list and was blacklisted
         foreach (QJoystickDevice *backupJoystick, backupDevices)
         {
             if (backupJoystick->blacklisted &&
                 backupJoystick->vendorid == joystick->vendorid &&
                 backupJoystick->productid == joystick->productid &&
                 backupJoystick->serial == joystick->serial)
             {
                 joystick->blacklisted = true;
                 break;
             }
         }

         if (joystick->blacklisted)
         {
            addInputDevice(joystick);
            virtualJoystick()->setJoystickID(inputDevices().count() - 1);
         }
      }
#endif
   }

   /* Sort normally */
   else
   {
      /* Register SDL joysticks */
      foreach (QJoystickDevice *joystick, sdlJoysticks()->joysticks())
      {
         addInputDevice(joystick);
         // joystick->blacklisted = m_settings->value(joystick->name, false).toBool();
         // Check if the joystick is in the backup list and was blacklisted
         foreach (QJoystickDevice *backupJoystick, backupDevices)
         {
             if (backupJoystick->blacklisted &&
                 backupJoystick->vendorid == joystick->vendorid &&
                 backupJoystick->productid == joystick->productid &&
                 backupJoystick->serial == joystick->serial)
             {
                 joystick->blacklisted = true;
                 break;
             }
         }
      }

#ifdef VIRTUALJOYSTICK
      /* Register virtual joystick */
      if (virtualJoystick()->joystickEnabled())
      {
         QJoystickDevice *joystick = virtualJoystick()->joystick();
         // joystick->blacklisted = m_settings->value(joystick->name, false).toBool();
         // Check if the joystick is in the backup list and was blacklisted
         foreach (QJoystickDevice *backupJoystick, backupDevices)
         {
             if (backupJoystick->blacklisted &&
                 backupJoystick->vendorid == joystick->vendorid &&
                 backupJoystick->productid == joystick->productid &&
                 backupJoystick->serial == joystick->serial)
             {
                 joystick->blacklisted = true;
                 break;
             }
         }

         addInputDevice(joystick);
         virtualJoystick()->setJoystickID(inputDevices().count() - 1);
      }
#endif
   }

   emit countChanged();
}

#ifdef VIRTUALJOYSTICK
/**
 * Changes the axis value range of the virtual joystick.
 *
 * Take into account that maximum axis values supported by the \c QJoysticks
 * system is from \c -1 to \c 1.
 */
void QJoysticks::setVirtualJoystickRange(qreal range)
{
   virtualJoystick()->setAxisRange(range);
}

/**
 * Enables or disables the virtual joystick
 */
void QJoysticks::setVirtualJoystickEnabled(bool enabled)
{
   virtualJoystick()->setJoystickEnabled(enabled);
}

void QJoysticks::setVirtualJoystickAxisSensibility(qreal sensibility)
{
   virtualJoystick()->setAxisSensibility(sensibility);
}
#endif

/**
 * Removes all the registered joysticks and emits appropriate signals.
 */
void QJoysticks::resetJoysticks()
{
   m_devices.clear();
   emit countChanged();
}

/**
 * Registers the given \a device to the \c QJoysticks system
 */
void QJoysticks::addInputDevice(QJoystickDevice *device)
{
   Q_ASSERT(device);
   m_devices.append(device);
}

/**
 * Configures the QML-friendly signal based on the information given by the
 * \a event data and updates the joystick values
 */
void QJoysticks::onPOVEvent(const QJoystickPOVEvent &e)
{
   if (e.joystick == nullptr)
      return;

   if (!isBlacklisted(e.joystick->id))
   {
      if (e.pov < getInputDevice(e.joystick->id)->povs.count())
      {
         getInputDevice(e.joystick->id)->povs[e.pov] = e.angle;
         emit povChanged(e.joystick->id, e.pov, e.angle);
      }
   }
}

/**
 * Configures the QML-friendly signal based on the information given by the
 * \a event data and updates the joystick values
 */
void QJoysticks::onAxisEvent(const QJoystickAxisEvent &e)
{
   if (e.joystick == nullptr)
      return;

   if (!isBlacklisted(e.joystick->id))
   {
      if (e.axis < getInputDevice(e.joystick->id)->axes.count())
      {
         getInputDevice(e.joystick->id)->axes[e.axis] = e.value;
         emit axisChanged(e.joystick->id, e.axis, e.value);
      }
   }
}

/**
 * Configures the QML-friendly signal based on the information given by the
 * \a event data and updates the joystick values
 */
void QJoysticks::onButtonEvent(const QJoystickButtonEvent &e)
{
   if (e.joystick == nullptr)
      return;

   if (!isBlacklisted(e.joystick->id))
   {
      if (e.button < getInputDevice(e.joystick->id)->buttons.count())
      {
         getInputDevice(e.joystick->id)->buttons[e.button] = e.pressed;
         emit buttonChanged(e.joystick->id, e.button, e.pressed);
      }
   }
}
