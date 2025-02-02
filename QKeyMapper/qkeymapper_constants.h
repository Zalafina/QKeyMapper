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
extern const int KEYMAPPINGTABWIDGET_NARROW_LEFT;
extern const int KEYMAPPINGTABWIDGET_NARROW_WIDTH;
extern const int KEYMAPPINGTABWIDGET_WIDE_LEFT;
extern const int KEYMAPPINGTABWIDGET_WIDE_WIDTH;
extern const int KEYMAPPINGDATATABLE_NARROW_LEFT;
extern const int KEYMAPPINGDATATABLE_NARROW_WIDTH;
extern const int KEYMAPPINGDATATABLE_WIDE_LEFT;
extern const int KEYMAPPINGDATATABLE_WIDE_WIDTH;
extern const int KEYMAPPINGDATATABLE_TOP;
extern const int KEYMAPPINGDATATABLE_HEIGHT;
extern const int DEFAULT_ICON_WIDTH;
extern const int DEFAULT_ICON_HEIGHT;
extern const int MOUSEWHEEL_SCROLL_NONE;
extern const int MOUSEWHEEL_SCROLL_UP;
extern const int MOUSEWHEEL_SCROLL_DOWN;
extern const Qt::Key KEY_REFRESH;
extern const Qt::Key KEY_RECORD_START;
extern const Qt::Key KEY_RECORD_STOP;
// extern const Qt::Key KEY_PASSTHROUGH;
// extern const Qt::Key KEY_KEYUP_ACTION;
// extern const Qt::Key KEY_REMOVE_LAST;
extern const int LANGUAGE_CHINESE;
extern const int LANGUAGE_ENGLISH;
extern const int UPDATE_SITE_GITHUB;
extern const int UPDATE_SITE_GITEE;
extern const int KEYPRESS_TYPE_LONGPRESS;
extern const int KEYPRESS_TYPE_DOUBLEPRESS;
extern const int GLOBALSETTING_INDEX;
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
extern const int REPEAT_MODE_NONE;
extern const int REPEAT_MODE_BYKEY;
extern const int REPEAT_MODE_BYTIMES;
extern const int REPEAT_TIMES_MIN;
extern const int REPEAT_TIMES_MAX;
extern const int REPEAT_TIMES_DEFAULT;
extern const char *CROSSHAIR_CENTERCOLOR_DEFAULT;
extern const char *CROSSHAIR_CROSSHAIRCOLOR_DEFAULT;
extern const int CROSSHAIR_OPACITY_MIN;
extern const int CROSSHAIR_OPACITY_MAX;
extern const int CROSSHAIR_CENTERSIZE_MIN;
extern const int CROSSHAIR_CENTERSIZE_MAX;
extern const int CROSSHAIR_CENTERSIZE_DEFAULT;
extern const int CROSSHAIR_CENTEROPACITY_DEFAULT;
extern const int CROSSHAIR_CROSSHAIRWIDTH_MIN;
extern const int CROSSHAIR_CROSSHAIRWIDTH_MAX;
extern const int CROSSHAIR_CROSSHAIRWIDTH_DEFAULT;
extern const int CROSSHAIR_CROSSHAIRLENGTH_MIN;
extern const int CROSSHAIR_CROSSHAIRLENGTH_MAX;
extern const int CROSSHAIR_CROSSHAIRLENGTH_DEFAULT;
extern const int CROSSHAIR_CROSSHAIROPACITY_DEFAULT;
extern const int CROSSHAIR_X_OFFSET_MIN;
extern const int CROSSHAIR_X_OFFSET_MAX;
extern const int CROSSHAIR_X_OFFSET_DEFAULT;
extern const int CROSSHAIR_Y_OFFSET_MIN;
extern const int CROSSHAIR_Y_OFFSET_MAX;
extern const int CROSSHAIR_Y_OFFSET_DEFAULT;
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
extern const int NOTIFICATION_POSITION_NONE;
extern const int NOTIFICATION_POSITION_TOP_LEFT;
extern const int NOTIFICATION_POSITION_TOP_CENTER;
extern const int NOTIFICATION_POSITION_TOP_RIGHT;
extern const int NOTIFICATION_POSITION_BOTTOM_LEFT;
extern const int NOTIFICATION_POSITION_BOTTOM_CENTER;
extern const int NOTIFICATION_POSITION_BOTTOM_RIGHT;
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

extern const int INPUT_INIT;
extern const int INPUT_KEY_UP;
extern const int INPUT_KEY_DOWN;
extern const int INPUT_MOUSE_WHEEL;

extern const bool EXTENED_FLAG_TRUE;
extern const bool EXTENED_FLAG_FALSE;

extern const int INITIAL_ROW_INDEX;

extern const int INITIAL_PLAYER_INDEX;

extern const int INITIAL_KEYBOARD_INDEX;
extern const int INITIAL_MOUSE_INDEX;

extern const int HOOKPROC_STATE_STOPPED;
extern const int HOOKPROC_STATE_STARTED;
extern const int HOOKPROC_STATE_STOPPING;
extern const int HOOKPROC_STATE_STARTING;
extern const int HOOKPROC_STATE_RESTART_STOPPING;
extern const int HOOKPROC_STATE_RESTART_STARTING;

extern const int KEY_INTERCEPT_NONE;
extern const int KEY_INTERCEPT_BLOCK;
extern const int KEY_INTERCEPT_PASSTHROUGH;
extern const int KEY_INTERCEPT_BLOCK_COMBINATIONKEYUP;
extern const int KEY_INTERCEPT_BLOCK_KEY_RECORD;

extern const int KEY_PROC_NONE;
extern const int KEY_PROC_BURST;
extern const int KEY_PROC_LOCK;
extern const int KEY_PROC_LOCK_PASSTHROUGH;
extern const int KEY_PROC_PASSTHROUGH;

extern const int SENDTIMING_NORMAL;
extern const int SENDTIMING_KEYDOWN;
extern const int SENDTIMING_KEYUP;
extern const int SENDTIMING_KEYDOWN_AND_KEYUP;
extern const int SENDTIMING_NORMAL_AND_KEYUP;

extern const uint LOCK_STATE_LOCKOFF;
extern const uint LOCK_STATE_LOCKON;
extern const uint LOCK_STATE_LOCKON_PLUS;

extern const int SPLIT_WITH_PLUS;
extern const int SPLIT_WITH_NEXT;
extern const int SPLIT_WITH_PLUSANDNEXT;

extern const int REMOVE_MAPPINGTAB_FAILED;
extern const int REMOVE_MAPPINGTAB_LASTONE;
extern const int REMOVE_MAPPINGTAB_SUCCESS;

extern const int TRY_LOCK_WAIT_TIME;

extern const int SENDVIRTUALKEY_STATE_NORMAL;
extern const int SENDVIRTUALKEY_STATE_FORCE;
extern const int SENDVIRTUALKEY_STATE_MODIFIERS;
extern const int SENDVIRTUALKEY_STATE_BURST_TIMEOUT;
extern const int SENDVIRTUALKEY_STATE_BURST_STOP;
extern const int SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL;
extern const int SENDVIRTUALKEY_STATE_KEYSEQ_HOLDDOWN;
extern const int SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT;

extern const int SENDMODE_NORMAL;
extern const int SENDMODE_FORCE_STOP;
extern const int SENDMODE_BURSTKEY_START;
extern const int SENDMODE_BURSTKEY_STOP;
extern const int SENDMODE_BURSTKEY_STOP_ON_HOOKSTOPPED;
extern const int SENDMODE_BURSTKEY_STOP_ON_HOOKRESTART;
extern const int SENDMODE_KEYSEQ_NORMAL;
extern const int SENDMODE_KEYSEQ_HOLDDOWN;
extern const int SENDMODE_KEYSEQ_REPEAT;

extern const int SENDTYPE_NORMAL;
extern const int SENDTYPE_DOWN;
extern const int SENDTYPE_UP;
extern const int SENDTYPE_BOTH;
extern const int SENDTYPE_EXCLUSION;

extern const int SENDTYPE_BOTH_WAITTIME;

extern const int INPUTSTOP_NONE;
extern const int INPUTSTOP_SINGLE;
extern const int INPUTSTOP_KEYSEQ;

extern const int MOUSE_WHEEL_UP;
extern const int MOUSE_WHEEL_DOWN;

extern const int MOUSE_WHEEL_KEYUP_WAITTIME;

extern const int SETMOUSEPOSITION_WAITTIME_MAX;

extern const WORD XBUTTON_NONE;

extern const int JOYSTICK_PLAYER_INDEX_MIN;
extern const int JOYSTICK_PLAYER_INDEX_MAX;

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

extern const BYTE VJOY_PUSHLEVEL_MIN;
extern const BYTE VJOY_PUSHLEVEL_MAX;

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

extern const int MOUSE2VJOY_RECENTER_TIMEOUT_MIN;
extern const int MOUSE2VJOY_RECENTER_TIMEOUT_MAX;
extern const int MOUSE2VJOY_RECENTER_TIMEOUT_DEFAULT;

extern const int VJOY_KEYUP_WAITTIME;
extern const quint8 VK_MOUSE2VJOY_HOLD;
#endif

extern const quint8 VK_KEY2MOUSE_UP;
extern const quint8 VK_KEY2MOUSE_DOWN;
extern const quint8 VK_KEY2MOUSE_LEFT;
extern const quint8 VK_KEY2MOUSE_RIGHT;
extern const quint8 VK_GAMEPAD_HOME;
extern const quint8 VK_CROSSHAIR_NORMAL;
extern const quint8 VK_CROSSHAIR_TYPEA;

extern const int INTERCEPTION_RETURN_NORMALSEND;
extern const int INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION;
extern const int INTERCEPTION_RETURN_BLOCKEDBY_LOWLEVELHOOK;

extern const unsigned int INTERCEPTION_EXTRA_INFO;
extern const unsigned int INTERCEPTION_EXTRA_INFO_BLOCKED;

extern ULONG_PTR VIRTUAL_KEY_SEND;
extern ULONG_PTR VIRTUAL_KEY_SEND_NORMAL;
extern ULONG_PTR VIRTUAL_KEY_SEND_FORCE;
extern ULONG_PTR VIRTUAL_KEY_SEND_MODIFIERS;
extern ULONG_PTR VIRTUAL_KEY_SEND_BURST_TIMEOUT;
extern ULONG_PTR VIRTUAL_KEY_SEND_KEYSEQ_NORMAL;
extern ULONG_PTR VIRTUAL_KEY_SEND_BURST_STOP;
extern ULONG_PTR VIRTUAL_KEY_SEND_KEYSEQ_REPEAT;
extern ULONG_PTR VIRTUAL_MOUSE_POINTCLICK;
extern ULONG_PTR VIRTUAL_MOUSE_WHEEL;
extern ULONG_PTR VIRTUAL_KEY_OVERLAY;
extern ULONG_PTR VIRTUAL_RESEND_REALKEY;

extern const ULONG_PTR VIRTUAL_UNICODE_CHAR;
extern const ULONG_PTR VIRTUAL_CUSTOM_KEYS;
extern const ULONG_PTR VIRTUAL_MOUSE_MOVE;
extern const ULONG_PTR VIRTUAL_MOUSE_MOVE_BYKEYS;

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
extern const char *CHECK_UPDATES_URL_GITHUB;
extern const char *CHECK_UPDATES_URL_GITEE;
extern const char *UPDATER_USER_AGENT_X64;
extern const char *UPDATE_DOWNLOAD_DIR;
extern const char *SETTINGS_BACKUP_DIR;
extern const char *ZIPUPDATER_DIR;
extern const char *ZIPUPDATER_EXE;
extern const char *ZIPUPDATER_DLL_MSVCP;
extern const char *ZIPUPDATER_DLL_VCRUNTIME;
extern const char *LANGUAGECODE_CHINESE;
extern const char *LANGUAGECODE_ENGLISH;
extern const char *LANGUAGECODE_ENGLISH_US;
extern const char *PREFIX_SHORTCUT;
extern const char *OLD_PREFIX_SHORTCUT;
extern const char *JOIN_DEVICE;
extern const char *PREFIX_PASSTHROUGH;
extern const char *PREFIX_SEND_DOWN;
extern const char *PREFIX_SEND_UP;
extern const char *PREFIX_SEND_BOTH;
extern const char *PREFIX_SEND_EXCLUSION;
extern const char *KEYBOARD_MODIFIERS;
extern const char *KEYSEQUENCE_STR;
extern const char *KEYSEQUENCE_FINAL_STR;
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
extern const char *KEY_RECORD_START_STR;
extern const char *KEY_RECORD_STOP_STR;
extern const char *SUCCESS_COLOR;
extern const char *FAILURE_COLOR;
extern const char *WARNING_COLOR;
extern const char* LAST_WINDOWPOSITION;
extern const char* LANGUAGE_INDEX;
extern const char *SHOW_PROCESSLIST;
extern const char *SHOW_NOTES;
extern const char *NOTIFICATION_POSITION;
extern const char *UPDATE_SITE;
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
extern const char *KEYMAPDATA_MAPPINGKEYS_KEYUP;
extern const char *KEYMAPDATA_NOTE;
extern const char* KEYMAPDATA_BURST;
extern const char* KEYMAPDATA_LOCK;
extern const char *KEYMAPDATA_MAPPINGKEYUNLOCK;
extern const char *KEYMAPDATA_POSTMAPPINGKEY;
extern const char *KEYMAPDATA_CHECKCOMBKEYORDER;
extern const char *KEYMAPDATA_UNBREAKABLE;
extern const char *KEYMAPDATA_PASSTHROUGH;
// extern const char *KEYMAPDATA_KEYUP_ACTION;
extern const char *KEYMAPDATA_SENDTIMING;
extern const char *KEYMAPDATA_KEYSEQHOLDDOWN;
extern const char* KEYMAPDATA_BURSTPRESS_TIME;
extern const char* KEYMAPDATA_BURSTRELEASE_TIME;
extern const char *KEYMAPDATA_REPEATMODE;
extern const char *KEYMAPDATA_REPEATIMES;
extern const char *KEYMAPDATA_CROSSHAIR_CENTERCOLOR;
extern const char *KEYMAPDATA_CROSSHAIR_CENTERSIZE;
extern const char *KEYMAPDATA_CROSSHAIR_CENTEROPACITY;
extern const char *KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR;
extern const char *KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH;
extern const char *KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH;
extern const char *KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY;
extern const char *KEYMAPDATA_CROSSHAIR_SHOWCENTER;
extern const char *KEYMAPDATA_CROSSHAIR_SHOWTOP;
extern const char *KEYMAPDATA_CROSSHAIR_SHOWBOTTOM;
extern const char *KEYMAPDATA_CROSSHAIR_SHOWLEFT;
extern const char *KEYMAPDATA_CROSSHAIR_SHOWRIGHT;
extern const char *KEYMAPDATA_CROSSHAIR_X_OFFSET;
extern const char *KEYMAPDATA_CROSSHAIR_Y_OFFSET;
extern const char* KEY2MOUSE_X_SPEED;
extern const char* KEY2MOUSE_Y_SPEED;
#ifdef VIGEM_CLIENT_SUPPORT
extern const char* MOUSE2VJOY_X_SENSITIVITY;
extern const char* MOUSE2VJOY_Y_SENSITIVITY;
extern const char* MOUSE2VJOY_LOCKCURSOR;
extern const char *MOUSE2VJOY_DIRECTMODE;
extern const char *MOUSE2VJOY_RECENTER_TIMEOUT;
#endif
extern const char *MAPPINGTABLE_LASTTABINDEX;
extern const char *MAPPINGTABLE_TABNAMELIST;
extern const char *MAPPINGTABLE_TABHOTKEYLIST;
extern const char* CLEARALL;
extern const char* PROCESSINFO_FILENAME;
extern const char* PROCESSINFO_WINDOWTITLE;
extern const char* PROCESSINFO_FILEPATH;
extern const char* PROCESSINFO_FILENAME_CHECKED;
extern const char* PROCESSINFO_WINDOWTITLE_CHECKED;
extern const char *PROCESSINFO_DESCRIPTION;
extern const char* DATAPORT_NUMBER;
extern const char* GRIP_THRESHOLD_BRAKE;
extern const char* GRIP_THRESHOLD_ACCEL;
extern const char* AUTOSTARTMAPPING_CHECKED;
extern const char *SENDTOSAMEWINDOWS_CHECKED;
extern const char *ACCEPTVIRTUALGAMEPADINPUT_CHECKED;
extern const char* MAPPINGSTART_KEY;
extern const char *MAPPINGSTOP_KEY;
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
extern const char *SETUPDIALOG_MAPKEY_LINEEDIT_NAME;
extern const char *SETUPDIALOG_MAPKEY_KEYUP_LINEEDIT_NAME;
extern const char* WINDOWSWITCHKEY_LINEEDIT_NAME;
extern const char* MAPPINGSTARTKEY_LINEEDIT_NAME;
extern const char* MAPPINGSTOPKEY_LINEEDIT_NAME;
extern const char *MAPPINGTABLE_TAB_TEXT;
extern const char *ADDTAB_TAB_TEXT;
extern const char *KEY_NONE_STR;
extern const char* KEY_BLOCKED_STR;
extern const char *GAMEPAD_HOME_STR;
extern const char *MOUSE_MOVE_PREFIX;
extern const char* MOUSE_BUTTON_PREFIX;
extern const char* MOUSE_WINDOWPOINT_POSTFIX;
extern const char* MOUSE_SCREENPOINT_POSTFIX;
extern const char* MOUSE_L_STR;
extern const char* MOUSE_R_STR;
extern const char* MOUSE_M_STR;
extern const char* MOUSE_X1_STR;
extern const char* MOUSE_X2_STR;
extern const char* MOUSE_L_WINDOWPOINT_STR;
extern const char* MOUSE_R_WINDOWPOINT_STR;
extern const char* MOUSE_M_WINDOWPOINT_STR;
extern const char* MOUSE_X1_WINDOWPOINT_STR;
extern const char* MOUSE_X2_WINDOWPOINT_STR;
extern const char *MOUSE_L_SCREENPOINT_STR;
extern const char *MOUSE_R_SCREENPOINT_STR;
extern const char *MOUSE_M_SCREENPOINT_STR;
extern const char *MOUSE_X1_SCREENPOINT_STR;
extern const char *MOUSE_X2_SCREENPOINT_STR;
extern const char *MOUSE_MOVE_WINDOWPOINT_STR;
extern const char *MOUSE_MOVE_SCREENPOINT_STR;
extern const int SHOW_MODE_NONE;
extern const int SHOW_MODE_SCREEN_MOUSEPOINTS;
extern const int SHOW_MODE_WINDOW_MOUSEPOINTS;
extern const int SHOW_MODE_CROSSHAIR_NORMAL;
extern const int SHOW_MODE_CROSSHAIR_TYPEA;
extern const int PICK_WINDOW_POINT_KEY;
extern const int PICK_SCREEN_POINT_KEY;
extern const char *SENDTIMING_STR_NORMAL;
extern const char *SENDTIMING_STR_KEYDOWN;
extern const char *SENDTIMING_STR_KEYUP;
extern const char *SENDTIMING_STR_KEYDOWN_AND_KEYUP;
extern const char *SENDTIMING_STR_NORMAL_AND_KEYUP;
extern const char *SHOW_KEY_DEBUGINFO;
extern const char *SHOW_POINTS_IN_WINDOW_KEY;
extern const char *SHOW_POINTS_IN_SCREEN_KEY;
extern const char *SHOW_CAR_ORDINAL_KEY;
extern const char *MOUSE_WHEEL_STR;
extern const char* MOUSE_WHEEL_UP_STR;
extern const char* MOUSE_WHEEL_DOWN_STR;
extern const char *MOUSE_WHEEL_LEFT_STR;
extern const char *MOUSE_WHEEL_RIGHT_STR;
extern const char *SETTING_DESCRIPTION_FORMAT;
extern const char *ORIKEY_WITHNOTE_FORMAT;
extern const char *SENDTEXT_STR;
extern const char *KEYSEQUENCEBREAK_STR;
extern const char* JOY_KEY_PREFIX;
extern const char* VJOY_KEY_PREFIX;
extern const char* VJOY_MOUSE2LS_STR;
extern const char* VJOY_MOUSE2RS_STR;
extern const char* MOUSE2VJOY_PREFIX;
extern const char* MOUSE2VJOY_HOLD_KEY_STR;
extern const char *MOUSE2VJOY_DIRECT_KEY_STR_DEPRECATED;
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
extern const char *CROSSHAIR_PREFIX;
extern const char *CROSSHAIR_NORMAL_STR;
extern const char *CROSSHAIR_TYPEA_STR;
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

/* Translate const Strings */
extern const char *CROSSHAIRSETUPBUTTON_STR;

/* Chinese const Strings */
extern const char* REFRESHBUTTON_CHINESE;
extern const char* KEYMAPBUTTON_START_CHINESE;
extern const char* KEYMAPBUTTON_STOP_CHINESE;
extern const char* SAVEMAPLISTBUTTON_CHINESE;
extern const char* DELETEONEBUTTON_CHINESE;
extern const char* CLEARALLBUTTON_CHINESE;
extern const char *PROCESSLISTBUTTON_CHINESE;
extern const char *SHOWNOTESBUTTON_CHINESE;
extern const char* ADDMAPDATABUTTON_CHINESE;
extern const char* NAMECHECKBOX_CHINESE;
extern const char* TITLECHECKBOX_CHINESE;
extern const char *SETTINGDESCLABEL_CHINESE;
extern const char *GLOBALSETTING_DESC_CHINESE;
extern const char* ORIKEYLABEL_CHINESE;
extern const char* ORIKEYSEQLABEL_CHINESE;
extern const char* MAPKEYLABEL_CHINESE;
extern const char *ORIKEYLISTLABEL_CHINESE;
extern const char *MAPKEYLISTLABEL_CHINESE;
extern const char* BURSTPRESSLABEL_CHINESE;
extern const char* BURSTRELEASE_CHINESE;
extern const char *MILLISECOND_SUFFIX_CHINESE;
extern const char* WAITTIME_CHINESE;
extern const char *PUSHLEVEL_CHINESE;
extern const char *SENDTEXTLABEL_CHINESE;
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
extern const char *ACCEPTVIRTUALGAMEPADINPUTCHECKBOX_CHINESE;
extern const char* AUTOSTARTUPCHECKBOX_CHINESE;
extern const char* STARTUPMINIMIZEDCHECKBOX_CHINESE;
extern const char* SOUNDEFFECTCHECKBOX_CHINESE;
extern const char *NOTIFICATIONLABEL_CHINESE;
extern const char *LANGUAGELABEL_CHINESE;
extern const char *UPDATESITELABEL_CHINESE;
extern const char* WINDOWSWITCHKEYLABEL_CHINESE;
extern const char* MAPPINGSTARTKEYLABEL_CHINESE;
extern const char *MAPPINGSTOPKEYLABEL_CHINESE;
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
extern const char *VJOYRECENTERLABEL_CHINESE;
extern const char *VJOYRECENTERSPINBOX_UNRECENTER_CHINESE;
extern const char* VIGEMBUSSTATUSLABEL_UNAVAILABLE_CHINESE;
extern const char* VIGEMBUSSTATUSLABEL_AVAILABLE_CHINESE;
extern const char* INSTALLVIGEMBUSBUTTON_CHINESE;
extern const char* UNINSTALLVIGEMBUSBUTTON_CHINESE;
extern const char* ENABLEVIRTUALJOYSTICKCHECKBOX_CHINESE;
extern const char* LOCKCURSORCHECKBOX_CHINESE;
extern const char *DIRECTMODECHECKBOX_CHINESE;
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
extern const char *GAMEPADSELECTLABEL_CHINESE;
extern const char *TRAYMENU_SHOWACTION_CHINESE;
extern const char *TRAYMENU_HIDEACTION_CHINESE;
extern const char *TRAYMENU_QUITACTION_CHINESE;
extern const char *ORIGINALKEYLABEL_CHINESE;
extern const char *MAPPINGKEYLABEL_CHINESE;
extern const char *KEYUPMAPPINGLABEL_CHINESE;
extern const char *RECORDKEYSBUTTON_CHINESE;
extern const char *RECORDSTARTBUTTON_CHINESE;
extern const char *RECORDSTOPBUTTON_CHINESE;
extern const char *ITEMNOTELABEL_CHINESE;
extern const char *BURSTCHECKBOX_CHINESE;
extern const char *LOCKCHECKBOX_CHINESE;
extern const char *MAPPINGKEYUNLOCKCHECKBOX_CHINESE;
extern const char *POSTMAPPINGKEYCHECKBOX_CHINESE;
extern const char *KEYUPACTIONCHECKBOX_CHINESE;
extern const char *SENDTIMINGLABEL_CHINESE;
extern const char *CHECKCOMBKEYORDERCHECKBOX_CHINESE;
extern const char *UNBREAKABLECHECKBOX_CHINESE;
extern const char *PASSTHROUGHCHECKBOX_CHINESE;
extern const char *KEYSEQHOLDDOWNCHECKBOX_CHINESE;
extern const char *REPEATBYKEYCHECKBOX_CHINESE;
extern const char *REPEATBYTIMESCHECKBOX_CHINESE;
extern const char *UPDATEBUTTON_CHINESE;
extern const char *TABNAMELABEL_CHINESE;
extern const char *TABHOTKEYLABEL_CHINESE;
extern const char *EXPORTTABLEBUTTON_CHINESE;
extern const char *IMPORTTABLEBUTTON_CHINESE;
extern const char *REMOVETABLEBUTTON_CHINESE;
extern const char *SETTINGTAB_GENERAL_CHINESE;
extern const char *SETTINGTAB_MAPPING_CHINESE;
extern const char *SETTINGTAB_VGAMEPAD_CHINESE;
extern const char *SETTINGTAB_MULTIINPUT_CHINESE;
extern const char *SETTINGTAB_FORZA_CHINESE;
extern const char *POSITION_NONE_STR_CHINESE;
extern const char *POSITION_TOP_LEFT_STR_CHINESE;
extern const char *POSITION_TOP_CENTER_STR_CHINESE;
extern const char *POSITION_TOP_RIGHT_STR_CHINESE;
extern const char *POSITION_BOTTOM_LEFT_STR_CHINESE;
extern const char *POSITION_BOTTOM_CENTER_STR_CHINESE;
extern const char *POSITION_BOTTOM_RIGHT_STR_CHINESE;
extern const char *SENDTIMING_NORMAL_STR_CHINESE;
extern const char *SENDTIMING_KEYDOWN_STR_CHINESE;
extern const char *SENDTIMING_KEYUP_STR_CHINESE;
extern const char *SENDTIMING_KEYDOWN_AND_KEYUP_STR_CHINESE;
extern const char *SENDTIMING_NORMAL_AND_KEYUP_STR_CHINESE;
extern const char *CHECKUPDATEBUTTON_CHINESE;

/* English const Strings */
extern const char* REFRESHBUTTON_ENGLISH;
extern const char* KEYMAPBUTTON_START_ENGLISH;
extern const char* KEYMAPBUTTON_STOP_ENGLISH;
extern const char* SAVEMAPLISTBUTTON_ENGLISH;
extern const char* DELETEONEBUTTON_ENGLISH;
extern const char* CLEARALLBUTTON_ENGLISH;
extern const char *PROCESSLISTBUTTON_ENGLISH;
extern const char *SHOWNOTESBUTTON_ENGLISH;
extern const char* ADDMAPDATABUTTON_ENGLISH;
extern const char* NAMECHECKBOX_ENGLISH;
extern const char* TITLECHECKBOX_ENGLISH;
extern const char *SETTINGDESCLABEL_ENGLISH;
extern const char *GLOBALSETTING_DESC_ENGLISH;
extern const char* ORIKEYLABEL_ENGLISH;
extern const char* ORIKEYSEQLABEL_ENGLISH;
extern const char* MAPKEYLABEL_ENGLISH;
extern const char *ORIKEYLISTLABEL_ENGLISH;
extern const char *MAPKEYLISTLABEL_ENGLISH;
extern const char* BURSTPRESSLABEL_ENGLISH;
extern const char* BURSTRELEASE_ENGLISH;
extern const char *MILLISECOND_SUFFIX_ENGLISH;
extern const char* WAITTIME_ENGLISH;
extern const char *PUSHLEVEL_ENGLISH;
extern const char* SENDTEXTLABEL_ENGLISH;
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
extern const char *ACCEPTVIRTUALGAMEPADINPUTCHECKBOX_ENGLISH;
extern const char* AUTOSTARTUPCHECKBOX_ENGLISH;
extern const char* STARTUPMINIMIZEDCHECKBOX_ENGLISH;
extern const char* SOUNDEFFECTCHECKBOX_ENGLISH;
extern const char *NOTIFICATIONLABEL_ENGLISH;
extern const char *LANGUAGELABEL_ENGLISH;
extern const char *UPDATESITELABEL_ENGLISH;
extern const char* WINDOWSWITCHKEYLABEL_ENGLISH;
extern const char* MAPPINGSTARTKEYLABEL_ENGLISH;
extern const char *MAPPINGSTOPKEYLABEL_ENGLISH;
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
extern const char *VJOYRECENTERLABEL_ENGLISH;
extern const char *VJOYRECENTERSPINBOX_UNRECENTER_ENGLISH;
extern const char* VIGEMBUSSTATUSLABEL_UNAVAILABLE_ENGLISH;
extern const char* VIGEMBUSSTATUSLABEL_AVAILABLE_ENGLISH;
extern const char* INSTALLVIGEMBUSBUTTON_ENGLISH;
extern const char* UNINSTALLVIGEMBUSBUTTON_ENGLISH;
extern const char* ENABLEVIRTUALJOYSTICKCHECKBOX_ENGLISH;
extern const char* LOCKCURSORCHECKBOX_ENGLISH;
extern const char *DIRECTMODECHECKBOX_ENGLISH;
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
extern const char *GAMEPADSELECTLABEL_ENGLISH;
extern const char *TRAYMENU_SHOWACTION_ENGLISH;
extern const char *TRAYMENU_HIDEACTION_ENGLISH;
extern const char *TRAYMENU_QUITACTION_ENGLISH;
extern const char *ORIGINALKEYLABEL_ENGLISH;
extern const char *MAPPINGKEYLABEL_ENGLISH;
extern const char *KEYUPMAPPINGLABEL_ENGLISH;
extern const char *ITEMNOTELABEL_ENGLISH;
extern const char *RECORDKEYSBUTTON_ENGLISH;
extern const char *RECORDSTARTBUTTON_ENGLISH;
extern const char *RECORDSTOPBUTTON_ENGLISH;
extern const char *BURSTCHECKBOX_ENGLISH;
extern const char *LOCKCHECKBOX_ENGLISH;
extern const char *MAPPINGKEYUNLOCKCHECKBOX_ENGLISH;
extern const char *POSTMAPPINGKEYCHECKBOX_ENGLISH;
extern const char *KEYUPACTIONCHECKBOX_ENGLISH;
extern const char *SENDTIMINGLABEL_ENGLISH;
extern const char *CHECKCOMBKEYORDERCHECKBOX_ENGLISH;
extern const char *UNBREAKABLECHECKBOX_ENGLISH;
extern const char *PASSTHROUGHCHECKBOX_ENGLISH;
extern const char *KEYSEQHOLDDOWNCHECKBOX_ENGLISH;
extern const char *REPEATBYKEYCHECKBOX_ENGLISH;
extern const char *REPEATBYTIMESCHECKBOX_ENGLISH;
extern const char *UPDATEBUTTON_ENGLISH;
extern const char *TABNAMELABEL_ENGLISH;
extern const char *TABHOTKEYLABEL_ENGLISH;
extern const char *EXPORTTABLEBUTTON_ENGLISH;
extern const char *IMPORTTABLEBUTTON_ENGLISH;
extern const char *REMOVETABLEBUTTON_ENGLISH;
extern const char *SETTINGTAB_GENERAL_ENGLISH;
extern const char *SETTINGTAB_MAPPING_ENGLISH;
extern const char *SETTINGTAB_VGAMEPAD_ENGLISH;
extern const char *SETTINGTAB_MULTIINPUT_ENGLISH;
extern const char *SETTINGTAB_FORZA_ENGLISH;
extern const char *POSITION_NONE_STR_ENGLISH;
extern const char *POSITION_TOP_LEFT_STR_ENGLISH;
extern const char *POSITION_TOP_CENTER_STR_ENGLISH;
extern const char *POSITION_TOP_RIGHT_STR_ENGLISH;
extern const char *POSITION_BOTTOM_LEFT_STR_ENGLISH;
extern const char *POSITION_BOTTOM_CENTER_STR_ENGLISH;
extern const char *POSITION_BOTTOM_RIGHT_STR_ENGLISH;
extern const char *SENDTIMING_NORMAL_STR_ENGLISH;
extern const char *SENDTIMING_KEYDOWN_STR_ENGLISH;
extern const char *SENDTIMING_KEYUP_STR_ENGLISH;
extern const char *SENDTIMING_KEYDOWN_AND_KEYUP_STR_ENGLISH;
extern const char *SENDTIMING_NORMAL_AND_KEYUP_STR_ENGLISH;
extern const char *CHECKUPDATEBUTTON_ENGLISH;

/* constant values for QItemSetupDialog */
extern const int ITEMSETUP_EDITING_MAPPINGKEY;
extern const int ITEMSETUP_EDITING_KEYUPMAPPINGKEY;

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

extern const char *ITEMSETUPDIALOG_WINDOWTITLE_ENGLISH;
extern const char *ITEMSETUPDIALOG_WINDOWTITLE_CHINESE;

extern const char *TABLESETUPDIALOG_WINDOWTITLE_ENGLISH;
extern const char *TABLESETUPDIALOG_WINDOWTITLE_CHINESE;

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

void generateVirtualInputRandomValues();

#endif // QKEYMAPPER_CONSTANTS_H
