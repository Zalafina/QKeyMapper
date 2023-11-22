# Introduction

The QJoysticks library allows your Qt application to read input from both real joysticks/gamepads (using SDL) and a virtual joystick (using the keyboard).

SDL binaries and headers for Windows and OS X are already included in this library. Linux uses will need to install libsdl2-dev using the package manager of their distro of choice.

## Integrating QJoysticks with your projects
1. Copy this folder in your "3rd-party" folder.
2. Include the QJoysticks project include (*pri*) file using the include() function.
3. That's all! Check the example project as a reference for your project.

## QML compatibility
This library can be used from QML, just register it as a context property when initializing your QML engine (check the example project for more information).

## WTFs Section

### 1. What are the joystick IDs and how are they assigned?

Each joystick has an numerical ID, which is used by the library and you to access joystick information. The ID is assigned based on the order in which a joystick was attached. Joystick IDs are always congruent to the lists returned by the following functions:

```
QStringList joystickNames = QJoysticks::getInstance()->deviceNames();
QList<QJoystickDevice*> joysticks = QJoysticks::getInstance()->inputDevices();
```

**NOTE:** The virtual joystick will always be the last joystick to be registered, in other words, it will always have the greatest numerical ID.

### 2. What is the virtual joystick?

The virtual joystick uses your computer keyboard to generate axis, button and POV events. The keyboard mappings are:

| Keyboard Keys       | Joystick Mappings |
|---------------------|-------------------|
| W,A,S,D             | Left Stick        |
| I,J,K,L             | Right Stick       |
| Arrow Keys          | POV hat           |
| Q,E                 | Left Trigger      |
| U,O                 | Right Trigger     |
| 0,1,2,3,4,5,6,7,8,9 | Joystick Buttons  |

**NOTE:** For the moment, I have not implemented support for custom mappings.

The virtual joystick can be enabled or disabled with the following function:

```
// Enable the virtual joystick
QJoysticks::getInstance()->setVirtualJoystickEnabled (true);

// Disable the virtual joystick
QJoysticks::getInstance()->setVirtualJoystickEnabled (false);
```

### 3. How are joystick events reported?

The QJosyticks library reports joystick events with the following signals:

```
// Only for C++ applications
void POVEvent (const QJoystickPOVEvent& event);
void axisEvent (const QJoystickAxisEvent& event);
void buttonEvent (const QJoystickButtonEvent& event);

// For C++ applications & QML applications
void povChanged (const int js, const int pov, const int angle);
void axisChanged (const int js, const int axis, const double value);
void buttonChanged (const int js, const int button, const bool pressed);
```

### 3. How are axis values reported?

This library reports an axis value with a range from -1.0 to 1.0 (with 0.0 being neutral).
You can customize the axis range of the virtual joystick with the following function:

```
QJoysticks::getInstance()->setVirtualJoystickAxisRange (0.8);
```

You cannot customize the axis range of the SDL joysticks.

## License
This project is released under the MIT license.
