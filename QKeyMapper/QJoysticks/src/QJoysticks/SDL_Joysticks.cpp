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

#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QJoysticks/SDL_Joysticks.h>

/**
 * Holds a generic mapping to be applied to joysticks that have not been mapped
 * by the SDL project or by the database.
 *
 * This mapping is different on each supported operating system.
 */
static QString GENERIC_MAPPINGS;

/**
 * Load a different generic/backup mapping for each operating system.
 */
#ifdef SDL_SUPPORTED
#   if defined Q_OS_WIN
#      define GENERIC_MAPPINGS_PATH ":/QJoysticks/SDL/GenericMappings/Windows.txt"
#   elif defined Q_OS_MAC
#      define GENERIC_MAPPINGS_PATH ":/QJoysticks/SDL/GenericMappings/OSX.txt"
#   elif defined Q_OS_LINUX && !defined Q_OS_ANDROID
#      define GENERIC_MAPPINGS_PATH ":/QJoysticks/SDL/GenericMappings/Linux.txt"
#   endif
#endif

SDL_Joysticks::SDL_Joysticks(QObject *parent)
   : QObject(parent)
{

#ifdef SDL_SUPPORTED
   if (SDL_Init(SDL_INIT_GAMECONTROLLER))
   {
      qDebug() << "Cannot initialize SDL:" << SDL_GetError();
      qApp->quit();
   }

   QFile database(":/QJoysticks/SDL/gamecontrollerdb.txt");
   if (database.open(QFile::ReadOnly))
   {
      while (!database.atEnd())
      {
         QString line = QString::fromUtf8(database.readLine());
         SDL_GameControllerAddMapping(line.toStdString().c_str());
      }

      database.close();
   }

   QFile genericMappings(GENERIC_MAPPINGS_PATH);
   if (genericMappings.open(QFile::ReadOnly))
   {
      GENERIC_MAPPINGS = QString::fromUtf8(genericMappings.readAll());
      genericMappings.close();
   }

   QTimer::singleShot(100, Qt::PreciseTimer, this, SLOT(update()));
#endif
}

SDL_Joysticks::~SDL_Joysticks()
{
   for (QMap<int, QJoystickDevice *>::iterator i = m_joysticks.begin(); i != m_joysticks.end(); ++i)
   {
      delete i.value();
   }

#ifdef SDL_SUPPORTED
   SDL_Quit();
#endif
}

/**
 * Returns a list with all the registered joystick devices
 */
QMap<int, QJoystickDevice *> SDL_Joysticks::joysticks()
{
#ifdef SDL_SUPPORTED
   int index = 0;
   QMap<int, QJoystickDevice *> joysticks;
   for (QMap<int, QJoystickDevice *>::iterator it = m_joysticks.begin(); it != m_joysticks.end(); ++it)
   {
      it.value()->id = index;
      joysticks[index++] = it.value();
   }

   return joysticks;
#endif
   return QMap<int, QJoystickDevice *>();
}

/**
 * Based on the data contained in the \a request, this function will instruct
 * the appropriate joystick to rumble for
 */
void SDL_Joysticks::rumble(const QJoystickRumble &request)
{
#ifdef SDL_SUPPORTED
   SDL_Haptic *haptic = SDL_HapticOpen(request.joystick->id);

   if (haptic)
   {
      SDL_HapticRumbleInit(haptic);
      SDL_HapticRumblePlay(haptic, request.strength, request.length);
   }
#else
   Q_UNUSED(request);
#endif
}

/**
 * Polls for new SDL events and reacts to each event accordingly.
 */
void SDL_Joysticks::update()
{
#ifdef SDL_SUPPORTED
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_JOYDEVICEADDED:
            configureJoystick(&event);
            break;
        case SDL_JOYDEVICEREMOVED: {
            SDL_JoystickID instanceId = event.jdevice.which;
            if (m_joysticks.contains(instanceId)
                && m_joysticks[instanceId] != nullptr) {
                const QJoystickDevice joystick_removed = *m_joysticks[instanceId];
                emit joystickRemoved(joystick_removed);
            }

            SDL_Joystick *js = SDL_JoystickFromInstanceID(instanceId);
            if (js)
            {
                SDL_JoystickClose(js);
            }

            SDL_GameController *gc = SDL_GameControllerFromInstanceID(instanceId);
            if (gc)
            {
                SDL_GameControllerClose(gc);
            }

            if (m_joysticks.contains(instanceId))
            {
                if (m_joysticks[instanceId] != nullptr) {
                    delete m_joysticks[instanceId];
                }
                m_joysticks.remove(instanceId);
            }

            emit countChanged();
        }
        break;
        case SDL_JOYAXISMOTION:
        {
            // SDL_JOY* events may also come from GameControllers, so we need to filter them out
            SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.jaxis.which);
            // Skip processing if the instance is already handled by the GameController API to avoid duplicate handling
            if (gc == nullptr) {
                emit axisEvent(getAxisEvent(&event));
            }
        }
        break;
        case SDL_CONTROLLERAXISMOTION:
            emit axisEvent(getControllerAxisEvent(&event));
            break;
        case SDL_CONTROLLERSENSORUPDATE:
        {
            SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.csensor.which);
            if (gc != nullptr)
            {
                if (event.csensor.sensor == SDL_SENSOR_GYRO || event.csensor.sensor == SDL_SENSOR_ACCEL) {
                    emit sensorEvent(getSensorEvent(&event));
                }
            }
        }
        break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            emit buttonEvent(getControllerButtonEvent(&event));
            break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        {
            // SDL_JOY* events may also come from GameControllers, so we need to filter them out
            SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.jbutton.which);
            // Skip processing if the instance is already handled by the GameController API to avoid duplicate handling
            if (gc == nullptr) {
                emit buttonEvent(getButtonEvent(&event));
            }
        }
        break;
        case SDL_JOYHATMOTION:
        {
            // SDL_JOY* events may also come from GameControllers, so we need to filter them out
            SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.jhat.which);
            // Skip processing if the instance is already handled by the GameController API to avoid duplicate handling
            if (gc == nullptr) {
                emit POVEvent(getPOVEvent(&event));
            }
        }
        case SDL_JOYBATTERYUPDATED:
            emit batteryEvent(getBatteryEvent(&event));
            break;
        }
    }

    QTimer::singleShot(10, Qt::PreciseTimer, this, SLOT(update()));
#endif
}

/**
 * Checks if the joystick referenced by the \a event can be initialized.
 * If not, the function will apply a generic mapping to the joystick and
 * attempt to initialize the joystick again.
 */
void SDL_Joysticks::configureJoystick(const SDL_Event *event)
{
#ifdef SDL_SUPPORTED
    int device_index = event->jdevice.which;
    QJoystickDevice *joystick = getJoystick(device_index);

    if (joystick != nullptr) {
        if (!SDL_IsGameController(device_index))
        {
            SDL_Joystick *js = SDL_JoystickFromInstanceID(joystick->instanceID);
            if (js)
            {
                char guid[1024];
                SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(js), guid, sizeof(guid));

                QString mapping = QString("%1,%2,%3").arg(guid, SDL_JoystickName(js), GENERIC_MAPPINGS);

                SDL_GameControllerAddMapping(mapping.toStdString().c_str());
            }
        }
        else {
            QString gamecontroller_name = SDL_GameControllerNameForIndex(device_index);
            if (!gamecontroller_name.isEmpty()) {
                joystick->name = gamecontroller_name;
            }

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[SDL_Joysticks::configureJoystick] GameControllerName -> " << "Name = " << gamecontroller_name;
#endif

            SDL_GameController *gc = SDL_GameControllerOpen(device_index);
            if (gc != nullptr) {
                bool has_gyro = SDL_GameControllerHasSensor(gc, SDL_SENSOR_GYRO);
                bool has_accel = SDL_GameControllerHasSensor(gc, SDL_SENSOR_ACCEL);

                if (has_gyro)
                {
                    joystick->has_gyro = true;
                }
                if (has_accel)
                {
                    joystick->has_accel = true;
                }
            }
        }
        emit joystickAdded(joystick);
    }

    emit countChanged();
#else
    Q_UNUSED(event);
#endif
}

/**
 * Returns the josytick device registered with the given \a id.
 * If no joystick with the given \a id is found, then the function will warn
 * the user through the console.
 */
QJoystickDevice *SDL_Joysticks::getJoystick(int id)
{
#ifdef SDL_SUPPORTED
   QJoystickDevice *joystick = new QJoystickDevice;
   SDL_Joystick *sdl_joystick = SDL_JoystickOpen(id);

   if (sdl_joystick)
   {
      joystick->id = id;
      joystick->instanceID = SDL_JoystickInstanceID(sdl_joystick);
      joystick->playerindex = SDL_JoystickGetPlayerIndex(sdl_joystick);
      joystick->has_gyro = false;
      joystick->has_accel = false;
      joystick->sensor_disabled = false;
      joystick->blacklisted = false;
      joystick->name = SDL_JoystickName(sdl_joystick);
      joystick->serial = SDL_JoystickGetSerial(sdl_joystick);
      joystick->vendorid = SDL_JoystickGetVendor(sdl_joystick);
      joystick->productid = SDL_JoystickGetProduct(sdl_joystick);
      joystick->numbuttons = SDL_JoystickNumButtons(sdl_joystick);
      joystick->powerlevel = SDL_JoystickCurrentPowerLevel(sdl_joystick);
// #ifdef DEBUG_LOGOUT_ON
//       QString vendorIdStr = QString("0x%1").arg(QString::number(joystick->vendorid, 16).toUpper(), 4, '0');
//       QString productIdStr = QString("0x%1").arg(QString::number(joystick->productid, 16).toUpper(), 4, '0');
//       qDebug().nospace() << "[SDL_Joysticks] getJoystick -> " << "Name = " << joystick->name << ", VendorID = " << vendorIdStr << ", ProductID = " << productIdStr << ", ButtonNumbers = " << joystick->numbuttons << ", Serial = " << joystick->serial;
// #endif

      /* Get joystick properties */
      int povs = SDL_JoystickNumHats(sdl_joystick);
      int axes = SDL_JoystickNumAxes(sdl_joystick);
      int buttons = SDL_JoystickNumButtons(sdl_joystick);

      /* Initialize POVs */
      for (int i = 0; i < povs; ++i)
         joystick->povs.append(0);

      /* Initialize axes */
      for (int i = 0; i < axes; ++i)
         joystick->axes.append(0);

      /* Initialize buttons */
      for (int i = 0; i < buttons; ++i)
         joystick->buttons.append(false);

      if (m_joysticks.contains(joystick->instanceID)
          && m_joysticks[joystick->instanceID] != nullptr) {
        delete m_joysticks[joystick->instanceID];
      }
      m_joysticks[joystick->instanceID] = joystick;
   }
   else
   {
      qWarning() << Q_FUNC_INFO << "Cannot find joystick with id:" << id;
      delete joystick; // Delete the joystick object if SDL_JoystickOpen failed
      joystick = nullptr;
   }

   return joystick;
#else
   Q_UNUSED(id);
   return NULL;
#endif
}

/**
 * Reads the contents of the given \a event and constructs a new
 * \c QJoystickPOVEvent to be used with the \c QJoysticks system.
 */
QJoystickPOVEvent SDL_Joysticks::getPOVEvent(const SDL_Event *sdl_event)
{
   QJoystickPOVEvent event;
   event.pov = 0;
   event.angle = 0;
   event.joystick = Q_NULLPTR;

   if (!m_joysticks.contains(sdl_event->jhat.which))
   {
      return event;
   }

#ifdef SDL_SUPPORTED
   event.pov = sdl_event->jhat.hat;
   event.joystick = m_joysticks[sdl_event->jhat.which];

   switch (sdl_event->jhat.value)
   {
      case SDL_HAT_RIGHTUP:
         event.angle = 45;
         break;
      case SDL_HAT_RIGHTDOWN:
         event.angle = 135;
         break;
      case SDL_HAT_LEFTDOWN:
         event.angle = 225;
         break;
      case SDL_HAT_LEFTUP:
         event.angle = 315;
         break;
      case SDL_HAT_UP:
         event.angle = 0;
         break;
      case SDL_HAT_RIGHT:
         event.angle = 90;
         break;
      case SDL_HAT_DOWN:
         event.angle = 180;
         break;
      case SDL_HAT_LEFT:
         event.angle = 270;
         break;
      default:
         event.angle = -1;
         break;
   }
#else
   Q_UNUSED(sdl_event);
#endif

   return event;
}

/**
 * Reads the contents of the given \a event and constructs a new
 * \c QJoystickAxisEvent to be used with the \c QJoysticks system.
 */
QJoystickAxisEvent SDL_Joysticks::getAxisEvent(const SDL_Event *sdl_event)
{
   QJoystickAxisEvent event;
   event.axis = 0;
   event.value = 0;
   event.joystick = Q_NULLPTR;

   if (!m_joysticks.contains(sdl_event->jaxis.which))
   {
       return event;
   }

   event.axis = sdl_event->jaxis.axis;
   event.value = static_cast<qreal>(sdl_event->jaxis.value) / 32767;
   event.event_type = JoystickEvent;
   event.joystick = m_joysticks[sdl_event->jaxis.which];

   return event;
}

QJoystickAxisEvent SDL_Joysticks::getControllerAxisEvent(const SDL_Event *sdl_event)
{
    QJoystickAxisEvent event;
    event.axis = 0;
    event.value = 0;
    event.joystick = Q_NULLPTR;

    if (!m_joysticks.contains(sdl_event->caxis.which))
    {
        return event;
    }

    event.axis = sdl_event->caxis.axis;
    event.value = static_cast<qreal>(sdl_event->caxis.value) / 32767;
    event.event_type = GameControllerEvent;
    event.joystick = m_joysticks[sdl_event->caxis.which];

    return event;
}

/**
 * Reads the contents of the given \a event and constructs a new
 * \c QJoystickButtonEvent to be used with the \c QJoysticks system.
 */
QJoystickButtonEvent SDL_Joysticks::getButtonEvent(const SDL_Event *sdl_event)
{
   QJoystickButtonEvent event;
   event.button = 0;
   event.pressed = SDL_RELEASED;
   event.joystick = Q_NULLPTR;

   if (!m_joysticks.contains(sdl_event->jbutton.which))
   {
      return event;
   }

#ifdef SDL_SUPPORTED
   event.event_type = JoystickEvent;
   event.button = sdl_event->jbutton.button;
   event.pressed = sdl_event->jbutton.state == SDL_PRESSED;
   event.joystick = m_joysticks[sdl_event->jbutton.which];
   // event.joystick->buttons[event.button] = event.pressed;
#else
   Q_UNUSED(sdl_event);
#endif

   return event;
}

QJoystickButtonEvent SDL_Joysticks::getControllerButtonEvent(const SDL_Event *sdl_event)
{
    QJoystickButtonEvent event;
    event.button = SDL_CONTROLLER_BUTTON_INVALID;
    event.pressed = SDL_RELEASED;
    event.joystick = Q_NULLPTR;

    if (!m_joysticks.contains(sdl_event->cbutton.which))
    {
        return event;
    }

    event.event_type = GameControllerEvent;
    event.button = sdl_event->cbutton.button;
    event.pressed = sdl_event->cbutton.state == SDL_PRESSED;
    event.joystick = m_joysticks[sdl_event->cbutton.which];
    // event.joystick->buttons[event.button] = event.pressed;

    return event;
}

QJoystickSensorEvent SDL_Joysticks::getSensorEvent(const SDL_Event *sdl_event)
{
    SDL_GameController *gc = SDL_GameControllerFromInstanceID(sdl_event->csensor.which);
    QJoystickSensorEvent event;
    event.gyroX = 0;
    event.gyroY = 0;
    event.gyroZ = 0;
    event.accelX = 0;
    event.accelY = 0;
    event.accelZ = 0;
    event.timestamp = 0;
    event.joystick = Q_NULLPTR;
    event.sensorType = sdl_event->csensor.sensor;
    bool has_gyro = SDL_GameControllerHasSensor(gc, SDL_SENSOR_GYRO);
    bool has_accel = SDL_GameControllerHasSensor(gc, SDL_SENSOR_ACCEL);

    if (has_gyro)
    {
        float gyro[3] = {0};
        SDL_GameControllerGetSensorDataWithTimestamp(gc, SDL_SENSOR_GYRO, &event.timestamp, &gyro[0], 3);
        static constexpr float toDegPerSec = float(180. / M_PI);
        event.gyroX = gyro[0] * toDegPerSec;
        event.gyroY = gyro[1] * toDegPerSec;
        event.gyroZ = gyro[2] * toDegPerSec;
    }
    if (has_accel)
    {
        float accel[3] = {0};
        SDL_GameControllerGetSensorDataWithTimestamp(gc, SDL_SENSOR_ACCEL, &event.timestamp, &accel[0], 3);
        static constexpr float toGs = 1.f / 9.8f;
        event.accelX = accel[0] * toGs;
        event.accelY = accel[1] * toGs;
        event.accelZ = accel[2] * toGs;
    }

    event.joystick = m_joysticks[sdl_event->csensor.which];

    return event;
}

QJoystickBatteryEvent SDL_Joysticks::getBatteryEvent(const SDL_Event *sdl_event)
{
    QJoystickBatteryEvent event;
    event.powerlevel = sdl_event->jbattery.level;
    event.joystick = Q_NULLPTR;

    if (!m_joysticks.contains(sdl_event->jbattery.which))
    {
        return event;
    }

    event.joystick = m_joysticks[sdl_event->jbattery.which];
    m_joysticks[sdl_event->jbattery.which]->powerlevel = sdl_event->jbattery.level;

    return event;
}
