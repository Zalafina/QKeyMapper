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

const int DEFAULT_ICON_WIDTH = 48;
const int DEFAULT_ICON_HEIGHT = 48;

const int MOUSEWHEEL_SCROLL_NONE = 0;
const int MOUSEWHEEL_SCROLL_UP = 1;
const int MOUSEWHEEL_SCROLL_DOWN = 2;

// const int LANGUAGE_CHINESE = 0;
// const int LANGUAGE_ENGLISH = 1;

const int CUSTOMSETTING_INDEX_MAX = 30;

const int TITLESETTING_INDEX_INVALID = -1;
const int TITLESETTING_INDEX_ANYTITLE = 0;
const int TITLESETTING_INDEX_MAX = 9;

const int BURST_TIME_MIN = 1;
const int BURST_TIME_MAX = 5000;

const int BURST_PRESS_TIME_DEFAULT   = 40;
const int BURST_RELEASE_TIME_DEFAULT = 20;

// const int MAPPING_WAITTIME_MIN = 0;
// const int MAPPING_WAITTIME_MAX = 5000;

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

#ifdef VIGEM_CLIENT_SUPPORT
const int RECONNECT_VIGEMCLIENT_WAIT_TIME = 2000;
#endif

const int INSTALL_INTERCEPTION_LOOP_WAIT_TIME = 10;
const int INSTALL_INTERCEPTION_LOOP_WAIT_TIME_MAX = 2000/INSTALL_INTERCEPTION_LOOP_WAIT_TIME;

/* constant values for QKeyMapper_Worker */
const int KEY_INIT = -1;
const int KEY_UP = 0;
const int KEY_DOWN = 1;

const int SENDMODE_NORMAL        = 1;
const int SENDMODE_FORCE_STOP    = 2;

const int MOUSE_WHEEL_UP = 1;
const int MOUSE_WHEEL_DOWN = 2;

const int MOUSE_WHEEL_KEYUP_WAITTIME = 20;

const int SETMOUSEPOSITION_WAITTIME_MAX = 100;

// const int MAPPING_WAITTIME_MIN = 0;
// const int MAPPING_WAITTIME_MAX = 5000;

const WORD XBUTTON_NONE = 0x0000;

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
const int VIRTUAL_JOYSTICK_SENSITIVITY_MAX = 1000;
const int VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT = 12;

const int VIRTUAL_GAMEPAD_NUMBER_MIN = 1;
const int VIRTUAL_GAMEPAD_NUMBER_MAX = 4;

const int MOUSE2VJOY_RESET_TIMEOUT = 200;
const int VJOY_KEYUP_WAITTIME = 20;
const quint8 VK_MOUSE2VJOY_HOLD = 0x3A;
const quint8 VK_MOUSE2VJOY_DIRECT = 0x3B;
#endif

const quint8 VK_BLOCKED         = 0x0F;
const quint8 VK_KEY2MOUSE_UP    = 0x8A;
const quint8 VK_KEY2MOUSE_DOWN  = 0x8B;
const quint8 VK_KEY2MOUSE_LEFT  = 0x8C;
const quint8 VK_KEY2MOUSE_RIGHT = 0x8D;

const ULONG_PTR VIRTUAL_KEYBOARD_PRESS   = 0xACBDACBD;
const ULONG_PTR VIRTUAL_MOUSE2JOY_KEYS   = 0x3A3A3A3A;
const ULONG_PTR VIRTUAL_MOUSE_CLICK      = 0xCEDFCEDF;
const ULONG_PTR VIRTUAL_MOUSE_POINTCLICK = 0xBBDFBBDF;
const ULONG_PTR VIRTUAL_MOUSE_MOVE       = 0xBFBCBFBC;
const ULONG_PTR VIRTUAL_MOUSE_MOVE_BYKEYS= 0x3F3F3F3F;
const ULONG_PTR VIRTUAL_MOUSE_WHEEL      = 0xEBFAEBFA;
const ULONG_PTR VIRTUAL_WIN_PLUS_D       = 0xDBDBDBDB;

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

const char *PROCESS_UNKNOWN = "QKeyMapperUnknown";

const char *DEFAULT_NAME = "ForzaHorizon4.exe";
const char *CONFIG_FILENAME = "keymapdata.ini";
#ifdef SETTINGSFILE_CONVERT
const char *CONFIG_BACKUP_FILENAME = "keymapdata_backup.ini";
#endif

const char *DISPLAYSWITCH_KEY_DEFAULT    = "L-Ctrl+`";
const char *MAPPINGSWITCH_KEY_DEFAULT    = "L-Ctrl+F6";
const char *ORIGINAL_KEYSEQ_DEFAULT      = PREFIX_SHORTCUT;

/* General global settings >>> */
const char *LAST_WINDOWPOSITION = "LastWindowPosition";
const char *LANGUAGE_INDEX = "LanguageIndex";
const char *SETTINGSELECT = "SettingSelect";
const char *AUTO_STARTUP = "AutoStartup";
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
const char *KEYMAPDATA_BURST = "KeyMapData_Burst";
const char *KEYMAPDATA_LOCK = "KeyMapData_Lock";
const char *KEYMAPDATA_BURSTPRESS_TIME = "KeyMapData_BurstPressTime";
const char *KEYMAPDATA_BURSTRELEASE_TIME = "KeyMapData_BurstReleaseTime";
const char *KEY2MOUSE_X_SPEED = "Key2Mouse_XSpeed";
const char *KEY2MOUSE_Y_SPEED = "Key2Mouse_YSpeed";
#ifdef VIGEM_CLIENT_SUPPORT
const char *MOUSE2VJOY_X_SENSITIVITY = "Mouse2vJoy_XSensitivity";
const char *MOUSE2VJOY_Y_SENSITIVITY = "Mouse2vJoy_YSensitivity";
const char *MOUSE2VJOY_LOCKCURSOR = "Mouse2vJoy_LockCursor";
#endif
const char *CLEARALL = "KeyMapData_ClearAll";

const char *PROCESSINFO_FILENAME = "ProcessInfo_FileName";
const char *PROCESSINFO_WINDOWTITLE = "ProcessInfo_WindowTitle";
const char *PROCESSINFO_FILEPATH = "ProcessInfo_FilePath";
const char *PROCESSINFO_FILENAME_CHECKED = "ProcessInfo_FileNameChecked";
const char *PROCESSINFO_WINDOWTITLE_CHECKED = "ProcessInfo_WindowTitleChecked";

const char *DATAPORT_NUMBER = "DataPortNumber";
const char *GRIP_THRESHOLD_BRAKE = "GripThresholdBrake";
const char *GRIP_THRESHOLD_ACCEL = "GripThresholdAccel";
// const char *DISABLEWINKEY_CHECKED = "DisableWinKeyChecked";
const char *AUTOSTARTMAPPING_CHECKED = "AutoStartMappingChecked";
const char *MAPPINGSWITCH_KEYSEQ = "MappingSwitch_KeySequence";

const char *SAO_FONTFILENAME = ":/sao_ui.otf";

const char *USBIDS_QRC = ":/usb.ids";

const char *SOUNDFILE_START_QRC = ":/QKeyMapperStart.wav";
const char *SOUNDFILE_START = "QKeyMapperStart.wav";
const char *SOUNDFILE_STOP_QRC = ":/QKeyMapperStop.wav";
const char *SOUNDFILE_STOP = "QKeyMapperStop.wav";

const char *FONTNAME_ENGLISH = "Microsoft YaHei UI";
const char *FONTNAME_CHINESE = "NSimSun";

const char *ORIKEY_COMBOBOX_NAME = "orikeyComboBox";
const char *MAPKEY_COMBOBOX_NAME = "mapkeyComboBox";

const char *WINDOWSWITCHKEY_LINEEDIT_NAME = "windowswitchkeyLineEdit";
const char *MAPPINGSWITCHKEY_LINEEDIT_NAME = "mappingswitchkeyLineEdit";

const char *KEY_BLOCKED_STR = "BLOCKED";

const char *MOUSE_BUTTON_PREFIX  = "Mouse-";
const char *MOUSE_POINT_POSTFIX  = "_Point";
const char *MOUSE_L_STR  = "Mouse-L";
const char *MOUSE_R_STR  = "Mouse-R";
const char *MOUSE_M_STR  = "Mouse-M";
const char *MOUSE_X1_STR = "Mouse-X1";
const char *MOUSE_X2_STR = "Mouse-X2";
const char *MOUSE_L_POINT_STR  = "Mouse-L_Point";
const char *MOUSE_R_POINT_STR  = "Mouse-R_Point";
const char *MOUSE_M_POINT_STR  = "Mouse-M_Point";
const char *MOUSE_X1_POINT_STR = "Mouse-X1_Point";
const char *MOUSE_X2_POINT_STR = "Mouse-X2_Point";

const char *SHOW_MOUSE_POINTS_KEY    = "F9";
const char *SHOW_CAR_ORDINAL_KEY     = "F10";

const char *MOUSE_WHEEL_UP_STR   = "Mouse-WheelUp";
const char *MOUSE_WHEEL_DOWN_STR = "Mouse-WheelDown";

const char *JOY_KEY_PREFIX  = "Joy-";
const char *VJOY_KEY_PREFIX  = "vJoy-";

const char *VJOY_MOUSE2LS_STR = "vJoy-Mouse2LS";
const char *VJOY_MOUSE2RS_STR = "vJoy-Mouse2RS";
const char *MOUSE2VJOY_PREFIX = "Mouse2vJoy-";
const char *MOUSE2VJOY_HOLD_KEY_STR = "Mouse2vJoy-Hold";
const char *MOUSE2VJOY_DIRECT_KEY_STR = "Mouse2vJoy-Direct";

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

/* Chinese const Strings */
const char *REFRESHBUTTON_CHINESE = "刷新";
const char *KEYMAPBUTTON_START_CHINESE = "开始映射";
const char *KEYMAPBUTTON_STOP_CHINESE = "停止映射";
const char *SAVEMAPLISTBUTTON_CHINESE = "保存设定";
const char *DELETEONEBUTTON_CHINESE = "删除";
const char *CLEARALLBUTTON_CHINESE = "清空";
const char *ADDMAPDATABUTTON_CHINESE = "添加";
const char *NAMECHECKBOX_CHINESE = "进程";
const char *TITLECHECKBOX_CHINESE = "标题";
const char *ORIKEYLABEL_CHINESE = "原始按键";
const char *ORIKEYSEQLABEL_CHINESE = "原始组合键";
const char *MAPKEYLABEL_CHINESE = "映射按键";
const char *BURSTPRESSLABEL_CHINESE = "连发按下";
const char *BURSTRELEASE_CHINESE = "连发抬起";
// const char *BURSTPRESS_MSLABEL_CHINESE = "毫秒";
// const char *BURSTRELEASE_MSLABEL_CHINESE = "毫秒";
const char *WAITTIME_CHINESE = "延时";
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
const char *AUTOSTARTMAPPINGCHECKBOX_CHINESE = "自动映射并最小化";
const char *AUTOSTARTUPCHECKBOX_CHINESE = "开机自动启动";
const char *SOUNDEFFECTCHECKBOX_CHINESE = "音效";
const char *WINDOWSWITCHKEYLABEL_CHINESE = "显示切换键";
const char *MAPPINGSWITCHKEYLABEL_CHINESE = "映射开关键";
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
const char *VIGEMBUSSTATUSLABEL_UNAVAILABLE_CHINESE = "ViGEm不可用";
const char *VIGEMBUSSTATUSLABEL_AVAILABLE_CHINESE = "ViGEm可用";
const char *INSTALLVIGEMBUSBUTTON_CHINESE = "安装ViGEm";
const char *UNINSTALLVIGEMBUSBUTTON_CHINESE = "卸载ViGEm";
const char *ENABLEVIRTUALJOYSTICKCHECKBOX_CHINESE = "虚拟手柄";
const char *LOCKCURSORCHECKBOX_CHINESE = "锁定光标";
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

/* English const Strings */
const char *REFRESHBUTTON_ENGLISH = "Refresh";
const char *KEYMAPBUTTON_START_ENGLISH = "MappingStart";
const char *KEYMAPBUTTON_STOP_ENGLISH = "MappingStop";
const char *SAVEMAPLISTBUTTON_ENGLISH = "SaveSetting";
const char *DELETEONEBUTTON_ENGLISH = "Delete";
const char *CLEARALLBUTTON_ENGLISH = "Clear";
const char *ADDMAPDATABUTTON_ENGLISH = "ADD";
const char *NAMECHECKBOX_ENGLISH = "Process";
const char *TITLECHECKBOX_ENGLISH = "Title";
const char *ORIKEYLABEL_ENGLISH = "OriKey";
const char *ORIKEYSEQLABEL_ENGLISH = "OriKeySeq";
const char *MAPKEYLABEL_ENGLISH = "MapKey";
const char *BURSTPRESSLABEL_ENGLISH = "BurstPress";
const char *BURSTRELEASE_ENGLISH = "BurstRelease";
// const char *BURSTPRESS_MSLABEL_ENGLISH = "ms";
// const char *BURSTRELEASE_MSLABEL_ENGLISH = "ms";
const char *WAITTIME_ENGLISH = "Delay";
const char *POINT_ENGLISH = "Point";
// const char *WAITTIME_MSLABEL_ENGLISH = "ms";
const char *MOUSEXSPEEDLABEL_ENGLISH = "X Speed";
const char *MOUSEYSPEEDLABEL_ENGLISH = "Y Speed";
// const char *SETTINGSELECTLABEL_ENGLISH = "Setting";
const char *REMOVESETTINGBUTTON_ENGLISH = "Remove";
// const char *DISABLEWINKEYCHECKBOX_ENGLISH = "Disable WIN";
const char *DATAPORTLABEL_ENGLISH = "DataPort";
const char *BRAKETHRESHOLDLABEL_ENGLISH = "BrakeThreshold";
const char *ACCELTHRESHOLDLABEL_ENGLISH = "AccelThreshold";
const char *AUTOSTARTMAPPINGCHECKBOX_ENGLISH = "AutoStartMinimize";
const char *AUTOSTARTUPCHECKBOX_ENGLISH = "Auto Startup";
const char *SOUNDEFFECTCHECKBOX_ENGLISH = "Sound Effect";
const char *WINDOWSWITCHKEYLABEL_ENGLISH = "WindowKey";
const char *MAPPINGSWITCHKEYLABEL_ENGLISH = "MappingKey";
const char *PROCESSINFOTABLE_COL1_ENGLISH = "Process";
const char *PROCESSINFOTABLE_COL2_ENGLISH = "PID";
const char *PROCESSINFOTABLE_COL3_ENGLISH = "Window Title";
const char *KEYMAPDATATABLE_COL1_ENGLISH = "OriginalKey";
const char *KEYMAPDATATABLE_COL2_ENGLISH = "MappingKey";
const char *KEYMAPDATATABLE_COL3_ENGLISH = "Burst";
const char *KEYMAPDATATABLE_COL4_ENGLISH = "Lock";
#ifdef VIGEM_CLIENT_SUPPORT
const char *VIRTUALGAMEPADGROUPBOX_ENGLISH = "Virtual Gamepad";
const char *VJOYXSENSLABEL_ENGLISH = "X Sensitivity";
const char *VJOYYSENSLABEL_ENGLISH = "Y Sensitivity";
const char *VIGEMBUSSTATUSLABEL_UNAVAILABLE_ENGLISH = "ViGEmUnavailable";
const char *VIGEMBUSSTATUSLABEL_AVAILABLE_ENGLISH = "ViGEmAvailable";
const char *INSTALLVIGEMBUSBUTTON_ENGLISH = "InstallViGEm";
const char *UNINSTALLVIGEMBUSBUTTON_ENGLISH = "UninstallViGEm";
const char *ENABLEVIRTUALJOYSTICKCHECKBOX_ENGLISH = "VirtualGamepad";
const char *LOCKCURSORCHECKBOX_ENGLISH = "Lock Cursor";
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

const char *DEVICELIST_WINDOWTITLE_ENGLISH = "Input Device List";

const char *DEVICELIST_WINDOWTITLE_CHINESE = "输入设备列表";

const char *KEYBOARDLABEL_ENGLISH    = "Keyboard";
const char *MOUSELABEL_ENGLISH       = "Mouse";

const char *KEYBOARDLABEL_CHINESE    = "键盘";
const char *MOUSELABEL_CHINESE       = "鼠标";

const char *CONFIRMBUTTON_ENGLISH    = "OK";
const char *CANCELBUTTON_ENGLISH     = "Cancel";

const char *CONFIRMBUTTON_CHINESE    = "确认";
const char *CANCELBUTTON_CHINESE     = "取消";

const char *DEVICE_TABLE_COL0_ENGLISH = "No.";
const char *DEVICE_TABLE_COL1_ENGLISH = "Device Description";
const char *DEVICE_TABLE_COL2_ENGLISH = "HardwareID";
const char *DEVICE_TABLE_COL3_ENGLISH = "VendorID";
const char *DEVICE_TABLE_COL4_ENGLISH = "ProductID";
const char *DEVICE_TABLE_COL5_ENGLISH = "Vendor";
const char *DEVICE_TABLE_COL6_ENGLISH = "Product";
const char *DEVICE_TABLE_COL7_ENGLISH = "Manufacturer";
const char *DEVICE_TABLE_COL8_ENGLISH = "Disable";

const char *DEVICE_TABLE_COL0_CHINESE = "No.";
const char *DEVICE_TABLE_COL1_CHINESE = "设备描述";
const char *DEVICE_TABLE_COL2_CHINESE = "硬件ID";
const char *DEVICE_TABLE_COL3_CHINESE = "VendorID";
const char *DEVICE_TABLE_COL4_CHINESE = "ProductID";
const char *DEVICE_TABLE_COL5_CHINESE = "厂商";
const char *DEVICE_TABLE_COL6_CHINESE = "产品名";
const char *DEVICE_TABLE_COL7_CHINESE = "制造商";
const char *DEVICE_TABLE_COL8_CHINESE = "禁用";
