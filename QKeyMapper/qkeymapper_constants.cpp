#include <QRandomGenerator>

#include "qkeymapper_worker.h"
#include "qkeymapper_constants.h"

/* constant values for QKeyMapper */
//const unsigned int WIN_TITLESTR_MAX = 200U;
const unsigned int CYCLE_CHECK_TIMEOUT = 300U;
const unsigned int CYCLE_CHECK_LOOPCOUNT_MAX = 100000U;
const unsigned int CYCLE_CHECK_LOOPCOUNT_RESET = 500U;

const unsigned int CYCLE_REFRESH_PROCESSINFOTABLE_TIMEOUT = 3000U;

const unsigned int GLOBAL_MAPPING_START_WAIT = 2100U / CYCLE_CHECK_TIMEOUT;

const int VIRTUALGAMEPADTYPECOMBOBOX_X = 610;

const int PROCESSINFO_TABLE_COLUMN_COUNT = 3;
const int KEYMAPPINGDATA_TABLE_COLUMN_COUNT = 4;

const int INITIAL_WINDOW_POSITION = -1;

const int PROCESS_NAME_COLUMN = 0;
const int PROCESS_PID_COLUMN = 1;
const int PROCESS_TITLE_COLUMN = 2;

const int PROCESS_NAME_COLUMN_WIDTH_MAX = 200;

const int ORIGINAL_KEY_COLUMN = 0;
const int MAPPING_KEY_COLUMN = 1;
const int BURST_MODE_COLUMN = 2;
const int LOCK_COLUMN = 3;

const int KEYMAPPINGTABWIDGET_NARROW_LEFT   = 526;
const int KEYMAPPINGTABWIDGET_NARROW_WIDTH  = 458;
const int KEYMAPPINGTABWIDGET_WIDE_LEFT     = 26;
const int KEYMAPPINGTABWIDGET_WIDE_WIDTH    = 958;

const int KEYMAPPINGDATATABLE_NARROW_LEFT   = 0;
const int KEYMAPPINGDATATABLE_NARROW_WIDTH  = 454;
const int KEYMAPPINGDATATABLE_WIDE_LEFT     = 0;
const int KEYMAPPINGDATATABLE_WIDE_WIDTH    = 954;

const int KEYMAPPINGDATATABLE_TOP       = 0;
const int KEYMAPPINGDATATABLE_HEIGHT    = 324;

const int DEFAULT_ICON_WIDTH = 48;
const int DEFAULT_ICON_HEIGHT = 48;

const int MOUSEWHEEL_SCROLL_NONE = 0;
const int MOUSEWHEEL_SCROLL_UP = 1;
const int MOUSEWHEEL_SCROLL_DOWN = 2;

const Qt::Key KEY_REFRESH           = Qt::Key_F5;
const Qt::Key KEY_RECORD_START      = Qt::Key_F11;
const Qt::Key KEY_RECORD_STOP       = Qt::Key_F12;
// const Qt::Key KEY_PASSTHROUGH   = Qt::Key_F12;
// const Qt::Key KEY_KEYUP_ACTION  = Qt::Key_F2;
// const Qt::Key KEY_REMOVE_LAST   = Qt::Key_Backspace;

const int LANGUAGE_CHINESE = 0;
const int LANGUAGE_ENGLISH = 1;

const int UPDATE_SITE_GITHUB = 0;
const int UPDATE_SITE_GITEE = 1;

const int KEYPRESS_TYPE_LONGPRESS   = 0;
const int KEYPRESS_TYPE_DOUBLEPRESS = 1;

const int GLOBALSETTING_INDEX = 1;

const int CUSTOMSETTING_INDEX_MAX = 30;

const int TITLESETTING_INDEX_INVALID = -1;
const int TITLESETTING_INDEX_ANYTITLE = 0;
const int TITLESETTING_INDEX_MAX = 9;

const int BURST_TIME_MIN = 1;
const int BURST_TIME_MAX = 99999;

const int BURST_PRESS_TIME_DEFAULT   = 40;
const int BURST_RELEASE_TIME_DEFAULT = 20;

const int MAPPING_WAITTIME_MIN = 0;
const int MAPPING_WAITTIME_MAX = 999999;

const int PRESSTIME_MIN = 0;
const int PRESSTIME_MAX = 9999;

const int REPEAT_MODE_NONE      = 0;
const int REPEAT_MODE_BYKEY     = 1;
const int REPEAT_MODE_BYTIMES   = 2;

const int REPEAT_TIMES_MIN = 1;
const int REPEAT_TIMES_MAX = 99999999;
const int REPEAT_TIMES_DEFAULT = 1;

const char *CROSSHAIR_CENTERCOLOR_DEFAULT = "70a1ff";
const char *CROSSHAIR_CROSSHAIRCOLOR_DEFAULT = "70a1ff";
const int CROSSHAIR_OPACITY_MIN = 0;
const int CROSSHAIR_OPACITY_MAX = 255;
const int CROSSHAIR_CENTERSIZE_MIN = 1;
const int CROSSHAIR_CENTERSIZE_MAX = 500;
const int CROSSHAIR_CENTERSIZE_DEFAULT = 2;
const int CROSSHAIR_CENTEROPACITY_DEFAULT = 200;
const int CROSSHAIR_CROSSHAIRWIDTH_MIN = 1;
const int CROSSHAIR_CROSSHAIRWIDTH_MAX = 500;
const int CROSSHAIR_CROSSHAIRWIDTH_DEFAULT = 2;
const int CROSSHAIR_CROSSHAIRLENGTH_MIN = 1;
const int CROSSHAIR_CROSSHAIRLENGTH_MAX = 900;
const int CROSSHAIR_CROSSHAIRLENGTH_DEFAULT = 30;
const int CROSSHAIR_CROSSHAIROPACITY_DEFAULT = 200;
const int CROSSHAIR_X_OFFSET_MIN = -5000;
const int CROSSHAIR_X_OFFSET_MAX = 5000;
const int CROSSHAIR_X_OFFSET_DEFAULT = 0;
const int CROSSHAIR_Y_OFFSET_MIN = -5000;
const int CROSSHAIR_Y_OFFSET_MAX = 5000;
const int CROSSHAIR_Y_OFFSET_DEFAULT = 0;

const int DATA_PORT_MIN = 1;
const int DATA_PORT_MAX = 65535;
const int DATA_PORT_DEFAULT = 5300;

const int GRIP_THRESHOLD_DECIMALS = 5;
const double GRIP_THRESHOLD_BRAKE_MIN = 0.00001;
const double GRIP_THRESHOLD_BRAKE_MAX = 1000.00000;
const double GRIP_THRESHOLD_BRAKE_DEFAULT = 0.47000;
const double GRIP_THRESHOLD_ACCEL_MIN = 0.00001;
const double GRIP_THRESHOLD_ACCEL_MAX = 1000.00000;
const double GRIP_THRESHOLD_ACCEL_DEFAULT = 1.00000;
const double GRIP_THRESHOLD_SINGLE_STEP = 0.01;

const int MOUSE_SPEED_MIN = 1;
const int MOUSE_SPEED_MAX = 15;
const int MOUSE_SPEED_DEFAULT = 3;

const int UI_SCALE_NORMAL = 0;
const int UI_SCALE_1K_PERCENT_100 = 1;
const int UI_SCALE_1K_PERCENT_125 = 2;
const int UI_SCALE_1K_PERCENT_150 = 3;
const int UI_SCALE_2K_PERCENT_100 = 4;
const int UI_SCALE_2K_PERCENT_125 = 5;
const int UI_SCALE_2K_PERCENT_150 = 6;
const int UI_SCALE_4K_PERCENT_100 = 7;
const int UI_SCALE_4K_PERCENT_125 = 8;
const int UI_SCALE_4K_PERCENT_150 = 9;

const int MOUSE_POINT_RADIUS = 12;

const int SHOW_POINTSIN_SCREEN_OFF  = 0;
const int SHOW_POINTSIN_SCREEN_ON   = 1;
const int SHOW_POINTSIN_WINDOW_OFF  = 2;
const int SHOW_POINTSIN_WINDOW_ON   = 3;

const int NOTIFICATION_POSITION_NONE            = 0;
const int NOTIFICATION_POSITION_TOP_LEFT        = 1;
const int NOTIFICATION_POSITION_TOP_CENTER      = 2;
const int NOTIFICATION_POSITION_TOP_RIGHT       = 3;
const int NOTIFICATION_POSITION_BOTTOM_LEFT     = 4;
const int NOTIFICATION_POSITION_BOTTOM_CENTER   = 5;
const int NOTIFICATION_POSITION_BOTTOM_RIGHT    = 6;

#ifdef VIGEM_CLIENT_SUPPORT
const int RECONNECT_VIGEMCLIENT_WAIT_TIME = 2000;
#endif

const int INSTALL_INTERCEPTION_LOOP_WAIT_TIME = 10;
const int INSTALL_INTERCEPTION_LOOP_WAIT_TIME_MAX = 2000/INSTALL_INTERCEPTION_LOOP_WAIT_TIME;

/* constant values for QKeyMapper_Worker */
const int KEY_INIT = -1;
const int KEY_UP = 0;
const int KEY_DOWN = 1;

const int INPUT_INIT        = -1;
const int INPUT_KEY_UP      = 0;
const int INPUT_KEY_DOWN    = 1;
const int INPUT_MOUSE_WHEEL = 2;

const bool EXTENED_FLAG_TRUE = true;
const bool EXTENED_FLAG_FALSE = false;

const int INITIAL_ROW_INDEX = -1;

const int INITIAL_PLAYER_INDEX = -1;

const int INITIAL_KEYBOARD_INDEX = -1;
const int INITIAL_MOUSE_INDEX = -1;

const int HOOKPROC_STATE_STOPPED            = 0;
const int HOOKPROC_STATE_STARTED            = 1;
const int HOOKPROC_STATE_STOPPING           = 2;
const int HOOKPROC_STATE_STARTING           = 3;
const int HOOKPROC_STATE_RESTART_STOPPING   = 4;
const int HOOKPROC_STATE_RESTART_STARTING   = 5;

const int KEY_INTERCEPT_NONE                            = 0;
const int KEY_INTERCEPT_BLOCK                           = 1;
const int KEY_INTERCEPT_PASSTHROUGH                     = 2;
const int KEY_INTERCEPT_BLOCK_COMBINATIONKEYUP          = 3;
const int KEY_INTERCEPT_BLOCK_KEY_RECORD                = 4;

const int KEY_PROC_NONE             = 0;
const int KEY_PROC_BURST            = 1;
const int KEY_PROC_LOCK             = 2;
const int KEY_PROC_LOCK_PASSTHROUGH = 3;
const int KEY_PROC_PASSTHROUGH      = 4;

const int SENDTIMING_NORMAL             = 0;
const int SENDTIMING_KEYDOWN            = 1;
const int SENDTIMING_KEYUP              = 2;
const int SENDTIMING_KEYDOWN_AND_KEYUP  = 3;
const int SENDTIMING_NORMAL_AND_KEYUP   = 4;

const uint LOCK_STATE_LOCKOFF       = 0;
const uint LOCK_STATE_LOCKON        = 1;
const uint LOCK_STATE_LOCKON_PLUS   = 2;

const int SPLIT_WITH_PLUS           = 0;
const int SPLIT_WITH_NEXT           = 1;
const int SPLIT_WITH_PLUSANDNEXT    = 2;

const int REMOVE_MAPPINGTAB_FAILED  = 0;
const int REMOVE_MAPPINGTAB_LASTONE = 1;
const int REMOVE_MAPPINGTAB_SUCCESS = 2;

const int TRY_LOCK_WAIT_TIME = 1000;

const int SENDVIRTUALKEY_STATE_NORMAL           = 0;
const int SENDVIRTUALKEY_STATE_FORCE            = 1;
const int SENDVIRTUALKEY_STATE_MODIFIERS        = 2;
const int SENDVIRTUALKEY_STATE_BURST_TIMEOUT    = 3;
const int SENDVIRTUALKEY_STATE_BURST_STOP       = 4;
const int SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL    = 5;
const int SENDVIRTUALKEY_STATE_KEYSEQ_HOLDDOWN  = 6;
const int SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT    = 7;

const int SENDMODE_NORMAL                           = 1;
const int SENDMODE_FORCE_STOP                       = 2;
const int SENDMODE_BURSTKEY_START                   = 3;
const int SENDMODE_BURSTKEY_STOP                    = 4;
const int SENDMODE_BURSTKEY_STOP_ON_HOOKSTOPPED     = 5;
const int SENDMODE_BURSTKEY_STOP_ON_HOOKRESTART     = 6;
const int SENDMODE_KEYSEQ_NORMAL                    = 7;
const int SENDMODE_KEYSEQ_HOLDDOWN                  = 8;
const int SENDMODE_KEYSEQ_REPEAT                    = 9;

const int SENDTYPE_NORMAL   = 0;
const int SENDTYPE_DOWN     = 1;
const int SENDTYPE_UP       = 2;
const int SENDTYPE_BOTH     = 3;
const int SENDTYPE_EXCLUSION = 4;

const int SENDTYPE_BOTH_WAITTIME = 20;

const int INPUTSTOP_NONE    = 0;
const int INPUTSTOP_SINGLE  = 1;
const int INPUTSTOP_KEYSEQ  = 2;

const int MOUSE_WHEEL_UP = 1;
const int MOUSE_WHEEL_DOWN = 2;

const int MOUSE_WHEEL_KEYUP_WAITTIME = 20;

const int SETMOUSEPOSITION_WAITTIME_MAX = 100;

const WORD XBUTTON_NONE = 0x0000;

const int JOYSTICK_PLAYER_INDEX_MIN = 0;
const int JOYSTICK_PLAYER_INDEX_MAX = 9;

const int JOYSTICK_POV_ANGLE_RELEASE = -1;
const int JOYSTICK_POV_ANGLE_UP      = 0;
const int JOYSTICK_POV_ANGLE_DOWN    = 180;
const int JOYSTICK_POV_ANGLE_LEFT    = 270;
const int JOYSTICK_POV_ANGLE_RIGHT   = 90;
const int JOYSTICK_POV_ANGLE_L_UP    = 315;
const int JOYSTICK_POV_ANGLE_L_DOWN  = 225;
const int JOYSTICK_POV_ANGLE_R_UP    = 45;
const int JOYSTICK_POV_ANGLE_R_DOWN  = 135;

const int JOYSTICK_AXIS_LS_HORIZONTAL    = 0;
const int JOYSTICK_AXIS_LS_VERTICAL      = 1;
const int JOYSTICK_AXIS_RS_HORIZONTAL    = 2;
const int JOYSTICK_AXIS_RS_VERTICAL      = 3;
const int JOYSTICK_AXIS_LT_BUTTON        = 4;
const int JOYSTICK_AXIS_RT_BUTTON        = 5;

const qreal JOYSTICK_AXIS_NEAR_ZERO_THRESHOLD = 1e-04;

const qreal JOYSTICK_AXIS_LT_RT_KEYUP_THRESHOLD      = 0.15;
const qreal JOYSTICK_AXIS_LT_RT_KEYDOWN_THRESHOLD    = 0.5;

const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD                = -0.5;
const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD              = 0.5;
const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MIN_THRESHOLD       = -0.15;
const qreal JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MAX_THRESHOLD       = 0.15;

const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD            = -0.5;
const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD           = 0.5;
const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MIN_THRESHOLD     = -0.15;
const qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MAX_THRESHOLD     = 0.15;

const qreal JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD  = -0.25;
const qreal JOYSTICK2MOUSE_AXIS_MINUS_MID_THRESHOLD  = -0.50;
const qreal JOYSTICK2MOUSE_AXIS_MINUS_HIGH_THRESHOLD  = -0.75;
const qreal JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD   = 0.25;
const qreal JOYSTICK2MOUSE_AXIS_PLUS_MID_THRESHOLD   = 0.50;
const qreal JOYSTICK2MOUSE_AXIS_PLUS_HIGH_THRESHOLD   = 0.75;

const int MOUSE_CURSOR_BOTTOMRIGHT_X = 65535;
const int MOUSE_CURSOR_BOTTOMRIGHT_Y = 65535;

const int KEY2MOUSE_CYCLECHECK_TIMEOUT = 2;

const int JOY2VJOY_LEFTSTICK_X   = 1;
const int JOY2VJOY_LEFTSTICK_Y   = 2;
const int JOY2VJOY_RIGHTSTICK_X  = 3;
const int JOY2VJOY_RIGHTSTICK_Y  = 4;

#ifdef VIGEM_CLIENT_SUPPORT
const USHORT VIRTUALGAMPAD_VENDORID_X360     = 0x045E;
const USHORT VIRTUALGAMPAD_PRODUCTID_X360    = 0xABCD;

const USHORT VIRTUALGAMPAD_VENDORID_DS4      = 0x054C;
const USHORT VIRTUALGAMPAD_PRODUCTID_DS4     = 0x05C4;
const char *VIRTUALGAMPAD_SERIAL_PREFIX_DS4 = "c0-13-37-";

const BYTE XINPUT_TRIGGER_MIN     = 0;
const BYTE XINPUT_TRIGGER_MAX     = 255;

const BYTE VJOY_PUSHLEVEL_MIN     = 0;
const BYTE VJOY_PUSHLEVEL_MAX     = 255;

const SHORT XINPUT_THUMB_MIN     = -32768;
const SHORT XINPUT_THUMB_RELEASE = 0;
const SHORT XINPUT_THUMB_MAX     = 32767;

const qreal THUMB_DISTANCE_MAX   = 32767;

// const BYTE AUTO_BRAKE_ADJUST_VALUE = 8;
// const BYTE AUTO_ACCEL_ADJUST_VALUE = 8;
// const BYTE AUTO_BRAKE_DEFAULT = 11 * AUTO_BRAKE_ADJUST_VALUE + 7;
// const BYTE AUTO_ACCEL_DEFAULT = 11 * AUTO_ACCEL_ADJUST_VALUE + 7;
const BYTE AUTO_BRAKE_ADJUST_VALUE = 4;
const BYTE AUTO_ACCEL_ADJUST_VALUE = 8;
const BYTE AUTO_BRAKE_DEFAULT = 23 * AUTO_BRAKE_ADJUST_VALUE + 3;
const BYTE AUTO_ACCEL_DEFAULT = 23 * AUTO_ACCEL_ADJUST_VALUE + 7;

const double GRIP_THRESHOLD_MAX = 1000.00000;

const qsizetype FORZA_MOTOR_7_SLED_DATA_LENGTH = 232;
const qsizetype FORZA_MOTOR_7_DASH_DATA_LENGTH = 311;
const qsizetype FORZA_MOTOR_8_DASH_DATA_LENGTH = 331;
const qsizetype FORZA_HORIZON_DATA_LENGTH = 324;

const qsizetype FIRAT_PART_DATA_LENGTH = 232;
const qsizetype SECOND_PART_DATA_LENGTH = 79;
const qsizetype FORZA_HORIZON_BUFFER_OFFSET = 12;

const int AUTO_ADJUST_NONE   = 0b0000;
const int AUTO_ADJUST_BRAKE  = 0b0001;
const int AUTO_ADJUST_ACCEL  = 0b0010;
const int AUTO_ADJUST_BOTH   = 0b0011;
const int AUTO_ADJUST_LT     = 0b0100;
const int AUTO_ADJUST_RT     = 0b1000;

const int VJOY_UPDATE_NONE           = 0;
const int VJOY_UPDATE_BUTTONS        = 1;
const int VJOY_UPDATE_JOYSTICKS      = 2;
const int VJOY_UPDATE_AUTO_BUTTONS   = 3;

const int VIRTUAL_JOYSTICK_SENSITIVITY_MIN = 1;
const int VIRTUAL_JOYSTICK_SENSITIVITY_MAX = 50000;
const int VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT = 4000;

const int VIRTUAL_GAMEPAD_NUMBER_MIN = 1;
const int VIRTUAL_GAMEPAD_NUMBER_MAX = 4;

const int MOUSE2VJOY_RECENTER_TIMEOUT_MIN = 0;
const int MOUSE2VJOY_RECENTER_TIMEOUT_MAX = 1000;
const int MOUSE2VJOY_RECENTER_TIMEOUT_DEFAULT = 50;

const int VJOY_KEYUP_WAITTIME = 20;
const quint8 VK_MOUSE2VJOY_HOLD = 0x3A;
#endif

const quint8 VK_KEY2MOUSE_UP    = 0x8A;
const quint8 VK_KEY2MOUSE_DOWN  = 0x8B;
const quint8 VK_KEY2MOUSE_LEFT  = 0x8C;
const quint8 VK_KEY2MOUSE_RIGHT = 0x8D;
const quint8 VK_GAMEPAD_HOME    = 0x07;
const quint8 VK_CROSSHAIR_NORMAL= 0x0A;
const quint8 VK_CROSSHAIR_TYPEA = 0x0B;

const int INTERCEPTION_RETURN_NORMALSEND = 0;
const int INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION = 1;
const int INTERCEPTION_RETURN_BLOCKEDBY_LOWLEVELHOOK = 2;

const unsigned int INTERCEPTION_EXTRA_INFO          = 0xAAAA0000;
const unsigned int INTERCEPTION_EXTRA_INFO_BLOCKED  = 0xBBBB0000;

// const ULONG_PTR VIRTUAL_KEY_SEND   = 0xACBDACBD;
// const ULONG_PTR VIRTUAL_MOUSE_POINTCLICK = 0xBBDFBBDF;
// const ULONG_PTR VIRTUAL_MOUSE_WHEEL      = 0xEBFAEBFA;
ULONG_PTR VIRTUAL_KEY_SEND;
ULONG_PTR VIRTUAL_KEY_SEND_NORMAL;
ULONG_PTR VIRTUAL_KEY_SEND_FORCE;
ULONG_PTR VIRTUAL_KEY_SEND_MODIFIERS;
ULONG_PTR VIRTUAL_KEY_SEND_BURST_TIMEOUT;
ULONG_PTR VIRTUAL_KEY_SEND_BURST_STOP;
ULONG_PTR VIRTUAL_KEY_SEND_KEYSEQ_NORMAL;
ULONG_PTR VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN;
ULONG_PTR VIRTUAL_KEY_SEND_KEYSEQ_REPEAT;
ULONG_PTR VIRTUAL_MOUSE_POINTCLICK;
ULONG_PTR VIRTUAL_MOUSE_WHEEL;
ULONG_PTR VIRTUAL_KEY_OVERLAY;
ULONG_PTR VIRTUAL_RESEND_REALKEY;

const ULONG_PTR VIRTUAL_UNICODE_CHAR     = 0xCDCACDC0;
const ULONG_PTR VIRTUAL_CUSTOM_KEYS      = 0x3A3A0000;
const ULONG_PTR VIRTUAL_MOUSE_MOVE       = 0xBFBCBFB0;
const ULONG_PTR VIRTUAL_MOUSE_MOVE_BYKEYS= 0x3F3F3F30;

const UINT SCANCODE_CTRL            = 0x1D;
const UINT SCANCODE_ALT             = 0x38;
const UINT SCANCODE_LSHIFT          = 0x2A;
const UINT SCANCODE_RSHIFT          = 0x36;
const UINT SCANCODE_DIVIDE          = 0x35;
const UINT SCANCODE_NUMLOCK         = 0x45;
const UINT SCANCODE_SNAPSHOT        = 0x54;
const UINT SCANCODE_PRINTSCREEN     = 0x37;
const UINT SCANCODE_LWIN            = 0x5B;
const UINT SCANCODE_RWIN            = 0x5C;
const UINT SCANCODE_APPS            = 0x5D;

/* constant chars for QKeyMapper */
const char *PROGRAM_NAME = "QKeyMapper";
const char *CHECK_UPDATES_URL_GITHUB = "https://api.github.com/repos/Zalafina/QKeyMapper/releases/latest";
const char *CHECK_UPDATES_URL_GITEE = "https://gitee.com/api/v5/repos/asukavov/QKeyMapper/releases/latest";
const char *UPDATER_USER_AGENT_X64 = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 Edg/131.0.0.0";
const char *UPDATE_DOWNLOAD_DIR = "update_files";
const char *SETTINGS_BACKUP_DIR = "settings_backup";
const char *ZIPUPDATER_DIR = "zipupdater";
const char *ZIPUPDATER_EXE = "zipupdater.exe";
const char *ZIPUPDATER_DLL_MSVCP = "msvcp140.dll";
const char *ZIPUPDATER_DLL_VCRUNTIME = "vcruntime140.dll";

const char *LANGUAGECODE_CHINESE = "zh_CN";
const char *LANGUAGECODE_ENGLISH = "en";
const char *LANGUAGECODE_ENGLISH_US = "en_US";

const char *PREFIX_SHORTCUT = "★";
const char *OLD_PREFIX_SHORTCUT = "★";
const char *JOIN_DEVICE     = "##";
const char *PREFIX_PASSTHROUGH = "$";
const char *PREFIX_SEND_DOWN    = "↓";
const char *PREFIX_SEND_UP      = "↑";
const char *PREFIX_SEND_BOTH    = "⇵";
const char *PREFIX_SEND_EXCLUSION = "！";

const char *KEYBOARD_MODIFIERS  = "KeyboardModifiers";
const char *KEYSEQUENCE_STR         = "KeySequence";
const char *KEYSEQUENCE_FINAL_STR   = "Final";
const char *HOLDDOWN_STR        = "HoldDown";
const char *CLEAR_VIRTUALKEYS   = "ClearVirtualKeys";

const char *PROCESS_UNKNOWN = "QKeyMapperUnknown";

const char *DEFAULT_NAME = "ForzaHorizon4.exe";
const char *CONFIG_FILENAME = "keymapdata.ini";
const char *CONFIG_LATEST_FILENAME = "keymapdata_latest.ini";
#ifdef SETTINGSFILE_CONVERT
const char *CONFIG_BACKUP_FILENAME = "keymapdata_backup.ini";
#endif

const char *DISPLAYSWITCH_KEY_DEFAULT    = "L-Ctrl+`";
const char *MAPPINGSWITCH_KEY_DEFAULT    = "L-Ctrl+F6";

const char *KEY_RECORD_START_STR    = "F11";
const char *KEY_RECORD_STOP_STR     = "F12";

const char *SUCCESS_COLOR = "#44bd32";
const char *FAILURE_COLOR = "#d63031";
const char *WARNING_COLOR = "#f39c12";

/* General global settings >>> */
const char *LAST_WINDOWPOSITION = "LastWindowPosition";
const char *LANGUAGE_INDEX = "LanguageIndex";
const char *SHOW_PROCESSLIST = "ShowProcessList";
const char *SHOW_NOTES = "ShowNotes";
const char *NOTIFICATION_POSITION = "NotificationPosition";
const char *UPDATE_SITE = "UpdateSite";
const char *SETTINGSELECT = "SettingSelect";
const char *AUTO_STARTUP = "AutoStartup";
const char *STARTUP_MINIMIZED = "StartupMinimized";
const char *PLAY_SOUNDEFFECT = "PlaySoundEffect";
const char *WINDOWSWITCH_KEYSEQ = "WindowSwitch_KeySequence";
#ifdef VIGEM_CLIENT_SUPPORT
const char *VIRTUALGAMEPAD_ENABLE = "VirtualGamepadEnable";
const char *VIRTUALGAMEPAD_TYPE = "VirtualGamepadType";
const char *VIRTUAL_GAMEPADLIST = "VirtualGamdpadList";
#endif
const char *MULTI_INPUT_ENABLE = "MultiInputEnable";
const char* FILTER_KEYS = "FilterKeys";
const char *DISABLED_KEYBOARDLIST = "DisabledKeyboardList";
const char *DISABLED_MOUSELIST = "DisabledMouseList";
/* General global settings <<< */

const char *GROUPNAME_EXECUTABLE_SUFFIX = ".exe";
const char *GROUPNAME_CUSTOMSETTING = "CustomSetting ";
const char *GROUPNAME_CUSTOMGLOBALSETTING = "CustomGlobalSetting ";
const char *GROUPNAME_GLOBALSETTING = "QKeyMapperGlobalSetting";
const char *WINDOWTITLE_STRING = "Title";
const char *ANYWINDOWTITLE_STRING = "AnyTitle";

const char *KEYMAPDATA_ORIGINALKEYS = "KeyMapData_OriginalKeys";
const char *KEYMAPDATA_MAPPINGKEYS = "KeyMapData_MappingKeys";
const char *KEYMAPDATA_MAPPINGKEYS_KEYUP = "KeyMapData_MappingKeys_KeyUp";
const char *KEYMAPDATA_NOTE = "KeyMapData_Note";
const char *KEYMAPDATA_BURST = "KeyMapData_Burst";
const char *KEYMAPDATA_LOCK = "KeyMapData_Lock";
const char *KEYMAPDATA_MAPPINGKEYUNLOCK = "KeyMapData_MappingKeyUnlock";
const char *KEYMAPDATA_POSTMAPPINGKEY = "KeyMapData_PostMappingKey";
const char *KEYMAPDATA_CHECKCOMBKEYORDER = "KeyMapData_CheckCombKeyOrder";
const char *KEYMAPDATA_UNBREAKABLE = "KeyMapData_Unbreakable";
const char *KEYMAPDATA_PASSTHROUGH = "KeyMapData_PassThrough";
// const char *KEYMAPDATA_KEYUP_ACTION = "KeyMapData_KeyUp_Action";
const char *KEYMAPDATA_SENDTIMING = "KeyMapData_SendTiming";
const char *KEYMAPDATA_KEYSEQHOLDDOWN = "KeyMapData_KeySeqHoldDown";
const char *KEYMAPDATA_BURSTPRESS_TIME = "KeyMapData_BurstPressTime";
const char *KEYMAPDATA_BURSTRELEASE_TIME = "KeyMapData_BurstReleaseTime";
const char *KEYMAPDATA_REPEATMODE = "KeyMapData_RepeatMode";
const char *KEYMAPDATA_REPEATIMES = "KeyMapData_RepeatTimes";
const char *KEYMAPDATA_CROSSHAIR_CENTERCOLOR = "KeyMapData_Crosshair_CenterColor";
const char *KEYMAPDATA_CROSSHAIR_CENTERSIZE = "KeyMapData_Crosshair_CenterSize";
const char *KEYMAPDATA_CROSSHAIR_CENTEROPACITY = "KeyMapData_Crosshair_CenterOpacity";
const char *KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR = "KeyMapData_Crosshair_CrosshairColor";
const char *KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH = "KeyMapData_Crosshair_CrosshairWidth";
const char *KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH = "KeyMapData_Crosshair_CrosshairLength";
const char *KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY = "KeyMapData_Crosshair_CrosshairOpacity";
const char *KEYMAPDATA_CROSSHAIR_SHOWCENTER = "KeyMapData_Crosshair_ShowCenter";
const char *KEYMAPDATA_CROSSHAIR_SHOWTOP = "KeyMapData_Crosshair_ShowTop";
const char *KEYMAPDATA_CROSSHAIR_SHOWBOTTOM = "KeyMapData_Crosshair_ShowBottom";
const char *KEYMAPDATA_CROSSHAIR_SHOWLEFT = "KeyMapData_Crosshair_ShowLeft";
const char *KEYMAPDATA_CROSSHAIR_SHOWRIGHT = "KeyMapData_Crosshair_ShowRight";
const char *KEYMAPDATA_CROSSHAIR_X_OFFSET = "KeyMapData_Crosshair_X_Offset";
const char *KEYMAPDATA_CROSSHAIR_Y_OFFSET = "KeyMapData_Crosshair_Y_Offset";
const char *KEY2MOUSE_X_SPEED = "Key2Mouse_XSpeed";
const char *KEY2MOUSE_Y_SPEED = "Key2Mouse_YSpeed";
#ifdef VIGEM_CLIENT_SUPPORT
const char *MOUSE2VJOY_X_SENSITIVITY = "Mouse2vJoy_XSensitivity";
const char *MOUSE2VJOY_Y_SENSITIVITY = "Mouse2vJoy_YSensitivity";
const char *MOUSE2VJOY_LOCKCURSOR = "Mouse2vJoy_LockCursor";
const char *MOUSE2VJOY_DIRECTMODE = "Mouse2vJoy_DirectMode";
const char *MOUSE2VJOY_RECENTER_TIMEOUT = "Mouse2vJoy_Recenter_Timeout";
#endif
const char *MAPPINGTABLE_LASTTABINDEX = "MappingTable_LastTabIndex";
const char *MAPPINGTABLE_TABNAMELIST = "MappingTable_TabNameList";
const char *MAPPINGTABLE_TABHOTKEYLIST = "MappingTable_TabHotkeyList";
const char *CLEARALL = "KeyMapData_ClearAll";

const char *PROCESSINFO_FILENAME = "ProcessInfo_FileName";
const char *PROCESSINFO_WINDOWTITLE = "ProcessInfo_WindowTitle";
const char *PROCESSINFO_FILEPATH = "ProcessInfo_FilePath";
const char *PROCESSINFO_FILENAME_CHECKED = "ProcessInfo_FileNameChecked";
const char *PROCESSINFO_WINDOWTITLE_CHECKED = "ProcessInfo_WindowTitleChecked";
const char *PROCESSINFO_DESCRIPTION = "ProcessInfo_Description";

const char *DATAPORT_NUMBER = "DataPortNumber";
const char *GRIP_THRESHOLD_BRAKE = "GripThresholdBrake";
const char *GRIP_THRESHOLD_ACCEL = "GripThresholdAccel";
// const char *DISABLEWINKEY_CHECKED = "DisableWinKeyChecked";
const char *AUTOSTARTMAPPING_CHECKED = "AutoStartMappingChecked";
const char *SENDTOSAMEWINDOWS_CHECKED = "SendToSameWindowsChecked";
const char *ACCEPTVIRTUALGAMEPADINPUT_CHECKED = "AcceptVirtualGamdpadInputChecked";
const char *MAPPINGSTART_KEY = "MappingStartKey";
const char *MAPPINGSTOP_KEY = "MappingStopKey";

const char *SAO_FONTFILENAME = ":/sao_ui.otf";

const char *USBIDS_QRC = ":/usb.ids";

const char *SOUNDFILE_START_QRC = ":/QKeyMapperStart.wav";
const char *SOUNDFILE_START = "QKeyMapperStart.wav";
const char *SOUNDFILE_STOP_QRC = ":/QKeyMapperStop.wav";
const char *SOUNDFILE_STOP = "QKeyMapperStop.wav";

const char *FONTNAME_ENGLISH = "Microsoft YaHei UI";
const char *FONTNAME_CHINESE = "NSimSun";

const QColor STATUS_ON_COLOR = QColor(236, 165, 56);

const char *ORIKEY_COMBOBOX_NAME = "orikeyComboBox";
const char *MAPKEY_COMBOBOX_NAME = "mapkeyComboBox";
const char *SETUPDIALOG_ORIKEY_COMBOBOX_NAME = "SetupDialog_OriginalKeyListComboBox";
const char *SETUPDIALOG_MAPKEY_COMBOBOX_NAME = "SetupDialog_MappingKeyListComboBox";

const char *SETUPDIALOG_MAPKEY_LINEEDIT_NAME        = "SetupDialog_MappingKeyLineEdit";
const char *SETUPDIALOG_MAPKEY_KEYUP_LINEEDIT_NAME  = "SetupDialog_MappingKey_KeyUpLineEdit";

const char *WINDOWSWITCHKEY_LINEEDIT_NAME = "windowswitchkeyLineEdit";
const char *MAPPINGSTARTKEY_LINEEDIT_NAME = "mappingStartKeyLineEdit";
const char *MAPPINGSTOPKEY_LINEEDIT_NAME = "mappingStopKeyLineEdit";

const char *MAPPINGTABLE_TAB_TEXT = "Tab";
const char *ADDTAB_TAB_TEXT = "+";

const char *KEY_NONE_STR = "NONE";
const char *KEY_BLOCKED_STR = "BLOCKED";
const char *GAMEPAD_HOME_STR = "GamepadHome";

const char *MOUSE_MOVE_PREFIX  = "Mouse-Move";
const char *MOUSE_BUTTON_PREFIX  = "Mouse-";
const char *MOUSE_WINDOWPOINT_POSTFIX  = "_WindowPoint";
const char *MOUSE_SCREENPOINT_POSTFIX  = "_ScreenPoint";
const char *MOUSE_L_STR  = "Mouse-L";
const char *MOUSE_R_STR  = "Mouse-R";
const char *MOUSE_M_STR  = "Mouse-M";
const char *MOUSE_X1_STR = "Mouse-X1";
const char *MOUSE_X2_STR = "Mouse-X2";
const char *MOUSE_L_WINDOWPOINT_STR  = "Mouse-L_WindowPoint";
const char *MOUSE_R_WINDOWPOINT_STR  = "Mouse-R_WindowPoint";
const char *MOUSE_M_WINDOWPOINT_STR  = "Mouse-M_WindowPoint";
const char *MOUSE_X1_WINDOWPOINT_STR = "Mouse-X1_WindowPoint";
const char *MOUSE_X2_WINDOWPOINT_STR = "Mouse-X2_WindowPoint";
const char *MOUSE_L_SCREENPOINT_STR  = "Mouse-L_ScreenPoint";
const char *MOUSE_R_SCREENPOINT_STR  = "Mouse-R_ScreenPoint";
const char *MOUSE_M_SCREENPOINT_STR  = "Mouse-M_ScreenPoint";
const char *MOUSE_X1_SCREENPOINT_STR = "Mouse-X1_ScreenPoint";
const char *MOUSE_X2_SCREENPOINT_STR = "Mouse-X2_ScreenPoint";
const char *MOUSE_MOVE_WINDOWPOINT_STR  = "Mouse-Move_WindowPoint";
const char *MOUSE_MOVE_SCREENPOINT_STR  = "Mouse-Move_ScreenPoint";

const int SHOW_MODE_NONE                = 0;
const int SHOW_MODE_SCREEN_MOUSEPOINTS  = 1;
const int SHOW_MODE_WINDOW_MOUSEPOINTS  = 2;
const int SHOW_MODE_CROSSHAIR_NORMAL    = 3;
const int SHOW_MODE_CROSSHAIR_TYPEA     = 4;

const int PICK_WINDOW_POINT_KEY  = VK_LMENU;
const int PICK_SCREEN_POINT_KEY  = VK_LCONTROL;

const char *SENDTIMING_STR_NORMAL               = "NORMAL";
const char *SENDTIMING_STR_KEYDOWN              = "KEYDOWN";
const char *SENDTIMING_STR_KEYUP                = "KEYUP";
const char *SENDTIMING_STR_KEYDOWN_AND_KEYUP    = "KEYDOWN_AND_KEYUP";
const char *SENDTIMING_STR_NORMAL_AND_KEYUP     = "NORMAL_AND_KEYUP";

const char *SHOW_KEY_DEBUGINFO          = "Application";
const char *SHOW_POINTS_IN_WINDOW_KEY   = "F8";
const char *SHOW_POINTS_IN_SCREEN_KEY   = "F9";
const char *SHOW_CAR_ORDINAL_KEY        = "Home";

const char *MOUSE_WHEEL_STR         = "Mouse-Wheel";
const char *MOUSE_WHEEL_UP_STR      = "Mouse-WheelUp";
const char *MOUSE_WHEEL_DOWN_STR    = "Mouse-WheelDown";
const char *MOUSE_WHEEL_LEFT_STR    = "Mouse-WheelLeft";
const char *MOUSE_WHEEL_RIGHT_STR   = "Mouse-WheelRight";

const char *SETTING_DESCRIPTION_FORMAT = "%1【%2】";
const char *ORIKEY_WITHNOTE_FORMAT = "%1【%2】";

const char *SENDTEXT_STR    = "SendText";
const char *KEYSEQUENCEBREAK_STR = "KeySequenceBreak";

const char *JOY_KEY_PREFIX  = "Joy-";
const char *VJOY_KEY_PREFIX  = "vJoy-";

const char *VJOY_MOUSE2LS_STR = "vJoy-Mouse2LS";
const char *VJOY_MOUSE2RS_STR = "vJoy-Mouse2RS";
const char *MOUSE2VJOY_PREFIX = "Mouse2vJoy-";
const char *MOUSE2VJOY_HOLD_KEY_STR = "Mouse2vJoy-Hold";
const char *MOUSE2VJOY_DIRECT_KEY_STR_DEPRECATED = "Mouse2vJoy-Direct";

const char *VJOY_LT_BRAKE_STR = "vJoy-Key11(LT)_BRAKE";
const char *VJOY_RT_BRAKE_STR = "vJoy-Key12(RT)_BRAKE";
const char *VJOY_LT_ACCEL_STR = "vJoy-Key11(LT)_ACCEL";
const char *VJOY_RT_ACCEL_STR = "vJoy-Key12(RT)_ACCEL";

const char *JOY_LS2VJOYLS_STR = "Joy-LS_2vJoyLS";
const char *JOY_RS2VJOYRS_STR = "Joy-RS_2vJoyRS";
const char *JOY_LS2VJOYRS_STR = "Joy-LS_2vJoyRS";
const char *JOY_RS2VJOYLS_STR = "Joy-RS_2vJoyLS";

const char *JOY_LT2VJOYLT_STR = "Joy-Key11(LT)_2vJoyLT";
const char *JOY_RT2VJOYRT_STR = "Joy-Key12(RT)_2vJoyRT";

const char *JOY_LS2MOUSE_STR = "Joy-LS2Mouse";
const char *JOY_RS2MOUSE_STR = "Joy-RS2Mouse";

const char *KEY2MOUSE_PREFIX     = "Key2Mouse-";
const char *KEY2MOUSE_UP_STR     = "Key2Mouse-Up";
const char *KEY2MOUSE_DOWN_STR   = "Key2Mouse-Down";
const char *KEY2MOUSE_LEFT_STR   = "Key2Mouse-Left";
const char *KEY2MOUSE_RIGHT_STR  = "Key2Mouse-Right";

const char *CROSSHAIR_PREFIX        = "Crosshair-";
const char *CROSSHAIR_NORMAL_STR    = "Crosshair-Normal";
const char *CROSSHAIR_TYPEA_STR     = "Crosshair-TypeA";

const char *FUNC_PREFIX          = "Func-";
const char *FUNC_REFRESH         = "Func-Refresh";
const char *FUNC_LOCKSCREEN      = "Func-LockScreen";
const char *FUNC_SHUTDOWN        = "Func-Shutdown";
const char *FUNC_REBOOT          = "Func-Reboot";
const char *FUNC_LOGOFF          = "Func-Logoff";
const char *FUNC_SLEEP           = "Func-Sleep";
const char *FUNC_HIBERNATE       = "Func-Hibernate";

const char *VIRTUAL_GAMEPAD_X360 = "X360";
const char *VIRTUAL_GAMEPAD_DS4  = "DS4";

const char *NO_INPUTDEVICE  = "No InputDevice";

/* Translate const Strings */
/* Common */
const char *UPDATEBUTTON_STR = "Update";
/* QItemSetupDialog */
const char *ITEMSETUPDIALOG_WINDOWTITLE_STR = "Mapping Item Setup";
const char *BURSTCHECKBOX_STR = "Burst";
const char *LOCKCHECKBOX_STR = "Lock";
const char *MAPPINGKEYUNLOCKCHECKBOX_STR = "MappingKeyUnlock";
const char *POSTMAPPINGKEYCHECKBOX_STR = "PostMappingKey";
const char *CHECKCOMBKEYORDERCHECKBOX_STR = "CheckCombKeyOrder";
const char *UNBREAKABLECHECKBOX_STR = "Unbreakable";
const char *PASSTHROUGHCHECKBOX_STR = "PassThrough";
const char *KEYSEQHOLDDOWNCHECKBOX_STR = "KeySeqHoldDown";
const char *REPEATBYKEYCHECKBOX_STR = "RepeatByKey";
const char *REPEATBYTIMESCHECKBOX_STR = "RepeatTimes";
const char *BURSTPRESSLABEL_STR = "BurstPress";
const char *BURSTRELEASE_STR = "BurstRelease";
const char *ORIGINALKEYLABEL_STR = "OriginalKey";
const char *MAPPINGKEYLABEL_STR = "MappingKey";
const char *KEYUPMAPPINGLABEL_STR = "KeyUpMapping";
const char *ITEMNOTELABEL_STR = "Note";
const char *ORIKEYLISTLABEL_STR = "OriginalKeyList";
const char *MAPKEYLISTLABEL_STR = "MappingKeyList";
const char *RECORDKEYSBUTTON_STR = "Record Keys";
const char *CROSSHAIRSETUPBUTTON_STR = "CrosshairSetup";
const char *SENDTIMINGLABEL_STR = "SendTiming";
/* QInputDeviceListWindow */
const char *DEVICELIST_WINDOWTITLE_STR = "Input Device List";
const char *CONFIRMBUTTON_STR = "OK";
const char *CANCELBUTTON_STR = "Cancel";
const char *KEYBOARDLABEL_STR = "Keyboard";
const char *MOUSELABEL_STR = "Mouse";
/* QTableSetupDialog */
const char *TABLESETUPDIALOG_WINDOWTITLE_STR = "Mapping Table Setup";
const char *TABNAMELABEL_STR = "TabName";
const char *TABHOTKEYLABEL_STR = "TabHotkey";
const char *EXPORTTABLEBUTTON_STR = "ExportTable";
const char *IMPORTTABLEBUTTON_STR = "ImportTable";
const char *REMOVETABLEBUTTON_STR = "RemoveTable";

/* Chinese const Strings */
const char *REFRESHBUTTON_CHINESE = "刷新";
const char *KEYMAPBUTTON_START_CHINESE = "开始映射";
const char *KEYMAPBUTTON_STOP_CHINESE = "停止映射";
const char *SAVEMAPLISTBUTTON_CHINESE = "保存设定";
const char *DELETEONEBUTTON_CHINESE = "删除";
const char *CLEARALLBUTTON_CHINESE = "清空";
const char *PROCESSLISTBUTTON_CHINESE = "进程列表";
const char *SHOWNOTESBUTTON_CHINESE = "显示备注";
const char *ADDMAPDATABUTTON_CHINESE = "添加";
const char *NAMECHECKBOX_CHINESE = "进程";
const char *TITLECHECKBOX_CHINESE = "标题";
const char *SETTINGDESCLABEL_CHINESE = "映射设定描述";
const char *GLOBALSETTING_DESC_CHINESE = "全局映射设定";
const char *ORIKEYLABEL_CHINESE = "原始按键";
const char *ORIKEYSEQLABEL_CHINESE = "原始组合键";
const char *MAPKEYLABEL_CHINESE = "映射按键";
// const char *ORIKEYLISTLABEL_CHINESE = "原始按键列表";
// const char *MAPKEYLISTLABEL_CHINESE = "映射按键列表";
// const char *BURSTPRESSLABEL_CHINESE = "连发按下";
// const char *BURSTRELEASE_CHINESE = "连发抬起";
// const char *BURSTPRESS_MSLABEL_CHINESE = "毫秒";
// const char *BURSTRELEASE_MSLABEL_CHINESE = "毫秒";
const char *MILLISECOND_SUFFIX_CHINESE = " 毫秒";
const char *WAITTIME_CHINESE = "延时";
const char *PUSHLEVEL_CHINESE = "轻推值";
const char *SENDTEXTLABEL_CHINESE = "文本";
const char *LONGPRESS_CHINESE = "长按";
const char* DOUBLEPRESS_CHINESE = "双击";
const char *POINT_CHINESE = "坐标";
// const char *WAITTIME_MSLABEL_CHINESE = "毫秒";
const char *MOUSEXSPEEDLABEL_CHINESE = "X轴速度";
const char *MOUSEYSPEEDLABEL_CHINESE = "Y轴速度";
// const char *SETTINGSELECTLABEL_CHINESE = "设定";
const char *REMOVESETTINGBUTTON_CHINESE = "移除";
// const char *DISABLEWINKEYCHECKBOX_CHINESE = "禁用WIN键";
const char *DATAPORTLABEL_CHINESE = "数据端口";
const char *BRAKETHRESHOLDLABEL_CHINESE = "刹车阈值";
const char *ACCELTHRESHOLDLABEL_CHINESE = "油门阈值";
const char *AUTOSTARTMAPPINGCHECKBOX_CHINESE = "自动匹配前台进程";
const char *SENDTOSAMETITLEWINDOWSCHECKBOX_CHINESE = "发送到同名窗口";
const char *ACCEPTVIRTUALGAMEPADINPUTCHECKBOX_CHINESE = "接受虚拟手柄输入";
const char *AUTOSTARTUPCHECKBOX_CHINESE = "开机自动启动";
const char *STARTUPMINIMIZEDCHECKBOX_CHINESE = "启动后自动最小化";
const char *SOUNDEFFECTCHECKBOX_CHINESE = "音效";
const char *NOTIFICATIONLABEL_CHINESE = "提示信息";
const char *LANGUAGELABEL_CHINESE = "语言";
const char *UPDATESITELABEL_CHINESE = "更新网站";
const char *WINDOWSWITCHKEYLABEL_CHINESE = "显示切换键";
const char *MAPPINGSTARTKEYLABEL_CHINESE = "映射开启键";
const char *MAPPINGSTOPKEYLABEL_CHINESE = "映射关闭键";
const char *PROCESSINFOTABLE_COL1_CHINESE = "进程";
const char *PROCESSINFOTABLE_COL2_CHINESE = "进程号";
const char *PROCESSINFOTABLE_COL3_CHINESE = "窗口标题";
const char *KEYMAPDATATABLE_COL1_CHINESE = "原始按键";
const char *KEYMAPDATATABLE_COL2_CHINESE = "映射按键";
const char *KEYMAPDATATABLE_COL3_CHINESE = "连发";
const char *KEYMAPDATATABLE_COL4_CHINESE = "锁定";
#ifdef VIGEM_CLIENT_SUPPORT
const char *VIRTUALGAMEPADGROUPBOX_CHINESE = "虚拟游戏手柄";
const char *VJOYXSENSLABEL_CHINESE = "X轴灵敏度";
const char *VJOYYSENSLABEL_CHINESE = "Y轴灵敏度";
const char *VJOYRECENTERLABEL_CHINESE = "回中延时";
const char *VJOYRECENTERSPINBOX_UNRECENTER_CHINESE = "不回中";
const char *VIGEMBUSSTATUSLABEL_UNAVAILABLE_CHINESE = "ViGEm不可用";
const char *VIGEMBUSSTATUSLABEL_AVAILABLE_CHINESE = "ViGEm可用";
const char *INSTALLVIGEMBUSBUTTON_CHINESE = "安装ViGEm";
const char *UNINSTALLVIGEMBUSBUTTON_CHINESE = "卸载ViGEm";
const char *ENABLEVIRTUALJOYSTICKCHECKBOX_CHINESE = "虚拟手柄";
const char *LOCKCURSORCHECKBOX_CHINESE = "锁定光标";
const char *DIRECTMODECHECKBOX_CHINESE = "直控模式";
#endif
const char *MULTIINPUTGROUPBOX_CHINESE = "多输入设备";
const char *MULTIINPUTENABLECHECKBOX_CHINESE = "多设备";
const char *MULTIINPUTDEVICELISTBUTTON_CHINESE = "设备列表";
const char *FILTERKEYSCHECKBOX_CHINESE = "筛选键";
const char *INSTALLINTERCEPTIONBUTTON_CHINESE = "安装驱动";
const char *UNINSTALLINTERCEPTIONBUTTON_CHINESE = "卸载驱动";
const char *MULTIINPUTSTATUSLABEL_UNAVAILABLE_CHINESE = "多输入不可用";
const char *MULTIINPUTSTATUSLABEL_REBOOTREQUIRED_CHINESE = "需要重启系统";
const char *MULTIINPUTSTATUSLABEL_AVAILABLE_CHINESE = "多输入可用";
const char *KEYBOARDSELECTLABEL_CHINESE = "键盘";
const char *MOUSESELECTLABEL_CHINESE    = "鼠标";
const char *GAMEPADSELECTLABEL_CHINESE  = "游戏手柄";
const char *TRAYMENU_SHOWACTION_CHINESE = "显示";
const char *TRAYMENU_HIDEACTION_CHINESE = "隐藏";
const char *TRAYMENU_QUITACTION_CHINESE = "退出";
// const char *ORIGINALKEYLABEL_CHINESE = "原始按键";
// const char *MAPPINGKEYLABEL_CHINESE = "映射按键";
// const char *KEYUPMAPPINGLABEL_CHINESE = "抬起映射";
// const char *RECORDKEYSBUTTON_CHINESE = "按键录制";
// const char *RECORDSTARTBUTTON_CHINESE = "开始录制";
// const char *RECORDSTOPBUTTON_CHINESE = "停止录制";
// const char *ITEMNOTELABEL_CHINESE = "备注";
// const char *BURSTCHECKBOX_CHINESE = "连发";
// const char *LOCKCHECKBOX_CHINESE = "锁定";
// const char *MAPPINGKEYUNLOCKCHECKBOX_CHINESE = "映射按键解锁";
// const char *POSTMAPPINGKEYCHECKBOX_CHINESE = "Post方式发送";
const char *KEYUPACTIONCHECKBOX_CHINESE = "按键抬起时动作";
// const char *SENDTIMINGLABEL_CHINESE = "发送时机";
// const char *CHECKCOMBKEYORDERCHECKBOX_CHINESE = "检查组合键按下顺序";
// const char *UNBREAKABLECHECKBOX_CHINESE = "不可打断";
// const char *PASSTHROUGHCHECKBOX_CHINESE = "原始按键穿透";
// const char *KEYSEQHOLDDOWNCHECKBOX_CHINESE = "按键序列按下保持";
// const char *REPEATBYKEYCHECKBOX_CHINESE = "按键按下时循环";
// const char *REPEATBYTIMESCHECKBOX_CHINESE = "循环次数";
// const char *UPDATEBUTTON_CHINESE = "更新";
// const char *TABNAMELABEL_CHINESE = "映射表名";
// const char *TABHOTKEYLABEL_CHINESE = "快捷键";
// const char *EXPORTTABLEBUTTON_CHINESE = "导出映射表";
// const char *IMPORTTABLEBUTTON_CHINESE = "导入映射表";
// const char *REMOVETABLEBUTTON_CHINESE = "删除映射表";
const char *SETTINGTAB_GENERAL_CHINESE = "通用";
const char *SETTINGTAB_MAPPING_CHINESE = "映射设定";
const char *SETTINGTAB_VGAMEPAD_CHINESE = "虚拟游戏手柄";
const char *SETTINGTAB_MULTIINPUT_CHINESE = "多输入设备";
const char *SETTINGTAB_FORZA_CHINESE = "极限竞速";
const char *POSITION_NONE_STR_CHINESE           = "不显示";
const char *POSITION_TOP_LEFT_STR_CHINESE       = "顶部左侧";
const char *POSITION_TOP_CENTER_STR_CHINESE     = "顶部居中";
const char *POSITION_TOP_RIGHT_STR_CHINESE      = "顶部右侧";
const char *POSITION_BOTTOM_LEFT_STR_CHINESE    = "底部左侧";
const char *POSITION_BOTTOM_CENTER_STR_CHINESE  = "底部居中";
const char *POSITION_BOTTOM_RIGHT_STR_CHINESE   = "底部右侧";
// const char *SENDTIMING_NORMAL_STR_CHINESE               = "正常";
// const char *SENDTIMING_KEYDOWN_STR_CHINESE              = "按下";
// const char *SENDTIMING_KEYUP_STR_CHINESE                = "抬起";
// const char *SENDTIMING_KEYDOWN_AND_KEYUP_STR_CHINESE    = "按下+抬起";
// const char *SENDTIMING_NORMAL_AND_KEYUP_STR_CHINESE     = "正常+抬起";
const char *CHECKUPDATEBUTTON_CHINESE = "检查更新";

/* English const Strings */
const char *REFRESHBUTTON_ENGLISH = "Refresh";
const char *KEYMAPBUTTON_START_ENGLISH = "MappingStart";
const char *KEYMAPBUTTON_STOP_ENGLISH = "MappingStop";
const char *SAVEMAPLISTBUTTON_ENGLISH = "SaveSetting";
const char *DELETEONEBUTTON_ENGLISH = "Delete";
const char *CLEARALLBUTTON_ENGLISH = "Clear";
const char *PROCESSLISTBUTTON_ENGLISH = "ProcessList";
const char *SHOWNOTESBUTTON_ENGLISH = "ShowNotes";
const char *ADDMAPDATABUTTON_ENGLISH = "ADD";
const char *NAMECHECKBOX_ENGLISH = "Process";
const char *TITLECHECKBOX_ENGLISH = "Title";
const char *SETTINGDESCLABEL_ENGLISH = "Description";
const char *GLOBALSETTING_DESC_ENGLISH = "Global keymapping setting";
const char *ORIKEYLABEL_ENGLISH = "OriKey";
const char *ORIKEYSEQLABEL_ENGLISH = "OriKeySeq";
const char *MAPKEYLABEL_ENGLISH = "MapKey";
// const char *ORIKEYLISTLABEL_ENGLISH = "OriginalKeyList";
// const char *MAPKEYLISTLABEL_ENGLISH = "MappingKeyList";
// const char *BURSTPRESSLABEL_ENGLISH = "BurstPress";
// const char *BURSTRELEASE_ENGLISH = "BurstRelease";
// const char *BURSTPRESS_MSLABEL_ENGLISH = "ms";
// const char *BURSTRELEASE_MSLABEL_ENGLISH = "ms";
const char *MILLISECOND_SUFFIX_ENGLISH = " ms";
const char *WAITTIME_ENGLISH = "Delay";
const char *PUSHLEVEL_ENGLISH = "PushLevel";
const char* SENDTEXTLABEL_ENGLISH = "Text";
const char* LONGPRESS_ENGLISH = "LongPress";
const char* DOUBLEPRESS_ENGLISH = "DoublePress";
const char *POINT_ENGLISH = "Point";
// const char *WAITTIME_MSLABEL_ENGLISH = "ms";
const char *MOUSEXSPEEDLABEL_ENGLISH = "X Speed";
const char *MOUSEYSPEEDLABEL_ENGLISH = "Y Speed";
// const char *SETTINGSELECTLABEL_ENGLISH = "Setting";
const char *REMOVESETTINGBUTTON_ENGLISH = "Remove";
// const char *DISABLEWINKEYCHECKBOX_ENGLISH = "Disable WIN";
const char *DATAPORTLABEL_ENGLISH = "DataPort";
const char *BRAKETHRESHOLDLABEL_ENGLISH = "BrakeValue";
const char *ACCELTHRESHOLDLABEL_ENGLISH = "AccelValue";
const char *AUTOSTARTMAPPINGCHECKBOX_ENGLISH = "Auto Match Foreground";
const char *SENDTOSAMETITLEWINDOWSCHECKBOX_ENGLISH = "Send To Same Windows";
const char *ACCEPTVIRTUALGAMEPADINPUTCHECKBOX_ENGLISH = "Accept Virtual Gamepad Input";
const char *AUTOSTARTUPCHECKBOX_ENGLISH = "Auto Startup";
const char *STARTUPMINIMIZEDCHECKBOX_ENGLISH = "Startup Minimized";
const char *SOUNDEFFECTCHECKBOX_ENGLISH = "Sound Effect";
const char *NOTIFICATIONLABEL_ENGLISH = "Notification";
const char *LANGUAGELABEL_ENGLISH = "Language";
const char *UPDATESITELABEL_ENGLISH = "UpdateSite";
const char *WINDOWSWITCHKEYLABEL_ENGLISH = "WindowKey";
const char *MAPPINGSTARTKEYLABEL_ENGLISH = "MappingStart";
const char *MAPPINGSTOPKEYLABEL_ENGLISH = "MappingStop";
const char *PROCESSINFOTABLE_COL1_ENGLISH = "Process";
const char *PROCESSINFOTABLE_COL2_ENGLISH = "PID";
const char *PROCESSINFOTABLE_COL3_ENGLISH = "Window Title";
const char *KEYMAPDATATABLE_COL1_ENGLISH = "OriginalKey";
const char *KEYMAPDATATABLE_COL2_ENGLISH = "MappingKey";
const char *KEYMAPDATATABLE_COL3_ENGLISH = "Burst";
const char *KEYMAPDATATABLE_COL4_ENGLISH = "Lock";
#ifdef VIGEM_CLIENT_SUPPORT
const char *VIRTUALGAMEPADGROUPBOX_ENGLISH = "Virtual Gamepad";
const char *VJOYXSENSLABEL_ENGLISH = "X Sens";
const char *VJOYYSENSLABEL_ENGLISH = "Y Sens";
const char *VJOYRECENTERLABEL_ENGLISH = "Recenter";
const char *VJOYRECENTERSPINBOX_UNRECENTER_ENGLISH = "Unrecenter";
const char *VIGEMBUSSTATUSLABEL_UNAVAILABLE_ENGLISH = "ViGEmUnavailable";
const char *VIGEMBUSSTATUSLABEL_AVAILABLE_ENGLISH = "ViGEmAvailable";
const char *INSTALLVIGEMBUSBUTTON_ENGLISH = "InstallViGEm";
const char *UNINSTALLVIGEMBUSBUTTON_ENGLISH = "UninstallViGEm";
const char *ENABLEVIRTUALJOYSTICKCHECKBOX_ENGLISH = "VirtualGamepad";
const char *LOCKCURSORCHECKBOX_ENGLISH = "Lock Cursor";
const char *DIRECTMODECHECKBOX_ENGLISH = "Direct Mode";
#endif
const char *MULTIINPUTGROUPBOX_ENGLISH = "Multi-InputDevice";
const char *MULTIINPUTENABLECHECKBOX_ENGLISH = "MultiDevice";
const char *MULTIINPUTDEVICELISTBUTTON_ENGLISH = "DeviceList";
const char *FILTERKEYSCHECKBOX_ENGLISH = "FilterKeys";
const char *INSTALLINTERCEPTIONBUTTON_ENGLISH = "Install Driver";
const char *UNINSTALLINTERCEPTIONBUTTON_ENGLISH = "Uninstall Driver";
const char *MULTIINPUTSTATUSLABEL_UNAVAILABLE_ENGLISH = "Unavailable";
const char *MULTIINPUTSTATUSLABEL_REBOOTREQUIRED_ENGLISH = "RebootRequired";
const char *MULTIINPUTSTATUSLABEL_AVAILABLE_ENGLISH = "Available";
const char *KEYBOARDSELECTLABEL_ENGLISH = "Keyboard";
const char *MOUSESELECTLABEL_ENGLISH    = "Mouse";
const char *GAMEPADSELECTLABEL_ENGLISH  = "Gamepad";
const char *TRAYMENU_SHOWACTION_ENGLISH = "Show";
const char *TRAYMENU_HIDEACTION_ENGLISH = "Hide";
const char *TRAYMENU_QUITACTION_ENGLISH = "Quit";
// const char *ORIGINALKEYLABEL_ENGLISH = "OriginalKey";
// const char *MAPPINGKEYLABEL_ENGLISH = "MappingKey";
// const char *KEYUPMAPPINGLABEL_ENGLISH = "KeyUpMapping";
// const char *ITEMNOTELABEL_ENGLISH = "Note";
// const char *RECORDKEYSBUTTON_ENGLISH = "Record Keys";
// const char *RECORDSTARTBUTTON_ENGLISH = "Start Record";
// const char *RECORDSTOPBUTTON_ENGLISH = "Stop Record";
// const char *BURSTCHECKBOX_ENGLISH = "Burst";
// const char *LOCKCHECKBOX_ENGLISH = "Lock";
// const char *MAPPINGKEYUNLOCKCHECKBOX_ENGLISH = "MappingKeyUnlock";
// const char *POSTMAPPINGKEYCHECKBOX_ENGLISH = "PostMappingKey";
const char *KEYUPACTIONCHECKBOX_ENGLISH = "KeyUpAction";
// const char *SENDTIMINGLABEL_ENGLISH = "SendTiming";
// const char *CHECKCOMBKEYORDERCHECKBOX_ENGLISH = "CheckCombKeyOrder";
// const char *UNBREAKABLECHECKBOX_ENGLISH = "Unbreakable";
// const char *PASSTHROUGHCHECKBOX_ENGLISH = "PassThrough";
// const char *KEYSEQHOLDDOWNCHECKBOX_ENGLISH = "KeySeqHoldDown";
// const char *REPEATBYKEYCHECKBOX_ENGLISH = "RepeatByKey";
// const char *REPEATBYTIMESCHECKBOX_ENGLISH = "RepeatTimes";
// const char *UPDATEBUTTON_ENGLISH = "Update";
// const char *TABNAMELABEL_ENGLISH = "TabName";
// const char *TABHOTKEYLABEL_ENGLISH = "TabHotkey";
// const char *EXPORTTABLEBUTTON_ENGLISH = "ExportTable";
// const char *IMPORTTABLEBUTTON_ENGLISH = "ImportTable";
// const char *REMOVETABLEBUTTON_ENGLISH = "RemoveTable";
const char *SETTINGTAB_GENERAL_ENGLISH = "General";
const char *SETTINGTAB_MAPPING_ENGLISH = "Mapping";
const char *SETTINGTAB_VGAMEPAD_ENGLISH = "V-Gamepad";
const char *SETTINGTAB_MULTIINPUT_ENGLISH = "Multi-Input";
const char *SETTINGTAB_FORZA_ENGLISH = "Forza";
const char *POSITION_NONE_STR_ENGLISH           = "None";
const char *POSITION_TOP_LEFT_STR_ENGLISH       = "T-Left";
const char *POSITION_TOP_CENTER_STR_ENGLISH     = "T-Center";
const char *POSITION_TOP_RIGHT_STR_ENGLISH      = "T-Right";
const char *POSITION_BOTTOM_LEFT_STR_ENGLISH    = "B-Left";
const char *POSITION_BOTTOM_CENTER_STR_ENGLISH  = "B-Center";
const char *POSITION_BOTTOM_RIGHT_STR_ENGLISH   = "B-Right";
// const char *SENDTIMING_NORMAL_STR_ENGLISH               = "Normal";
// const char *SENDTIMING_KEYDOWN_STR_ENGLISH              = "KeyDown";
// const char *SENDTIMING_KEYUP_STR_ENGLISH                = "KeyUp";
// const char *SENDTIMING_KEYDOWN_AND_KEYUP_STR_ENGLISH    = "KeyDown+KeyUp";
// const char *SENDTIMING_NORMAL_AND_KEYUP_STR_ENGLISH     = "Normal+KeyUp";
const char *CHECKUPDATEBUTTON_ENGLISH = "Check Updates";

/* constant values for QItemSetupDialog */
const int ITEMSETUP_EDITING_MAPPINGKEY      = 0;
const int ITEMSETUP_EDITING_KEYUPMAPPINGKEY = 1;

/* constant values for QInputDeviceListWindow */
const int DEVICE_TABLE_NUMBER_COLUMN         = 0;
const int DEVICE_TABLE_DEVICEDESC_COLUMN     = 1;
const int DEVICE_TABLE_HARDWAREID_COLUMN     = 2;
const int DEVICE_TABLE_VENDORID_COLUMN       = 3;
const int DEVICE_TABLE_PRODUCTID_COLUMN      = 4;
const int DEVICE_TABLE_VENDORSTR_COLUMN      = 5;
const int DEVICE_TABLE_PRODUCTSTR_COLUMN     = 6;
const int DEVICE_TABLE_MANUFACTURER_COLUMN   = 7;
const int DEVICE_TABLE_DISABLE_COLUMN        = 8;

const int KEYBOARD_TABLE_COLUMN_COUNT    = 9;
const int MOUSE_TABLE_COLUMN_COUNT       = 9;

// const char *DEVICELIST_WINDOWTITLE_ENGLISH = "Input Device List";
// const char *DEVICELIST_WINDOWTITLE_CHINESE = "输入设备列表";

// const char *ITEMSETUPDIALOG_WINDOWTITLE_ENGLISH = "Mapping Item Setup";
// const char *ITEMSETUPDIALOG_WINDOWTITLE_CHINESE = "映射项设定";

// const char *TABLESETUPDIALOG_WINDOWTITLE_ENGLISH = "Mapping Table Setup";
// const char *TABLESETUPDIALOG_WINDOWTITLE_CHINESE = "映射表设定";

// const char *KEYBOARDLABEL_ENGLISH    = "Keyboard";
// const char *MOUSELABEL_ENGLISH       = "Mouse";

// const char *KEYBOARDLABEL_CHINESE    = "键盘";
// const char *MOUSELABEL_CHINESE       = "鼠标";

// const char *CONFIRMBUTTON_ENGLISH    = "OK";
// const char *CANCELBUTTON_ENGLISH     = "Cancel";

// const char *CONFIRMBUTTON_CHINESE    = "确认";
// const char *CANCELBUTTON_CHINESE     = "取消";

// const char *DEVICE_TABLE_COL0_ENGLISH = "No.";
// const char *DEVICE_TABLE_COL1_ENGLISH = "Device Description";
// const char *DEVICE_TABLE_COL2_ENGLISH = "HardwareID";
// const char *DEVICE_TABLE_COL3_ENGLISH = "VendorID";
// const char *DEVICE_TABLE_COL4_ENGLISH = "ProductID";
// const char *DEVICE_TABLE_COL5_ENGLISH = "Vendor";
// const char *DEVICE_TABLE_COL6_ENGLISH = "Product";
// const char *DEVICE_TABLE_COL7_ENGLISH = "Manufacturer";
// const char *DEVICE_TABLE_COL8_ENGLISH = "Disable";

// const char *DEVICE_TABLE_COL0_CHINESE = "No.";
// const char *DEVICE_TABLE_COL1_CHINESE = "设备描述";
// const char *DEVICE_TABLE_COL2_CHINESE = "硬件ID";
// const char *DEVICE_TABLE_COL3_CHINESE = "VendorID";
// const char *DEVICE_TABLE_COL4_CHINESE = "ProductID";
// const char *DEVICE_TABLE_COL5_CHINESE = "厂商";
// const char *DEVICE_TABLE_COL6_CHINESE = "产品名";
// const char *DEVICE_TABLE_COL7_CHINESE = "制造商";
// const char *DEVICE_TABLE_COL8_CHINESE = "禁用";

ULONG_PTR generateUniqueRandomValue(QSet<ULONG_PTR>& existingValues) {
    ULONG_PTR newValue;
    do {
        newValue = QRandomGenerator::global()->generate() & ~0xF;  // 强制最低4位为0
    } while (existingValues.contains(newValue));
    existingValues.insert(newValue);
    return newValue;
}

void generateVirtualInputRandomValues() {
    QSet<ULONG_PTR> generatedValues;
    generatedValues.insert(INTERCEPTION_EXTRA_INFO_BLOCKED);
    generatedValues.insert(VIRTUAL_UNICODE_CHAR);
    generatedValues.insert(VIRTUAL_CUSTOM_KEYS);
    generatedValues.insert(VIRTUAL_MOUSE_MOVE);
    generatedValues.insert(VIRTUAL_MOUSE_MOVE_BYKEYS);
    VIRTUAL_KEY_SEND = generateUniqueRandomValue(generatedValues);
    VIRTUAL_MOUSE_POINTCLICK = generateUniqueRandomValue(generatedValues);
    VIRTUAL_MOUSE_WHEEL = generateUniqueRandomValue(generatedValues);
    VIRTUAL_KEY_OVERLAY = generateUniqueRandomValue(generatedValues);
    VIRTUAL_RESEND_REALKEY = generateUniqueRandomValue(generatedValues);

    VIRTUAL_KEY_SEND_NORMAL = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_NORMAL;
    VIRTUAL_KEY_SEND_FORCE = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_FORCE;
    VIRTUAL_KEY_SEND_MODIFIERS = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_MODIFIERS;
    VIRTUAL_KEY_SEND_BURST_TIMEOUT = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_BURST_TIMEOUT;
    VIRTUAL_KEY_SEND_BURST_STOP = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_BURST_STOP;
    VIRTUAL_KEY_SEND_KEYSEQ_NORMAL = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL;
    VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_KEYSEQ_HOLDDOWN;
    VIRTUAL_KEY_SEND_KEYSEQ_REPEAT = VIRTUAL_KEY_SEND | SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT;

#ifdef DEBUG_LOGOUT_ON
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_NORMAL: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_NORMAL, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_FORCE: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_FORCE, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_MODIFIERS: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_MODIFIERS, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_BURST_TIMEOUT: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_BURST_TIMEOUT, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_BURST_STOP: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_BURST_STOP, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_KEYSEQ_NORMAL: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_KEYSEQ_NORMAL, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_KEYSEQ_HOLDDOWN, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_SEND_KEYSEQ_REPEAT: 0x%08").arg((qulonglong)VIRTUAL_KEY_SEND_KEYSEQ_REPEAT, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_MOUSE_POINTCLICK: 0x%08").arg((qulonglong)VIRTUAL_MOUSE_POINTCLICK, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_MOUSE_WHEEL: 0x%08").arg((qulonglong)VIRTUAL_MOUSE_WHEEL, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_KEY_OVERLAY: 0x%08").arg((qulonglong)VIRTUAL_KEY_OVERLAY, 0, 16).toUpper();
    qDebug().noquote() << "[generateVirtualInputRandomValues]" << QString("VIRTUAL_RESEND_REALKEY: 0x%08").arg((qulonglong)VIRTUAL_RESEND_REALKEY, 0, 16).toUpper();
#endif
}
