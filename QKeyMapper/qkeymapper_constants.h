#ifndef QKEYMAPPER_CONSTANTS_H
#define QKEYMAPPER_CONSTANTS_H

#include <windows.h>

/* constant values for QKeyMapper */
extern const unsigned int CYCLE_CHECK_TIMEOUT;
extern const unsigned int CYCLE_CHECK_LOOPCOUNT_MAX;
extern const unsigned int CYCLE_CHECK_LOOPCOUNT_RESET;
extern const unsigned int CYCLE_REFRESH_PROCESSINFOTABLE_TIMEOUT;
extern const unsigned int GLOBAL_MAPPING_START_WAIT;
extern const int VIRTUALGAMEPADTYPECOMBOBOX_X;
extern const int PROCESSINFO_TABLE_COLUMN_COUNT;
extern const int KEYMAPPINGDATA_TABLE_COLUMN_COUNT;
extern const int INITIAL_WINDOW_POSITION;
extern const int PROCESS_NAME_COLUMN;
extern const int PROCESS_PID_COLUMN;
extern const int PROCESS_TITLE_COLUMN;
extern const int PROCESS_NAME_COLUMN_WIDTH_MAX;
extern const int ORIGINAL_KEY_COLUMN;
extern const int MAPPING_KEY_COLUMN;
extern const int BURST_MODE_COLUMN;
extern const int LOCK_COLUMN;
extern const int DEFAULT_ICON_WIDTH;
extern const int DEFAULT_ICON_HEIGHT;
extern const int MOUSEWHEEL_SCROLL_NONE;
extern const int MOUSEWHEEL_SCROLL_UP;
extern const int MOUSEWHEEL_SCROLL_DOWN;
extern const Qt::Key KEY_REFRESH;
extern const Qt::Key KEY_PASSTHROUGH;
extern const Qt::Key KEY_KEYUP_ACTION;
extern const Qt::Key KEY_REMOVE_LAST;
extern const int LANGUAGE_CHINESE;
extern const int LANGUAGE_ENGLISH;
extern const int KEYPRESS_TYPE_LONGPRESS;
extern const int KEYPRESS_TYPE_DOUBLEPRESS;
extern const int CUSTOMSETTING_INDEX_MAX;
extern const int TITLESETTING_INDEX_INVALID;
extern const int TITLESETTING_INDEX_ANYTITLE;
extern const int TITLESETTING_INDEX_MAX;
extern const int BURST_TIME_MIN;
extern const int BURST_TIME_MAX;
extern const int BURST_PRESS_TIME_DEFAULT;
extern const int BURST_RELEASE_TIME_DEFAULT;
extern const int MAPPING_WAITTIME_MIN;
extern const int MAPPING_WAITTIME_MAX;
extern const int PRESSTIME_MIN;
extern const int PRESSTIME_MAX;
extern const int DATA_PORT_MIN;
extern const int DATA_PORT_MAX;
extern const int DATA_PORT_DEFAULT;
extern const int GRIP_THRESHOLD_DECIMALS;
extern const double GRIP_THRESHOLD_BRAKE_MIN;
extern const double GRIP_THRESHOLD_BRAKE_MAX;
extern const double GRIP_THRESHOLD_BRAKE_DEFAULT;
extern const double GRIP_THRESHOLD_ACCEL_MIN;
extern const double GRIP_THRESHOLD_ACCEL_MAX;
extern const double GRIP_THRESHOLD_ACCEL_DEFAULT;
extern const double GRIP_THRESHOLD_SINGLE_STEP;
extern const int MOUSE_SPEED_MIN;
extern const int MOUSE_SPEED_MAX;
extern const int MOUSE_SPEED_DEFAULT;
extern const int UI_SCALE_NORMAL;
extern const int UI_SCALE_1K_PERCENT_100;
extern const int UI_SCALE_1K_PERCENT_125;
extern const int UI_SCALE_1K_PERCENT_150;
extern const int UI_SCALE_2K_PERCENT_100;
extern const int UI_SCALE_2K_PERCENT_125;
extern const int UI_SCALE_2K_PERCENT_150;
extern const int UI_SCALE_4K_PERCENT_100;
extern const int UI_SCALE_4K_PERCENT_125;
extern const int UI_SCALE_4K_PERCENT_150;
extern const int MOUSE_POINT_RADIUS;
extern const int SHOW_POINTSIN_SCREEN_OFF;
extern const int SHOW_POINTSIN_SCREEN_ON;
extern const int SHOW_POINTSIN_WINDOW_OFF;
extern const int SHOW_POINTSIN_WINDOW_ON;
#ifdef VIGEM_CLIENT_SUPPORT
extern const int RECONNECT_VIGEMCLIENT_WAIT_TIME;
extern const int VIRTUAL_JOYSTICK_SENSITIVITY_MIN;
extern const int VIRTUAL_JOYSTICK_SENSITIVITY_MAX;
extern const int VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT;
extern const int VIRTUAL_GAMEPAD_NUMBER_MIN;
extern const int VIRTUAL_GAMEPAD_NUMBER_MAX;
#endif
extern const int INSTALL_INTERCEPTION_LOOP_WAIT_TIME;
extern const int INSTALL_INTERCEPTION_LOOP_WAIT_TIME_MAX;

/* constant values for QKeyMapper_Worker */
extern const int KEY_INIT;
extern const int KEY_UP;
extern const int KEY_DOWN;

extern const bool EXTENED_FLAG_TRUE;
extern const bool EXTENED_FLAG_FALSE;

extern const int INITIAL_KEYBOARD_INDEX;
extern const int INITIAL_MOUSE_INDEX;

extern const int KEY_INTERCEPT_NONE;
extern const int KEY_INTERCEPT_BLOCK;
extern const int KEY_INTERCEPT_PASSTHROUGH;

extern const int KEY_PROC_NONE;
extern const int KEY_PROC_BURST;
extern const int KEY_PROC_LOCK;

extern const int SENDMODE_NORMAL;
extern const int SENDMODE_FORCE_STOP;
extern const int SENDMODE_KEYSEQ_HOLDDOWN;

extern const int MOUSE_WHEEL_UP;
extern const int MOUSE_WHEEL_DOWN;

extern const int MOUSE_WHEEL_KEYUP_WAITTIME;

extern const int SETMOUSEPOSITION_WAITTIME_MAX;

extern const WORD XBUTTON_NONE;

extern const int JOYSTICK_POV_ANGLE_RELEASE;
extern const int JOYSTICK_POV_ANGLE_UP;
extern const int JOYSTICK_POV_ANGLE_DOWN;
extern const int JOYSTICK_POV_ANGLE_LEFT;
extern const int JOYSTICK_POV_ANGLE_RIGHT;
extern const int JOYSTICK_POV_ANGLE_L_UP;
extern const int JOYSTICK_POV_ANGLE_L_DOWN;
extern const int JOYSTICK_POV_ANGLE_R_UP;
extern const int JOYSTICK_POV_ANGLE_R_DOWN;

extern const int JOYSTICK_AXIS_LS_HORIZONTAL;
extern const int JOYSTICK_AXIS_LS_VERTICAL;
extern const int JOYSTICK_AXIS_RS_HORIZONTAL;
extern const int JOYSTICK_AXIS_RS_VERTICAL;
extern const int JOYSTICK_AXIS_LT_BUTTON;
extern const int JOYSTICK_AXIS_RT_BUTTON;

extern const qreal JOYSTICK_AXIS_NEAR_ZERO_THRESHOLD;

extern const qreal JOYSTICK_AXIS_LT_RT_KEYUP_THRESHOLD;
extern const qreal JOYSTICK_AXIS_LT_RT_KEYDOWN_THRESHOLD;

extern const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD;
extern const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD;
extern const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MIN_THRESHOLD;
extern const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MAX_THRESHOLD;

extern const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD;
extern const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD;
extern const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MIN_THRESHOLD;
extern const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MAX_THRESHOLD;

extern const qreal JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD;
extern const qreal JOYSTICK2MOUSE_AXIS_MINUS_MID_THRESHOLD;
extern const qreal JOYSTICK2MOUSE_AXIS_MINUS_HIGH_THRESHOLD;
extern const qreal JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD;
extern const qreal JOYSTICK2MOUSE_AXIS_PLUS_MID_THRESHOLD;
extern const qreal JOYSTICK2MOUSE_AXIS_PLUS_HIGH_THRESHOLD;

extern const int MOUSE_CURSOR_BOTTOMRIGHT_X;
extern const int MOUSE_CURSOR_BOTTOMRIGHT_Y;

extern const int KEY2MOUSE_CYCLECHECK_TIMEOUT;

extern const int JOY2VJOY_LEFTSTICK_X;
extern const int JOY2VJOY_LEFTSTICK_Y;
extern const int JOY2VJOY_RIGHTSTICK_X;
extern const int JOY2VJOY_RIGHTSTICK_Y;

#ifdef VIGEM_CLIENT_SUPPORT
extern const USHORT VIRTUALGAMPAD_VENDORID_X360;
extern const USHORT VIRTUALGAMPAD_PRODUCTID_X360;

extern const USHORT VIRTUALGAMPAD_VENDORID_DS4;
extern const USHORT VIRTUALGAMPAD_PRODUCTID_DS4;
extern const char *VIRTUALGAMPAD_SERIAL_PREFIX_DS4;

extern const BYTE XINPUT_TRIGGER_MIN;
extern const BYTE XINPUT_TRIGGER_MAX;

extern const SHORT XINPUT_THUMB_MIN;
extern const SHORT XINPUT_THUMB_RELEASE;
extern const SHORT XINPUT_THUMB_MAX;

extern const qreal THUMB_DISTANCE_MAX;

extern const BYTE AUTO_BRAKE_ADJUST_VALUE;
extern const BYTE AUTO_ACCEL_ADJUST_VALUE;
extern const BYTE AUTO_BRAKE_DEFAULT;
extern const BYTE AUTO_ACCEL_DEFAULT;

extern const double GRIP_THRESHOLD_MAX;

extern const qsizetype FORZA_MOTOR_7_SLED_DATA_LENGTH;
extern const qsizetype FORZA_MOTOR_7_DASH_DATA_LENGTH;
extern const qsizetype FORZA_MOTOR_8_DASH_DATA_LENGTH;
extern const qsizetype FORZA_HORIZON_DATA_LENGTH;

extern const qsizetype FIRAT_PART_DATA_LENGTH;
extern const qsizetype SECOND_PART_DATA_LENGTH;
extern const qsizetype FORZA_HORIZON_BUFFER_OFFSET;

extern const int AUTO_ADJUST_NONE;
extern const int AUTO_ADJUST_BRAKE;
extern const int AUTO_ADJUST_ACCEL;
extern const int AUTO_ADJUST_BOTH;
extern const int AUTO_ADJUST_LT;
extern const int AUTO_ADJUST_RT;

extern const int VJOY_UPDATE_NONE;
extern const int VJOY_UPDATE_BUTTONS;
extern const int VJOY_UPDATE_JOYSTICKS;
extern const int VJOY_UPDATE_AUTO_BUTTONS;

extern const int VIRTUAL_JOYSTICK_SENSITIVITY_MIN;
extern const int VIRTUAL_JOYSTICK_SENSITIVITY_MAX;
extern const int VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT;

extern const int VIRTUAL_GAMEPAD_NUMBER_MIN;
extern const int VIRTUAL_GAMEPAD_NUMBER_MAX;

extern const int MOUSE2VJOY_RESET_TIMEOUT;
extern const int VJOY_KEYUP_WAITTIME;
extern const quint8 VK_MOUSE2VJOY_HOLD;
extern const quint8 VK_MOUSE2VJOY_DIRECT;
#endif

extern const quint8 VK_KEY2MOUSE_UP;
extern const quint8 VK_KEY2MOUSE_DOWN;
extern const quint8 VK_KEY2MOUSE_LEFT;
extern const quint8 VK_KEY2MOUSE_RIGHT;
extern const quint8 VK_GAMEPAD_HOME;

extern const unsigned int INTERCEPTION_EXTRA_INFO;

extern const ULONG_PTR VIRTUAL_KEYBOARD_PRESS;
extern const ULONG_PTR VIRTUAL_MOUSE2JOY_KEYS;
extern const ULONG_PTR VIRTUAL_MOUSE_CLICK;
extern const ULONG_PTR VIRTUAL_MOUSE_POINTCLICK;
extern const ULONG_PTR VIRTUAL_MOUSE_MOVE;
extern const ULONG_PTR VIRTUAL_MOUSE_MOVE_BYKEYS;
extern const ULONG_PTR VIRTUAL_MOUSE_WHEEL;
extern const ULONG_PTR VIRTUAL_WIN_PLUS_D;

extern const UINT SCANCODE_CTRL;
extern const UINT SCANCODE_ALT;
extern const UINT SCANCODE_LSHIFT;
extern const UINT SCANCODE_RSHIFT;
extern const UINT SCANCODE_DIVIDE;
extern const UINT SCANCODE_NUMLOCK;
extern const UINT SCANCODE_SNAPSHOT;
extern const UINT SCANCODE_PRINTSCREEN;
extern const UINT SCANCODE_LWIN;
extern const UINT SCANCODE_RWIN;
extern const UINT SCANCODE_APPS;

/* constant chars for QKeyMapper */
extern const char* PROGRAM_NAME;
extern const char *PREFIX_SHORTCUT;
extern const char *JOIN_DEVICE;
extern const char *KEYBOARD_MODIFIERS;
extern const char *KEYSEQUENCE_STR;
extern const char *HOLDDOWN_STR;
extern const char *CLEAR_VIRTUALKEYS;
extern const char* PROCESS_UNKNOWN;
extern const char* DEFAULT_NAME;
extern const char* CONFIG_FILENAME;
extern const char *CONFIG_LATEST_FILENAME;
#ifdef SETTINGSFILE_CONVERT
extern const char* CONFIG_BACKUP_FILENAME;
#endif
extern const char* DISPLAYSWITCH_KEY_DEFAULT;
extern const char* MAPPINGSWITCH_KEY_DEFAULT;
extern const char* ORIGINAL_KEYSEQ_DEFAULT;
extern const char* LAST_WINDOWPOSITION;
extern const char* LANGUAGE_INDEX;
extern const char* SETTINGSELECT;
extern const char* AUTO_STARTUP;
extern const char *STARTUP_MINIMIZED;
extern const char* PLAY_SOUNDEFFECT;
extern const char* WINDOWSWITCH_KEYSEQ;
#ifdef VIGEM_CLIENT_SUPPORT
extern const char* VIRTUALGAMEPAD_ENABLE;
extern const char* VIRTUALGAMEPAD_TYPE;
extern const char* VIRTUAL_GAMEPADLIST;
#endif
extern const char* MULTI_INPUT_ENABLE;
extern const char* FILTER_KEYS;
extern const char* DISABLED_KEYBOARDLIST;
extern const char* DISABLED_MOUSELIST;
extern const char* GROUPNAME_EXECUTABLE_SUFFIX;
extern const char* GROUPNAME_CUSTOMSETTING;
extern const char* GROUPNAME_CUSTOMGLOBALSETTING;
extern const char* GROUPNAME_GLOBALSETTING;
extern const char* WINDOWTITLE_STRING;
extern const char* ANYWINDOWTITLE_STRING;
extern const char* KEYMAPDATA_ORIGINALKEYS;
extern const char* KEYMAPDATA_MAPPINGKEYS;
extern const char* KEYMAPDATA_BURST;
extern const char* KEYMAPDATA_LOCK;
extern const char *KEYMAPDATA_PASSTHROUGH;
extern const char *KEYMAPDATA_KEYUP_ACTION;
extern const char *KEYMAPDATA_KEYSEQHOLDDOWN;
extern const char* KEYMAPDATA_BURSTPRESS_TIME;
extern const char* KEYMAPDATA_BURSTRELEASE_TIME;
extern const char* KEY2MOUSE_X_SPEED;
extern const char* KEY2MOUSE_Y_SPEED;
#ifdef VIGEM_CLIENT_SUPPORT
extern const char* MOUSE2VJOY_X_SENSITIVITY;
extern const char* MOUSE2VJOY_Y_SENSITIVITY;
extern const char* MOUSE2VJOY_LOCKCURSOR;
#endif
extern const char* CLEARALL;
extern const char* PROCESSINFO_FILENAME;
extern const char* PROCESSINFO_WINDOWTITLE;
extern const char* PROCESSINFO_FILEPATH;
extern const char* PROCESSINFO_FILENAME_CHECKED;
extern const char* PROCESSINFO_WINDOWTITLE_CHECKED;
extern const char* DATAPORT_NUMBER;
extern const char* GRIP_THRESHOLD_BRAKE;
extern const char* GRIP_THRESHOLD_ACCEL;
extern const char* AUTOSTARTMAPPING_CHECKED;
extern const char *SENDTOSAMEWINDOWS_CHECKED;
extern const char* MAPPINGSWITCH_KEYSEQ;
extern const char* SAO_FONTFILENAME;
extern const char *USBIDS_QRC;
extern const char* SOUNDFILE_START_QRC;
extern const char* SOUNDFILE_START;
extern const char* SOUNDFILE_STOP_QRC;
extern const char* SOUNDFILE_STOP;
extern const char* FONTNAME_ENGLISH;
extern const char* FONTNAME_CHINESE;
extern const QColor STATUS_ON_COLOR;
extern const char* ORIKEY_COMBOBOX_NAME;
extern const char* MAPKEY_COMBOBOX_NAME;
extern const char* SETUPDIALOG_ORIKEY_COMBOBOX_NAME;
extern const char* SETUPDIALOG_MAPKEY_COMBOBOX_NAME;
extern const char* WINDOWSWITCHKEY_LINEEDIT_NAME;
extern const char* MAPPINGSWITCHKEY_LINEEDIT_NAME;
extern const char *KEY_NONE_STR;
extern const char* KEY_BLOCKED_STR;
extern const char *GAMEPAD_HOME_STR;
extern const char* MOUSE_BUTTON_PREFIX;
extern const char* MOUSE_POINT_POSTFIX;
extern const char* MOUSE_L_STR;
extern const char* MOUSE_R_STR;
extern const char* MOUSE_M_STR;
extern const char* MOUSE_X1_STR;
extern const char* MOUSE_X2_STR;
extern const char* MOUSE_L_POINT_STR;
extern const char* MOUSE_R_POINT_STR;
extern const char* MOUSE_M_POINT_STR;
extern const char* MOUSE_X1_POINT_STR;
extern const char* MOUSE_X2_POINT_STR;
extern const char *SHOW_POINTS_IN_WINDOW_KEY;
extern const char *SHOW_POINTS_IN_SCREEN_KEY;
extern const char *SHOW_CAR_ORDINAL_KEY;
extern const char* MOUSE_WHEEL_UP_STR;
extern const char* MOUSE_WHEEL_DOWN_STR;
extern const char *MOUSE_WHEEL_LEFT_STR;
extern const char *MOUSE_WHEEL_RIGHT_STR;
extern const char* JOY_KEY_PREFIX;
extern const char* VJOY_KEY_PREFIX;
extern const char* VJOY_MOUSE2LS_STR;
extern const char* VJOY_MOUSE2RS_STR;
extern const char* MOUSE2VJOY_PREFIX;
extern const char* MOUSE2VJOY_HOLD_KEY_STR;
extern const char* MOUSE2VJOY_DIRECT_KEY_STR;
extern const char* VJOY_LT_BRAKE_STR;
extern const char* VJOY_RT_BRAKE_STR;
extern const char* VJOY_LT_ACCEL_STR;
extern const char* VJOY_RT_ACCEL_STR;
extern const char* JOY_LS2VJOYLS_STR;
extern const char* JOY_RS2VJOYRS_STR;
extern const char* JOY_LS2VJOYRS_STR;
extern const char* JOY_RS2VJOYLS_STR;
extern const char* JOY_LT2VJOYLT_STR;
extern const char* JOY_RT2VJOYRT_STR;
extern const char* JOY_LS2MOUSE_STR;
extern const char* JOY_RS2MOUSE_STR;
extern const char* KEY2MOUSE_PREFIX;
extern const char* KEY2MOUSE_UP_STR;
extern const char* KEY2MOUSE_DOWN_STR;
extern const char* KEY2MOUSE_LEFT_STR;
extern const char* KEY2MOUSE_RIGHT_STR;
extern const char* FUNC_PREFIX;
extern const char* FUNC_REFRESH;
extern const char* FUNC_LOCKSCREEN;
extern const char* FUNC_SHUTDOWN;
extern const char* FUNC_REBOOT;
extern const char* FUNC_LOGOFF;
extern const char* FUNC_SLEEP;
extern const char* FUNC_HIBERNATE;
extern const char* VIRTUAL_GAMEPAD_X360;
extern const char* VIRTUAL_GAMEPAD_DS4;
extern const char* NO_INPUTDEVICE;

/* Chinese const Strings */
extern const char* REFRESHBUTTON_CHINESE;
extern const char* KEYMAPBUTTON_START_CHINESE;
extern const char* KEYMAPBUTTON_STOP_CHINESE;
extern const char* SAVEMAPLISTBUTTON_CHINESE;
extern const char* DELETEONEBUTTON_CHINESE;
extern const char* CLEARALLBUTTON_CHINESE;
extern const char* ADDMAPDATABUTTON_CHINESE;
extern const char* NAMECHECKBOX_CHINESE;
extern const char* TITLECHECKBOX_CHINESE;
extern const char* ORIKEYLABEL_CHINESE;
extern const char* ORIKEYSEQLABEL_CHINESE;
extern const char* MAPKEYLABEL_CHINESE;
extern const char *ORIKEYLISTLABEL_CHINESE;
extern const char *MAPKEYLISTLABEL_CHINESE;
extern const char* BURSTPRESSLABEL_CHINESE;
extern const char* BURSTRELEASE_CHINESE;
extern const char* WAITTIME_CHINESE;
extern const char* LONGPRESS_CHINESE;
extern const char* DOUBLEPRESS_CHINESE;
extern const char* POINT_CHINESE;
extern const char* MOUSEXSPEEDLABEL_CHINESE;
extern const char* MOUSEYSPEEDLABEL_CHINESE;
extern const char* REMOVESETTINGBUTTON_CHINESE;
extern const char* DATAPORTLABEL_CHINESE;
extern const char* BRAKETHRESHOLDLABEL_CHINESE;
extern const char* ACCELTHRESHOLDLABEL_CHINESE;
extern const char* AUTOSTARTMAPPINGCHECKBOX_CHINESE;
extern const char *SENDTOSAMETITLEWINDOWSCHECKBOX_CHINESE;
extern const char* AUTOSTARTUPCHECKBOX_CHINESE;
extern const char* STARTUPMINIMIZEDCHECKBOX_CHINESE;
extern const char* SOUNDEFFECTCHECKBOX_CHINESE;
extern const char* WINDOWSWITCHKEYLABEL_CHINESE;
extern const char* MAPPINGSWITCHKEYLABEL_CHINESE;
extern const char* PROCESSINFOTABLE_COL1_CHINESE;
extern const char* PROCESSINFOTABLE_COL2_CHINESE;
extern const char* PROCESSINFOTABLE_COL3_CHINESE;
extern const char* KEYMAPDATATABLE_COL1_CHINESE;
extern const char* KEYMAPDATATABLE_COL2_CHINESE;
extern const char* KEYMAPDATATABLE_COL3_CHINESE;
extern const char* KEYMAPDATATABLE_COL4_CHINESE;
#ifdef VIGEM_CLIENT_SUPPORT
extern const char* VIRTUALGAMEPADGROUPBOX_CHINESE;
extern const char* VJOYXSENSLABEL_CHINESE;
extern const char* VJOYYSENSLABEL_CHINESE;
extern const char* VIGEMBUSSTATUSLABEL_UNAVAILABLE_CHINESE;
extern const char* VIGEMBUSSTATUSLABEL_AVAILABLE_CHINESE;
extern const char* INSTALLVIGEMBUSBUTTON_CHINESE;
extern const char* UNINSTALLVIGEMBUSBUTTON_CHINESE;
extern const char* ENABLEVIRTUALJOYSTICKCHECKBOX_CHINESE;
extern const char* LOCKCURSORCHECKBOX_CHINESE;
#endif
extern const char* MULTIINPUTGROUPBOX_CHINESE;
extern const char* MULTIINPUTENABLECHECKBOX_CHINESE;
extern const char* MULTIINPUTDEVICELISTBUTTON_CHINESE;
extern const char* FILTERKEYSCHECKBOX_CHINESE;
extern const char* INSTALLINTERCEPTIONBUTTON_CHINESE;
extern const char* UNINSTALLINTERCEPTIONBUTTON_CHINESE;
extern const char* MULTIINPUTSTATUSLABEL_UNAVAILABLE_CHINESE;
extern const char* MULTIINPUTSTATUSLABEL_REBOOTREQUIRED_CHINESE;
extern const char* MULTIINPUTSTATUSLABEL_AVAILABLE_CHINESE;
extern const char* KEYBOARDSELECTLABEL_CHINESE;
extern const char* MOUSESELECTLABEL_CHINESE;
extern const char *TRAYMENU_SHOWACTION_CHINESE;
extern const char *TRAYMENU_HIDEACTION_CHINESE;
extern const char *TRAYMENU_QUITACTION_CHINESE;
extern const char *ORIGINALKEYLABEL_CHINESE;
extern const char *MAPPINGKEYLABEL_CHINESE;
extern const char *BURSTCHECKBOX_CHINESE;
extern const char *LOCKCHECKBOX_CHINESE;
extern const char *KEYUPACTIONCHECKBOX_CHINESE;
extern const char *PASSTHROUGHCHECKBOX_CHINESE;
extern const char *KEYSEQHOLDDOWNCHECKBOX_CHINESE;
extern const char *UPDATEBUTTON_CHINESE;
extern const char *SETTINGTAB_GENERAL_CHINESE;
extern const char *SETTINGTAB_MAPPING_CHINESE;
extern const char *SETTINGTAB_VGAMEPAD_CHINESE;
extern const char *SETTINGTAB_MULTIINPUT_CHINESE;
extern const char *SETTINGTAB_FORZA_CHINESE;

/* English const Strings */
extern const char* REFRESHBUTTON_ENGLISH;
extern const char* KEYMAPBUTTON_START_ENGLISH;
extern const char* KEYMAPBUTTON_STOP_ENGLISH;
extern const char* SAVEMAPLISTBUTTON_ENGLISH;
extern const char* DELETEONEBUTTON_ENGLISH;
extern const char* CLEARALLBUTTON_ENGLISH;
extern const char* ADDMAPDATABUTTON_ENGLISH;
extern const char* NAMECHECKBOX_ENGLISH;
extern const char* TITLECHECKBOX_ENGLISH;
extern const char* ORIKEYLABEL_ENGLISH;
extern const char* ORIKEYSEQLABEL_ENGLISH;
extern const char* MAPKEYLABEL_ENGLISH;
extern const char *ORIKEYLISTLABEL_ENGLISH;
extern const char *MAPKEYLISTLABEL_ENGLISH;
extern const char* BURSTPRESSLABEL_ENGLISH;
extern const char* BURSTRELEASE_ENGLISH;
extern const char* WAITTIME_ENGLISH;
extern const char* LONGPRESS_ENGLISH;
extern const char* DOUBLEPRESS_ENGLISH;
extern const char* POINT_ENGLISH;
extern const char* MOUSEXSPEEDLABEL_ENGLISH;
extern const char* MOUSEYSPEEDLABEL_ENGLISH;
extern const char* REMOVESETTINGBUTTON_ENGLISH;
extern const char* DATAPORTLABEL_ENGLISH;
extern const char* BRAKETHRESHOLDLABEL_ENGLISH;
extern const char* ACCELTHRESHOLDLABEL_ENGLISH;
extern const char* AUTOSTARTMAPPINGCHECKBOX_ENGLISH;
extern const char *SENDTOSAMETITLEWINDOWSCHECKBOX_ENGLISH;
extern const char* AUTOSTARTUPCHECKBOX_ENGLISH;
extern const char* STARTUPMINIMIZEDCHECKBOX_ENGLISH;
extern const char* SOUNDEFFECTCHECKBOX_ENGLISH;
extern const char* WINDOWSWITCHKEYLABEL_ENGLISH;
extern const char* MAPPINGSWITCHKEYLABEL_ENGLISH;
extern const char* PROCESSINFOTABLE_COL1_ENGLISH;
extern const char* PROCESSINFOTABLE_COL2_ENGLISH;
extern const char* PROCESSINFOTABLE_COL3_ENGLISH;
extern const char* KEYMAPDATATABLE_COL1_ENGLISH;
extern const char* KEYMAPDATATABLE_COL2_ENGLISH;
extern const char* KEYMAPDATATABLE_COL3_ENGLISH;
extern const char* KEYMAPDATATABLE_COL4_ENGLISH;
#ifdef VIGEM_CLIENT_SUPPORT
extern const char* VIRTUALGAMEPADGROUPBOX_ENGLISH;
extern const char* VJOYXSENSLABEL_ENGLISH;
extern const char* VJOYYSENSLABEL_ENGLISH;
extern const char* VIGEMBUSSTATUSLABEL_UNAVAILABLE_ENGLISH;
extern const char* VIGEMBUSSTATUSLABEL_AVAILABLE_ENGLISH;
extern const char* INSTALLVIGEMBUSBUTTON_ENGLISH;
extern const char* UNINSTALLVIGEMBUSBUTTON_ENGLISH;
extern const char* ENABLEVIRTUALJOYSTICKCHECKBOX_ENGLISH;
extern const char* LOCKCURSORCHECKBOX_ENGLISH;
#endif
extern const char* MULTIINPUTGROUPBOX_ENGLISH;
extern const char* MULTIINPUTENABLECHECKBOX_ENGLISH;
extern const char* MULTIINPUTDEVICELISTBUTTON_ENGLISH;
extern const char *FILTERKEYSCHECKBOX_ENGLISH;
extern const char* INSTALLINTERCEPTIONBUTTON_ENGLISH;
extern const char* UNINSTALLINTERCEPTIONBUTTON_ENGLISH;
extern const char* MULTIINPUTSTATUSLABEL_UNAVAILABLE_ENGLISH;
extern const char* MULTIINPUTSTATUSLABEL_REBOOTREQUIRED_ENGLISH;
extern const char* MULTIINPUTSTATUSLABEL_AVAILABLE_ENGLISH;
extern const char* KEYBOARDSELECTLABEL_ENGLISH;
extern const char* MOUSESELECTLABEL_ENGLISH;
extern const char *TRAYMENU_SHOWACTION_ENGLISH;
extern const char *TRAYMENU_HIDEACTION_ENGLISH;
extern const char *TRAYMENU_QUITACTION_ENGLISH;
extern const char *ORIGINALKEYLABEL_ENGLISH;
extern const char *MAPPINGKEYLABEL_ENGLISH;
extern const char *BURSTCHECKBOX_ENGLISH;
extern const char *LOCKCHECKBOX_ENGLISH;
extern const char *KEYUPACTIONCHECKBOX_ENGLISH;
extern const char *PASSTHROUGHCHECKBOX_ENGLISH;
extern const char *KEYSEQHOLDDOWNCHECKBOX_ENGLISH;
extern const char *UPDATEBUTTON_ENGLISH;
extern const char *SETTINGTAB_GENERAL_ENGLISH;
extern const char *SETTINGTAB_MAPPING_ENGLISH;
extern const char *SETTINGTAB_VGAMEPAD_ENGLISH;
extern const char *SETTINGTAB_MULTIINPUT_ENGLISH;
extern const char *SETTINGTAB_FORZA_ENGLISH;

/* constant values for QInputDeviceListWindow */
extern const int DEVICE_TABLE_NUMBER_COLUMN;
extern const int DEVICE_TABLE_DEVICEDESC_COLUMN;
extern const int DEVICE_TABLE_HARDWAREID_COLUMN;
extern const int DEVICE_TABLE_VENDORID_COLUMN;
extern const int DEVICE_TABLE_PRODUCTID_COLUMN;
extern const int DEVICE_TABLE_VENDORSTR_COLUMN;
extern const int DEVICE_TABLE_PRODUCTSTR_COLUMN;
extern const int DEVICE_TABLE_MANUFACTURER_COLUMN;
extern const int DEVICE_TABLE_DISABLE_COLUMN;

extern const int KEYBOARD_TABLE_COLUMN_COUNT;
extern const int MOUSE_TABLE_COLUMN_COUNT;

extern const char *DEVICELIST_WINDOWTITLE_ENGLISH;
extern const char *DEVICELIST_WINDOWTITLE_CHINESE;

extern const char *KEYBOARDLABEL_ENGLISH;
extern const char *MOUSELABEL_ENGLISH;

extern const char *KEYBOARDLABEL_CHINESE;
extern const char *MOUSELABEL_CHINESE;

extern const char *CONFIRMBUTTON_ENGLISH;
extern const char *CANCELBUTTON_ENGLISH;

extern const char *CONFIRMBUTTON_CHINESE;
extern const char *CANCELBUTTON_CHINESE;

extern const char *DEVICE_TABLE_COL0_ENGLISH;
extern const char *DEVICE_TABLE_COL1_ENGLISH;
extern const char *DEVICE_TABLE_COL2_ENGLISH;
extern const char *DEVICE_TABLE_COL3_ENGLISH;
extern const char *DEVICE_TABLE_COL4_ENGLISH;
extern const char *DEVICE_TABLE_COL5_ENGLISH;
extern const char *DEVICE_TABLE_COL6_ENGLISH;
extern const char *DEVICE_TABLE_COL7_ENGLISH;
extern const char *DEVICE_TABLE_COL8_ENGLISH;

extern const char *DEVICE_TABLE_COL0_CHINESE;
extern const char *DEVICE_TABLE_COL1_CHINESE;
extern const char *DEVICE_TABLE_COL2_CHINESE;
extern const char *DEVICE_TABLE_COL3_CHINESE;
extern const char *DEVICE_TABLE_COL4_CHINESE;
extern const char *DEVICE_TABLE_COL5_CHINESE;
extern const char *DEVICE_TABLE_COL6_CHINESE;
extern const char *DEVICE_TABLE_COL7_CHINESE;
extern const char *DEVICE_TABLE_COL8_CHINESE;

#endif // QKEYMAPPER_CONSTANTS_H
