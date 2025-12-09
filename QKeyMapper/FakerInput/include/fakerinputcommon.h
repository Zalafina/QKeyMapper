#ifndef FAKERINPUTCOMMON_H
#define FAKERINPUTCOMMON_H

#include <minwindef.h>

// Report ID for the various virtual devices

#define REPORTID_KEYBOARD 0x01
#define REPORTID_ENHANCED_KEY   0x02
#define REPORTID_RELATIVE_MOUSE 0x03
#define REPORTID_ABSOLUTE_MOUSE 0x04
#define REPORTID_CONTROL  0x40
#define REPORTID_CHECK_API_VERSION   0x41

#define REPORTID_API_VERSION_FEATURE_ID 0x42

#define CONTROL_REPORT_SIZE 0x41
#define FAKERINPUT_API_VERSION 0x01


typedef struct _FAKERINPUT_CONTROL_REPORT_HEADER
{

    BYTE        ReportID;

    BYTE        ReportLength;

} FakerInputControlReportHeader;


//
// Keyboard specific report infomation
//

#define KBD_LCONTROL_BIT     1
#define KBD_LSHIFT_BIT       2
#define KBD_LALT_BIT         4
#define KBD_LGUI_BIT         8
#define KBD_RCONTROL_BIT     16
#define KBD_RSHIFT_BIT       32
#define KBD_RALT_BIT         64
#define KBD_RGUI_BIT         128

#define KBD_KEY_CODES        6

typedef struct _FAKERINPUT_KEYBOARD_REPORT
{

    BYTE      ReportID;

    // Left Control, Left Shift, Left Alt, Left GUI
    // Right Control, Right Shift, Right Alt, Right GUI
    BYTE      ShiftKeyFlags;

    BYTE      Reserved;

    // See https://www.usb.org/sites/default/files/hut1_22.pdf
    // for a list of key codes
    BYTE      KeyCodes[KBD_KEY_CODES];

} FakerInputKeyboardReport;

typedef struct _FAKERINPUT_MULTIMEDIA_REPORT
{
    BYTE        ReportID;

    BYTE        MultimediaKeys0;

    BYTE        MultimediaKeys1;

    BYTE        MultimediaKeys2;

} FakerInputMultimediaReport;

//
// Relative mouse specific report information
//

#define RELATIVE_MOUSE_MIN_COORDINATE   -32767
#define RELATIVE_MOUSE_MAX_COORDINATE   32767

typedef struct _FAKERINPUT_RELATIVE_MOUSE_REPORT
{

    BYTE        ReportID;

    BYTE        Button;

    SHORT      XValue;

    SHORT      YValue;

    BYTE        WheelPosition;

    BYTE        HWheelPosition;

} FakerInputRelativeMouseReport;


typedef struct _FAKERINPUT_ABS_MOUSE_REPORT
{

    BYTE        ReportID;

    BYTE        Button;

    USHORT      XValue;

    USHORT      YValue;

    BYTE        WheelPosition;

    //BYTE        HWheelPosition;

} FakerInputAbsMouseReport;


typedef struct _FAKERINPUT_API_VERSION_REPORT
{
    BYTE ReportID;

    UINT32 ApiVersion;

} FakerInputAPIVersionReport;


typedef struct _FAKERINPUT_API_VERSION_FEATURE
{
    BYTE ReportId;

    UINT32 ApiVersion;

} FakerInputAPIVersionFeature;


#endif // FAKERINPUTCOMMON_H