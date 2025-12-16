#pragma once

#include <windows.h>
#include <devpropdef.h>
#include <QPoint>
#include <QSize>
#include <QColor>
#include <QListWidgetItem>

namespace QKeyMapperConstants {

    /* constant values for QKeyMapper */
#ifdef CYCLECHECKTIMER_ENABLED
    inline constexpr const unsigned int CYCLE_CHECK_TIMEOUT = 300U;
#endif
#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
    inline constexpr const unsigned int CYCLE_CHECK_LOOPCOUNT_MAX = 100000U;
    inline constexpr const unsigned int CYCLE_CHECK_LOOPCOUNT_RESET = 500U;
    inline constexpr const unsigned int GLOBAL_MAPPING_START_WAIT = 2100U / CYCLE_CHECK_TIMEOUT;
#else
    inline constexpr unsigned int CHECK_GLOBALSETTING_SWITCH_TIMEOUT = 2000U;
#endif

    inline constexpr unsigned int CYCLE_REFRESH_PROCESSINFOTABLE_TIMEOUT = 3000U;

    inline constexpr unsigned int SWITCH_BACKTO_LAST_MATCHED_SETTING_TIMEOUT = 20000U;

    inline constexpr int POPUP_MESSAGE_DISPLAY_TIME_DEFAULT = 3000;

    inline constexpr int VIRTUALGAMEPADTYPECOMBOBOX_X = 610;

    inline constexpr int PROCESSINFO_TABLE_COLUMN_COUNT = 4;
    inline constexpr int KEYMAPPINGDATA_TABLE_COLUMN_COUNT = 5;
    inline constexpr int MACROLISTDATA_TABLE_COLUMN_COUNT = 4;

    inline constexpr int INITIAL_WINDOW_POSITION = -10000;

    inline constexpr int PROCESS_NAME_COLUMN = 0;
    inline constexpr int PROCESS_PID_COLUMN = 1;
    inline constexpr int PROCESS_TITLE_COLUMN = 2;
    inline constexpr int PROCESS_CLASS_COLUMN = 3;

    inline constexpr int PROCESS_NAME_COLUMN_WIDTH_MAX = 120;
    inline constexpr int CLASS_NAME_COLUMN_WIDTH_MAX = 80;

    inline constexpr int ORIGINAL_KEY_COLUMN = 0;
    inline constexpr int MAPPING_KEY_COLUMN = 1;
    inline constexpr int BURST_MODE_COLUMN = 2;
    inline constexpr int LOCK_COLUMN = 3;
    inline constexpr int CATEGORY_COLUMN = 4;

    inline constexpr int MACRO_NAME_COLUMN = 0;
    inline constexpr int MACRO_CONTENT_COLUMN = 1;
    inline constexpr int MACRO_CATEGORY_COLUMN = 2;
    inline constexpr int MACRO_NOTE_COLUMN = 3;

    inline constexpr int CATEGORY_FILTER_ALL_INDEX = 0;

    inline constexpr int MAPPING_UPDATE_ORDER_DEFAULT                   = 0;
    inline constexpr int MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST        = 1;
    inline constexpr int MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST         = 2;
    inline constexpr int MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST   = 3;

    inline constexpr int KEYRECORD_EDITMODE_CAPTURE = 0;
    inline constexpr int KEYRECORD_EDITMODE_MANUALEDIT = 1;

    inline constexpr int KEY_TYPE_COMMON       = 0;
    inline constexpr int KEY_TYPE_KEYBOARD     = 1;
    inline constexpr int KEY_TYPE_MOUSE        = 2;
    inline constexpr int KEY_TYPE_GAMEPAD      = 3;
    inline constexpr int KEY_TYPE_FUNCTION     = 4;

    inline constexpr int BLOCK_INPUTDEVICE_KEYBOARD         = 0;
    inline constexpr int BLOCK_INPUTDEVICE_MOUSE            = 1;

    inline constexpr int KEYMAPPINGTABWIDGET_NARROW_LEFT   = 526;
    inline constexpr int KEYMAPPINGTABWIDGET_NARROW_WIDTH  = 458;
    inline constexpr int KEYMAPPINGTABWIDGET_WIDE_LEFT     = 26;
    inline constexpr int KEYMAPPINGTABWIDGET_WIDE_WIDTH    = 958;

    inline constexpr int KEYMAPPINGDATATABLE_NARROW_LEFT   = 0;
    inline constexpr int KEYMAPPINGDATATABLE_NARROW_WIDTH  = 454;
    inline constexpr int KEYMAPPINGDATATABLE_WIDE_LEFT     = 0;
    inline constexpr int KEYMAPPINGDATATABLE_WIDE_WIDTH    = 954;

    inline constexpr int KEYMAPPINGDATATABLE_TOP       = 0;
    inline constexpr int KEYMAPPINGDATATABLE_HEIGHT    = 324;

    inline constexpr int ITEM_ICON_SIZE = 16;
    inline constexpr int BEST_ICON_SIZE = 256;
    inline constexpr int DEFAULT_ICON_WIDTH = 48;
    inline constexpr int DEFAULT_ICON_HEIGHT = 48;

    inline constexpr int PREFER_ICON_LARGE      = 0;
    inline constexpr int PREFER_ICON_SMALL      = 1;
    inline constexpr int PREFER_ICON_DEFAULT    = 2;

    inline constexpr int COMBOBOX_BLANK_ICON_WIDTH = 16;
    inline constexpr int COMBOBOX_BLANK_ICON_HEIGHT = 16;

    inline constexpr int MOUSEWHEEL_SCROLL_NONE = 0;
    inline constexpr int MOUSEWHEEL_SCROLL_UP = 1;
    inline constexpr int MOUSEWHEEL_SCROLL_DOWN = 2;

    inline constexpr int SYSTRAY_MENU_ITEM_PRESSED_NONE        = 0;
    inline constexpr int SYSTRAY_MENU_ITEM_PRESSED_SHOWHIDE    = 1;
    inline constexpr int SYSTRAY_MENU_ITEM_PRESSED_QUIT        = 2;

    inline constexpr Qt::Key KEY_REFRESH           = Qt::Key_F5;
    inline constexpr Qt::Key KEY_RECORD_START      = Qt::Key_F11;
    inline constexpr Qt::Key KEY_RECORD_STOP       = Qt::Key_F12;

    inline constexpr int LANGUAGE_CHINESE = 0;
    inline constexpr int LANGUAGE_ENGLISH = 1;
    inline constexpr int LANGUAGE_JAPANESE = 2;

    inline constexpr int UI_THEME_SYSTEMDEFAULT = 0;
    inline constexpr int UI_THEME_LIGHT         = 1;
    inline constexpr int UI_THEME_DARK          = 2;

    inline constexpr int UI_PALETTE_INITIAL         = -1;
    inline constexpr int UI_PALETTE_SYSTEMDEFAULT   = 0;
    inline constexpr int UI_PALETTE_CUSTOMLIGHT     = 1;
    inline constexpr int UI_PALETTE_CUSTOMDARK      = 2;

    inline constexpr int UPDATE_SITE_GITHUB = 0;
    inline constexpr int UPDATE_SITE_GITEE = 1;

    inline constexpr int KEYPRESS_TYPE_LONGPRESS   = 0;
    inline constexpr int KEYPRESS_TYPE_DOUBLEPRESS = 1;

    inline constexpr int EMPTYSETTING_INDEX  = 0;
    inline constexpr int GLOBALSETTING_INDEX = 1;

#ifdef ENABLE_SYSTEMFILTERKEYS_DEFAULT
    inline constexpr bool ENABLE_SYSTEM_FILTERKEY_CHECKED_DEFAULT  = true;
#else
    inline constexpr bool ENABLE_SYSTEM_FILTERKEY_CHECKED_DEFAULT  = false;
#endif

    // Define match type enumeration
    enum class WindowInfoMatchType : int {
        Ignore     = 0,
        Equals     = 1,
        Contains   = 2,
        StartsWith = 3,
        EndsWith   = 4,
        RegexMatch = 5
    };

    // Define helper constants (default value and range)
    inline constexpr WindowInfoMatchType WINDOWINFO_MATCH_TYPE_DEFAULT = WindowInfoMatchType::Contains;
    inline constexpr WindowInfoMatchType WINDOWINFO_MATCH_TYPE_MIN     = WindowInfoMatchType::Ignore;
    inline constexpr WindowInfoMatchType WINDOWINFO_MATCH_TYPE_MAX     = WindowInfoMatchType::RegexMatch;

    inline constexpr int WINDOWINFO_MATCH_INDEX_IGNORE     = 0;
    inline constexpr int WINDOWINFO_MATCH_INDEX_EQUALS     = 1;
    inline constexpr int WINDOWINFO_MATCH_INDEX_CONTAINS   = 2;
    inline constexpr int WINDOWINFO_MATCH_INDEX_STARTSWITH = 3;
    inline constexpr int WINDOWINFO_MATCH_INDEX_ENDSWITH   = 4;
    inline constexpr int WINDOWINFO_MATCH_INDEX_REGEXMATCH = 5;
    inline constexpr int WINDOWINFO_MATCH_INDEX_DEFAULT    = WINDOWINFO_MATCH_INDEX_CONTAINS;
    inline constexpr int WINDOWINFO_MATCH_INDEX_MIN        = WINDOWINFO_MATCH_INDEX_IGNORE;
    inline constexpr int WINDOWINFO_MATCH_INDEX_MAX        = WINDOWINFO_MATCH_INDEX_REGEXMATCH;

    inline constexpr int BURST_TIME_MIN = 1;
    inline constexpr int BURST_TIME_MAX = 99999;

    inline constexpr int BURST_PRESS_TIME_DEFAULT   = 40;
    inline constexpr int BURST_RELEASE_TIME_DEFAULT = 20;

    inline constexpr int MAPPING_WAITTIME_MIN = 0;
    inline constexpr int MAPPING_WAITTIME_MAX = 999999;

    inline constexpr int PRESSTIME_MIN = 0;
    inline constexpr int PRESSTIME_MAX = 9999;

    inline constexpr int REPEAT_MODE_NONE      = 0;
    inline constexpr int REPEAT_MODE_BYKEY     = 1;
    inline constexpr int REPEAT_MODE_BYTIMES   = 2;

    inline constexpr int REPEAT_TIMES_MIN = 1;
    inline constexpr int REPEAT_TIMES_MAX = 99999999;
    inline constexpr int REPEAT_TIMES_DEFAULT = 1;

    inline constexpr int COLORPICKER_BUTTON_WIDTH_TABFONTCOLOR = 101;
    inline constexpr int COLORPICKER_BUTTON_WIDTH_TABBGCOLOR = 101;
    inline constexpr int COLORPICKER_BUTTON_WIDTH_NOTIFICATION_FONTCOLOR = 61;
    inline constexpr int COLORPICKER_BUTTON_WIDTH_NOTIFICATION_BGCOLOR = 51;
    inline constexpr int COLORPICKER_BUTTON_WIDTH_FLOATINGWINDOW_BGCOLOR = 51;

    inline constexpr const char NOTIFICATION_COLOR_GLOBAL_DEFAULT_STR[] = "#26de81";
    inline constexpr const char NOTIFICATION_COLOR_NORMAL_DEFAULT_STR[] = "#d6a2e8";
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // QColor supports constexpr in Qt6
    inline constexpr QColor NOTIFICATION_COLOR_GLOBAL_DEFAULT = QColor(38, 222, 129);
    inline constexpr QColor NOTIFICATION_COLOR_NORMAL_DEFAULT = QColor(214, 162, 232);
    inline constexpr QColor NOTIFICATION_BACKGROUND_COLOR_DEFAULT = QColor(0,0,0,120);
#else
    // Fallback for Qt5: QColor is not constexpr
    inline const QColor NOTIFICATION_COLOR_GLOBAL_DEFAULT = QColor(38, 222, 129);
    inline const QColor NOTIFICATION_COLOR_NORMAL_DEFAULT = QColor(214, 162, 232);
    inline const QColor NOTIFICATION_BACKGROUND_COLOR_DEFAULT = QColor(0,0,0,120);
#endif

    inline constexpr int NOTIFICATION_FONT_SIZE_MIN = 1;
    inline constexpr int NOTIFICATION_FONT_SIZE_MAX = 72;
    inline constexpr int NOTIFICATION_FONT_SIZE_DEFAULT = 16;

    inline constexpr int NOTIFICATION_FONT_WEIGHT_LIGHT    = 0;
    inline constexpr int NOTIFICATION_FONT_WEIGHT_NORMAL   = 1;
    inline constexpr int NOTIFICATION_FONT_WEIGHT_BOLD     = 2;
    inline constexpr int NOTIFICATION_FONT_WEIGHT_DEFAULT  = NOTIFICATION_FONT_WEIGHT_BOLD;
    inline constexpr int NOTIFICATION_FONT_WEIGHT_MIN      = NOTIFICATION_FONT_WEIGHT_LIGHT;
    inline constexpr int NOTIFICATION_FONT_WEIGHT_MAX      = NOTIFICATION_FONT_WEIGHT_BOLD;

    inline constexpr bool NOTIFICATION_FONT_ITALIC_DEFAULT = false;
    inline constexpr int NOTIFICATION_DISPLAY_DURATION_DEFAULT = 3000;
    inline constexpr int NOTIFICATION_FADEIN_DURATION_DEFAULT = 0;
    inline constexpr int NOTIFICATION_FADEOUT_DURATION_DEFAULT = 500;
    inline constexpr double NOTIFICATION_OPACITY_DEFAULT = 1.0;
    inline constexpr int NOTIFICATION_PADDING_DEFAULT = 15;
    inline constexpr int NOTIFICATION_BORDER_RADIUS_DEFAULT = 5;
    inline constexpr int NOTIFICATION_X_OFFSET_DEFAULT = 0;
    inline constexpr int NOTIFICATION_Y_OFFSET_DEFAULT = 0;

    inline constexpr int NOTIFICATION_OPACITY_DECIMALS = 2;
    inline constexpr double NOTIFICATION_OPACITY_SINGLESTEP = 0.01;
    inline constexpr int NOTIFICATION_DURATION_MIN = 0;
    inline constexpr int NOTIFICATION_DURATION_MAX = 99999;
    inline constexpr double NOTIFICATION_OPACITY_MIN = 0.0;
    inline constexpr double NOTIFICATION_OPACITY_MAX = 1.0;
    inline constexpr int NOTIFICATION_PADDING_MIN = 0;
    inline constexpr int NOTIFICATION_PADDING_MAX = 1000;
    inline constexpr int NOTIFICATION_BORDER_RADIUS_MIN = 0;
    inline constexpr int NOTIFICATION_BORDER_RADIUS_MAX = 1000;
    inline constexpr int NOTIFICATION_OFFSET_MIN = -5000;
    inline constexpr int NOTIFICATION_OFFSET_MAX = 5000;

    inline constexpr const char QT_TOOLTIP_WINDOW_CLASS[] = "QWindowToolTipDropShadowSaveBits";
    inline constexpr const char QT_COMBOBOX_ITEMS_WINDOW_CLASS[] = "QWindowPopupDropShadowSaveBits";
    inline constexpr const char QT_TOOL_WINDOW_CLASS[] = "QWindowToolSaveBits";
    inline constexpr const char SYSTEM_SHADOW_WINDOW_CLASS[] = "SysShadow";

    inline constexpr const char CUSTOM_TRAYICONS_DIR[] = "custom_trayicons";
    inline constexpr const char ICON_FILE_SUFFIX[] = ".ico";
    inline constexpr const char TRAYICON_IDLE_DEFAULT_FILE[] = ":/Blue1.ico";
    inline constexpr const char TRAYICON_MONITORING_DEFAULT_FILE[] = ":/Red.ico";
    inline constexpr const char TRAYICON_GLOBAL_DEFAULT_FILE[] = ":/Green1.ico";
    inline constexpr const char TRAYICON_MATCHED_DEFAULT_FILE[] = ":/Purple.ico";

    inline constexpr Qt::CheckState TAB_HIDE_NOTIFICATION_DEFAULT = Qt::Unchecked;

    inline constexpr int TAB_CUSTOMIMAGE_WIDTH_DEFAULT = 48;
    inline constexpr int TAB_CUSTOMIMAGE_HEIGHT_DEFAULT = 48;
    inline constexpr int TAB_CUSTOMIMAGE_SHOW_NONE = 0;
    inline constexpr int TAB_CUSTOMIMAGE_SHOW_LEFT = 1;
    inline constexpr int TAB_CUSTOMIMAGE_SHOW_RIGHT = 2;
    inline constexpr int TAB_CUSTOMIMAGE_POSITION_MIN = TAB_CUSTOMIMAGE_SHOW_NONE;
    inline constexpr int TAB_CUSTOMIMAGE_POSITION_MAX = TAB_CUSTOMIMAGE_SHOW_RIGHT;
    inline constexpr int TAB_CUSTOMIMAGE_POSITION_DEFAULT = TAB_CUSTOMIMAGE_SHOW_NONE;
    inline constexpr int TAB_CUSTOMIMAGE_PADDING_MIN = 0;
    inline constexpr int TAB_CUSTOMIMAGE_PADDING_MAX = 1000;
    inline constexpr int TAB_CUSTOMIMAGE_PADDING_DEFAULT = 0;
    inline constexpr bool TAB_CUSTOMIMAGE_SHOW_AS_TRAYICON_DEFAULT = false;
    inline constexpr bool TAB_CUSTOMIMAGE_SHOW_AS_FLOATINGWINDOW_DEFAULT = false;

    inline constexpr const char ICON_SIZE_SEPARATORT[] = "×";

    inline constexpr int TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT = 0;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // QColor supports constexpr in Qt6
    inline constexpr QColor FLOATINGWINDOW_BACKGROUND_COLOR_DEFAULT = QColor(0,0,0,120);
#else
    // Fallback for Qt5: QColor is not constexpr
    inline const QColor FLOATINGWINDOW_BACKGROUND_COLOR_DEFAULT = QColor(0,0,0,120);
#endif

    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPLEFT        = 0;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPRIGHT       = 1;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPCENTER      = 2;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMLEFT     = 3;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMRIGHT    = 4;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMCENTER   = 5;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPLEFT        = 6;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPRIGHT       = 7;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPCENTER      = 8;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMLEFT     = 9;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMRIGHT    = 10;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMCENTER   = 11;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_MIN = FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPLEFT;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_MAX = FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMCENTER;
    inline constexpr int FLOATINGWINDOW_REFERENCEPOINT_DEFAULT = FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPLEFT;

    inline constexpr int FLOATINGWINDOW_POSITION_MIN_X = -50000;
    inline constexpr int FLOATINGWINDOW_POSITION_MIN_Y = -50000;
    inline constexpr int FLOATINGWINDOW_POSITION_MAX_X = 50000;
    inline constexpr int FLOATINGWINDOW_POSITION_MAX_Y = 50000;
    inline constexpr QPoint FLOATINGWINDOW_POSITION_DEFAULT = QPoint(50, 50);

    inline constexpr int FLOATINGWINDOW_SIZE_MIN = 16;
    inline constexpr int FLOATINGWINDOW_SIZE_MAX = 640;
    inline constexpr QSize FLOATINGWINDOW_SIZE_DEFAULT = QSize(64, 64);

    inline constexpr int FLOATINGWINDOW_RADIUS_MIN = 0;
    inline constexpr int FLOATINGWINDOW_RADIUS_MAX = FLOATINGWINDOW_SIZE_MAX / 2;
    inline constexpr int FLOATINGWINDOW_RADIUS_DEFAULT = 0;

    inline constexpr int FLOATINGWINDOW_OPACITY_DECIMALS = 2;
    inline constexpr double FLOATINGWINDOW_OPACITY_SINGLESTEP = 0.01;
    inline constexpr double FLOATINGWINDOW_OPACITY_MIN = 0.1;
    inline constexpr double FLOATINGWINDOW_OPACITY_MAX = 1.0;
    inline constexpr double FLOATINGWINDOW_OPACITY_DEFAULT = 1.0;

    inline constexpr bool FLOATINGWINDOW_MOUSE_PASSTHROUGH_DEFAULT = false;

    inline constexpr const char FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_DEFAULT[]  = "F11";
    // inline constexpr const char FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_NONE[]  = "None";
    // inline constexpr int FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_NONE_INDEX = 0;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // QColor supports constexpr in Qt6
    inline constexpr QColor SPECIAL_COLOR_COMBOBOX_TEXT = QColor(46, 134, 222);
#else
    // Fallback for Qt5: QColor is not constexpr
    inline const QColor SPECIAL_COLOR_COMBOBOX_TEXT = QColor(46, 134, 222);
#endif

    inline constexpr const char CROSSHAIR_CENTERCOLOR_DEFAULT[] = "70a1ff";
    inline constexpr const char CROSSHAIR_CROSSHAIRCOLOR_DEFAULT[] = "70a1ff";
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // QColor supports constexpr in Qt6
    inline constexpr QColor CROSSHAIR_CENTERCOLOR_DEFAULT_QCOLOR = QColor(112, 161, 255);
    inline constexpr QColor CROSSHAIR_CROSSHAIRCOLOR_DEFAULT_QCOLOR = QColor(112, 161, 255);
#else
    // Fallback for Qt5: QColor is not constexpr
    inline const QColor CROSSHAIR_CENTERCOLOR_DEFAULT_QCOLOR = QColor(112, 161, 255);
    inline const QColor CROSSHAIR_CROSSHAIRCOLOR_DEFAULT_QCOLOR = QColor(112, 161, 255);
#endif

    inline constexpr int CROSSHAIR_OPACITY_MIN = 0;
    inline constexpr int CROSSHAIR_OPACITY_MAX = 255;
    inline constexpr int CROSSHAIR_CENTERSIZE_MIN = 1;
    inline constexpr int CROSSHAIR_CENTERSIZE_MAX = 500;
    inline constexpr int CROSSHAIR_CENTERSIZE_DEFAULT = 2;
    inline constexpr int CROSSHAIR_CENTEROPACITY_DEFAULT = 200;
    inline constexpr int CROSSHAIR_CROSSHAIRWIDTH_MIN = 1;
    inline constexpr int CROSSHAIR_CROSSHAIRWIDTH_MAX = 500;
    inline constexpr int CROSSHAIR_CROSSHAIRWIDTH_DEFAULT = 2;
    inline constexpr int CROSSHAIR_CROSSHAIRLENGTH_MIN = 1;
    inline constexpr int CROSSHAIR_CROSSHAIRLENGTH_MAX = 900;
    inline constexpr int CROSSHAIR_CROSSHAIRLENGTH_DEFAULT = 30;
    inline constexpr int CROSSHAIR_CROSSHAIROPACITY_DEFAULT = 200;
    inline constexpr int CROSSHAIR_X_OFFSET_MIN = -5000;
    inline constexpr int CROSSHAIR_X_OFFSET_MAX = 5000;
    inline constexpr int CROSSHAIR_X_OFFSET_DEFAULT = 0;
    inline constexpr int CROSSHAIR_Y_OFFSET_MIN = -5000;
    inline constexpr int CROSSHAIR_Y_OFFSET_MAX = 5000;
    inline constexpr int CROSSHAIR_Y_OFFSET_DEFAULT = 0;

    inline constexpr int DATA_PORT_MIN = 1;
    inline constexpr int DATA_PORT_MAX = 65535;
    inline constexpr int DATA_PORT_DEFAULT = 5300;

    inline constexpr int GRIP_THRESHOLD_DECIMALS = 5;
    inline constexpr double GRIP_THRESHOLD_BRAKE_MIN = 0.00001;
    inline constexpr double GRIP_THRESHOLD_BRAKE_MAX = 1000.00000;
    inline constexpr double GRIP_THRESHOLD_BRAKE_DEFAULT = 0.47000;
    inline constexpr double GRIP_THRESHOLD_ACCEL_MIN = 0.00001;
    inline constexpr double GRIP_THRESHOLD_ACCEL_MAX = 1000.00000;
    inline constexpr double GRIP_THRESHOLD_ACCEL_DEFAULT = 1.00000;
    inline constexpr double GRIP_THRESHOLD_SINGLE_STEP = 0.01;

    inline constexpr int MOUSE_SPEED_MIN = 1;
    inline constexpr int MOUSE_SPEED_MAX = 99;
    inline constexpr int MOUSE_SPEED_DEFAULT = 3;

    inline constexpr int MOUSE_POLLING_INTERNAL_MIN     = 1;
    inline constexpr int MOUSE_POLLING_INTERNAL_MAX     = 99;
    inline constexpr int MOUSE_POLLING_INTERNAL_DEFAULT = 2;

    inline constexpr int UI_SCALE_NORMAL = 0;
    inline constexpr int UI_SCALE_1K_PERCENT_100 = 1;
    inline constexpr int UI_SCALE_1K_PERCENT_125 = 2;
    inline constexpr int UI_SCALE_1K_PERCENT_150 = 3;
    inline constexpr int UI_SCALE_2K_PERCENT_100 = 4;
    inline constexpr int UI_SCALE_2K_PERCENT_125 = 5;
    inline constexpr int UI_SCALE_2K_PERCENT_150 = 6;
    inline constexpr int UI_SCALE_4K_PERCENT_100 = 7;
    inline constexpr int UI_SCALE_4K_PERCENT_125 = 8;
    inline constexpr int UI_SCALE_4K_PERCENT_150 = 9;

    inline constexpr int DISPLAY_SCALE_DEFAULT      = 0;
    inline constexpr int DISPLAY_SCALE_PERCENT_100  = 1;
    inline constexpr int DISPLAY_SCALE_PERCENT_125  = 2;
    inline constexpr int DISPLAY_SCALE_PERCENT_150  = 3;
    inline constexpr int DISPLAY_SCALE_PERCENT_175  = 4;
    inline constexpr int DISPLAY_SCALE_PERCENT_200  = 5;
    inline constexpr int DISPLAY_SCALE_MIN = DISPLAY_SCALE_DEFAULT;
    inline constexpr int DISPLAY_SCALE_MAX = DISPLAY_SCALE_PERCENT_200;

    inline constexpr int MOUSE_POINT_RADIUS = 12;

    inline constexpr int SHOW_POINTSIN_SCREEN_OFF  = 0;
    inline constexpr int SHOW_POINTSIN_SCREEN_ON   = 1;
    inline constexpr int SHOW_POINTSIN_WINDOW_OFF  = 2;
    inline constexpr int SHOW_POINTSIN_WINDOW_ON   = 3;

    inline constexpr int NOTIFICATION_POSITION_NONE            = 0;
    inline constexpr int NOTIFICATION_POSITION_TOP_LEFT        = 1;
    inline constexpr int NOTIFICATION_POSITION_TOP_CENTER      = 2;
    inline constexpr int NOTIFICATION_POSITION_TOP_RIGHT       = 3;
    inline constexpr int NOTIFICATION_POSITION_BOTTOM_LEFT     = 4;
    inline constexpr int NOTIFICATION_POSITION_BOTTOM_CENTER   = 5;
    inline constexpr int NOTIFICATION_POSITION_BOTTOM_RIGHT    = 6;
    inline constexpr int NOTIFICATION_POSITION_DEFAULT = NOTIFICATION_POSITION_TOP_RIGHT;

    inline constexpr int STARTUP_POSITION_DEFAULT   = 0;
    inline constexpr int STARTUP_POSITION_LASTSAVED = 1;
    inline constexpr int STARTUP_POSITION_SPECIFY   = 2;
    inline constexpr int STARTUP_POSITION_MIN = STARTUP_POSITION_DEFAULT;
    inline constexpr int STARTUP_POSITION_MAX = STARTUP_POSITION_SPECIFY;

    inline constexpr int STARTUP_SPECIFY_POSITION_MIN_X = -50000;
    inline constexpr int STARTUP_SPECIFY_POSITION_MIN_Y = -50000;
    inline constexpr int STARTUP_SPECIFY_POSITION_MAX_X = 50000;
    inline constexpr int STARTUP_SPECIFY_POSITION_MAX_Y = 50000;
    inline constexpr QPoint STARTUP_SPECIFY_POSITION_DEFAULT = QPoint(0, 0);

    inline constexpr int RECONNECT_VIGEMCLIENT_WAIT_TIME = 2000;

    inline constexpr int RECONNECT_FAKERINPUTCLIENT_WAIT_TIME = 2000;

    // ViGEmBus driver installation detection constants
    inline constexpr int VIGEMBUS_INSTALL_WAIT_INTERVAL = 50;                                       // Check every 50ms
    inline constexpr int VIGEMBUS_INSTALL_WAIT_TIMEOUT = 8000/VIGEMBUS_INSTALL_WAIT_INTERVAL;       // Total timeout 8 seconds

    // FakerInput driver installation detection constants
    inline constexpr int FAKERINPUT_INSTALL_WAIT_INTERVAL = 50;                                     // Check every 50ms
    inline constexpr int FAKERINPUT_INSTALL_WAIT_TIMEOUT = 8000/FAKERINPUT_INSTALL_WAIT_INTERVAL;   // Total timeout 8 seconds

    inline constexpr int INSTALL_INTERCEPTION_LOOP_WAIT_TIME = 50;
    inline constexpr int INSTALL_INTERCEPTION_LOOP_WAIT_TIME_MAX = 5000/INSTALL_INTERCEPTION_LOOP_WAIT_TIME;

    /* constant values for QKeyMapper_Worker */
    inline constexpr int KEY_INIT = -1;
    inline constexpr int KEY_UP = 0;
    inline constexpr int KEY_DOWN = 1;

    inline constexpr int INPUT_INIT        = -1;
    inline constexpr int INPUT_KEY_UP      = 0;
    inline constexpr int INPUT_KEY_DOWN    = 1;
    inline constexpr int INPUT_MOUSE_WHEEL = 2;

    inline constexpr bool EXTENED_FLAG_TRUE = true;
    inline constexpr bool EXTENED_FLAG_FALSE = false;

    inline constexpr int INITIAL_ROW_INDEX = -1;

    inline constexpr int INITIAL_PLAYER_INDEX = -1;

    inline constexpr int INITIAL_KEYBOARD_INDEX = -1;
    inline constexpr int INITIAL_MOUSE_INDEX = -1;

    inline constexpr int HOOKPROC_STATE_STOPPED            = 0;
    inline constexpr int HOOKPROC_STATE_STARTED            = 1;
    inline constexpr int HOOKPROC_STATE_STOPPING           = 2;
    inline constexpr int HOOKPROC_STATE_STARTING           = 3;
    inline constexpr int HOOKPROC_STATE_RESTART_STOPPING   = 4;
    inline constexpr int HOOKPROC_STATE_RESTART_STARTING   = 5;

    inline constexpr int KEY_INTERCEPT_NONE                            = 0;
    inline constexpr int KEY_INTERCEPT_BLOCK                           = 1;
    inline constexpr int KEY_INTERCEPT_PASSTHROUGH                     = 2;
    inline constexpr int KEY_INTERCEPT_BLOCK_COMBINATIONKEYUP          = 3;
    inline constexpr int KEY_INTERCEPT_BLOCK_KEY_RECORD                = 4;
    inline constexpr int KEY_INTERCEPT_LONGPRESS_HANDLED               = 5;

    inline constexpr int KEY_PROC_NONE             = 0;
    inline constexpr int KEY_PROC_BURST            = 1;
    inline constexpr int KEY_PROC_LOCK             = 2;
    inline constexpr int KEY_PROC_LOCK_PASSTHROUGH = 3;
    inline constexpr int KEY_PROC_PASSTHROUGH      = 4;

    inline constexpr int SENDTIMING_NORMAL             = 0;
    inline constexpr int SENDTIMING_KEYDOWN            = 1;
    inline constexpr int SENDTIMING_KEYUP              = 2;
    inline constexpr int SENDTIMING_KEYDOWN_AND_KEYUP  = 3;
    inline constexpr int SENDTIMING_NORMAL_AND_KEYUP   = 4;

    inline constexpr int PASTETEXT_MODE_SHIFTINSERT     = 0;
    inline constexpr int PASTETEXT_MODE_CTRLV           = 1;

    inline constexpr int SENDMAPPINGKEY_METHOD_SENDINPUT    = 0;
    inline constexpr int SENDMAPPINGKEY_METHOD_SENDMESSAGE  = 1;
    inline constexpr int SENDMAPPINGKEY_METHOD_FAKERINPUT   = 2;

    inline constexpr uint LOCK_STATE_LOCKOFF       = 0;
    inline constexpr uint LOCK_STATE_LOCKON        = 1;
    inline constexpr uint LOCK_STATE_LOCKON_PLUS   = 2;

    inline constexpr int SPLIT_WITH_PLUS           = 0;
    inline constexpr int SPLIT_WITH_NEXT           = 1;
    inline constexpr int SPLIT_WITH_PLUSANDNEXT    = 2;

    inline constexpr int REMOVE_MAPPINGTAB_FAILED  = 0;
    inline constexpr int REMOVE_MAPPINGTAB_LASTONE = 1;
    inline constexpr int REMOVE_MAPPINGTAB_SUCCESS = 2;

    inline constexpr int TRY_LOCK_WAIT_TIME = 1000;

    inline constexpr int SENDMESSAGE_TIMEOUT = 200;

#ifdef FAKERINPUT_EXTRAINFO
    // Time window for matching (500ms) - events older than this are considered stale
    inline constexpr qint64 FAKERINPUT_EXTRAINFO_MATCH_TIME_WINDOW_MS = 500;
#endif // FAKERINPUT_EXTRAINFO

    // FakerInput HID Mouse Button Bit Definitions
    // According to HID Usage Tables for Generic Desktop Page (Mouse)
    inline constexpr BYTE FAKERINPUT_MOUSE_BUTTON_LEFT      = 0x01; // Button 1 (Left)
    inline constexpr BYTE FAKERINPUT_MOUSE_BUTTON_RIGHT     = 0x02; // Button 2 (Right)
    inline constexpr BYTE FAKERINPUT_MOUSE_BUTTON_MIDDLE    = 0x04; // Button 3 (Middle)
    inline constexpr BYTE FAKERINPUT_MOUSE_BUTTON_X1        = 0x08; // Button 4 (X1/Back)
    inline constexpr BYTE FAKERINPUT_MOUSE_BUTTON_X2        = 0x10; // Button 5 (X2/Forward)

    inline constexpr int SENDVIRTUALKEY_STATE_NORMAL           = 0;
    inline constexpr int SENDVIRTUALKEY_STATE_FORCE            = 1;
    inline constexpr int SENDVIRTUALKEY_STATE_MODIFIERS        = 2;
    inline constexpr int SENDVIRTUALKEY_STATE_BURST_TIMEOUT    = 3;
    inline constexpr int SENDVIRTUALKEY_STATE_BURST_STOP       = 4;
    inline constexpr int SENDVIRTUALKEY_STATE_KEYSEQ_NORMAL    = 5;
    inline constexpr int SENDVIRTUALKEY_STATE_KEYSEQ_HOLDDOWN  = 6;
    inline constexpr int SENDVIRTUALKEY_STATE_KEYSEQ_REPEAT    = 7;

    inline constexpr int SENDMODE_NORMAL                           = 1;
    inline constexpr int SENDMODE_FORCE_STOP                       = 2;
    inline constexpr int SENDMODE_BURSTKEY_START                   = 3;
    inline constexpr int SENDMODE_BURSTKEY_STOP                    = 4;
    inline constexpr int SENDMODE_BURSTKEY_STOP_ON_HOOKSTOPPED     = 5;
    inline constexpr int SENDMODE_BURSTKEY_STOP_ON_HOOKRESTART     = 6;
    inline constexpr int SENDMODE_KEYSEQ_NORMAL                    = 7;
    inline constexpr int SENDMODE_KEYSEQ_HOLDDOWN                  = 8;
    inline constexpr int SENDMODE_KEYSEQ_REPEAT                    = 9;

    inline constexpr int SENDTYPE_NORMAL   = 0;
    inline constexpr int SENDTYPE_DOWN     = 1;
    inline constexpr int SENDTYPE_UP       = 2;
    inline constexpr int SENDTYPE_BOTH     = 3;
    inline constexpr int SENDTYPE_EXCLUSION = 4;

    inline constexpr int SENDTYPE_BOTH_WAITTIME = 20;

    inline constexpr int INPUTSTOP_NONE    = 0;
    inline constexpr int INPUTSTOP_SINGLE  = 1;
    inline constexpr int INPUTSTOP_KEYSEQ  = 2;

    inline constexpr int MOUSE_WHEEL_UP = 1;
    inline constexpr int MOUSE_WHEEL_DOWN = 2;

    inline constexpr int MOUSE_WHEEL_KEYUP_WAITTIME = 20;

    inline constexpr int SETMOUSEPOSITION_WAITTIME_MAX = 100;

    inline constexpr WORD XBUTTON_NONE = 0x0000;

    inline constexpr int JOYSTICK_INVALID_INSTANCE_ID = -1;

    inline constexpr int JOYSTICK_PLAYER_INDEX_MIN = 0;
    inline constexpr int JOYSTICK_PLAYER_INDEX_MAX = 9;

    inline constexpr int JOYSTICK_POV_ANGLE_RELEASE = -1;
    inline constexpr int JOYSTICK_POV_ANGLE_UP      = 0;
    inline constexpr int JOYSTICK_POV_ANGLE_DOWN    = 180;
    inline constexpr int JOYSTICK_POV_ANGLE_LEFT    = 270;
    inline constexpr int JOYSTICK_POV_ANGLE_RIGHT   = 90;
    inline constexpr int JOYSTICK_POV_ANGLE_L_UP    = 315;
    inline constexpr int JOYSTICK_POV_ANGLE_L_DOWN  = 225;
    inline constexpr int JOYSTICK_POV_ANGLE_R_UP    = 45;
    inline constexpr int JOYSTICK_POV_ANGLE_R_DOWN  = 135;

    inline constexpr int JOYSTICK_AXIS_LS_HORIZONTAL    = 0;
    inline constexpr int JOYSTICK_AXIS_LS_VERTICAL      = 1;
    inline constexpr int JOYSTICK_AXIS_RS_HORIZONTAL    = 2;
    inline constexpr int JOYSTICK_AXIS_RS_VERTICAL      = 3;
    inline constexpr int JOYSTICK_AXIS_LT_BUTTON        = 4;
    inline constexpr int JOYSTICK_AXIS_RT_BUTTON        = 5;

    inline constexpr qreal JOYSTICK_AXIS_NEAR_ZERO_THRESHOLD = 1e-04;

    // inline constexpr qreal JOYSTICK_AXIS_LT_RT_KEYUP_THRESHOLD_GAMECONTROLLER       = 0.15;
    // inline constexpr qreal JOYSTICK_AXIS_LT_RT_KEYDOWN_THRESHOLD_GAMECONTROLLER     = 0.5;

    // inline constexpr qreal JOYSTICK_AXIS_LT_RT_KEYUP_THRESHOLD_JOYSTICK             = -0.7;
    // inline constexpr qreal JOYSTICK_AXIS_LT_RT_KEYDOWN_THRESHOLD_JOYSTICK           = 0;

    // inline constexpr qreal JOYSTICK_AXIS_LS_RS_VERTICAL_UP_THRESHOLD                = -0.5;
    // inline constexpr qreal JOYSTICK_AXIS_LS_RS_VERTICAL_DOWN_THRESHOLD              = 0.5;
    // inline constexpr qreal JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MIN_THRESHOLD       = -0.15;
    // inline constexpr qreal JOYSTICK_AXIS_LS_RS_VERTICAL_RELEASE_MAX_THRESHOLD       = 0.15;

    // inline constexpr qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_LEFT_THRESHOLD            = -0.5;
    // inline constexpr qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RIGHT_THRESHOLD           = 0.5;
    // inline constexpr qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MIN_THRESHOLD     = -0.15;
    // inline constexpr qreal JOYSTICK_AXIS_LS_RS_HORIZONTAL_RELEASE_MAX_THRESHOLD     = 0.15;

    inline constexpr int GAMEPAD_THRESHOLD_PERCENT_MIN                          = 0;
    inline constexpr int GAMEPAD_THRESHOLD_PERCENT_MAX                          = 100;
    inline constexpr int GAMEPAD_TRIGGER_PRESS_THRESHOLD_PERCENT_DEFAULT        = 50;
    inline constexpr int GAMEPAD_TRIGGER_RELEASE_THRESHOLD_PERCENT_DEFAULT      = 15;
    inline constexpr int GAMEPAD_STICK_PUSH_THRESHOLD_PERCENT_DEFAULT           = 50;
    inline constexpr int GAMEPAD_STICK_RELEASE_THRESHOLD_PERCENT_DEFAULT        = 15;

    inline constexpr qreal JOYSTICK2MOUSE_AXIS_MINUS_LOW_THRESHOLD  = -0.25;
    inline constexpr qreal JOYSTICK2MOUSE_AXIS_MINUS_MID_THRESHOLD  = -0.50;
    inline constexpr qreal JOYSTICK2MOUSE_AXIS_MINUS_HIGH_THRESHOLD  = -0.75;
    inline constexpr qreal JOYSTICK2MOUSE_AXIS_PLUS_LOW_THRESHOLD   = 0.25;
    inline constexpr qreal JOYSTICK2MOUSE_AXIS_PLUS_MID_THRESHOLD   = 0.50;
    inline constexpr qreal JOYSTICK2MOUSE_AXIS_PLUS_HIGH_THRESHOLD   = 0.75;

    inline constexpr int MOUSE_CURSOR_BOTTOMRIGHT_X = 65535;
    inline constexpr int MOUSE_CURSOR_BOTTOMRIGHT_Y = 65535;

    inline constexpr int KEY2MOUSE_CYCLECHECK_TIMEOUT = 2;

    inline constexpr int JOY2VJOY_LEFTSTICK_X   = 1;
    inline constexpr int JOY2VJOY_LEFTSTICK_Y   = 2;
    inline constexpr int JOY2VJOY_RIGHTSTICK_X  = 3;
    inline constexpr int JOY2VJOY_RIGHTSTICK_Y  = 4;

    inline constexpr int GAMEPADINFO_GYRO_NONE       = 0;
    inline constexpr int GAMEPADINFO_GYRO_ENABLED    = 1;
    inline constexpr int GAMEPADINFO_GYRO_DISABLED   = 2;

    // Gyro2Mouse setup
    inline constexpr double GYRO2MOUSE_SPEED_MIN = 0.0;
    inline constexpr double GYRO2MOUSE_SPEED_MAX = 99.99;
    inline constexpr double GYRO2MOUSE_SPEED_SINGLESTEP = 0.1;
    inline constexpr double GYRO2MOUSE_SPEED_DEFAULT = 3.0;

    inline constexpr double GYRO2MOUSE_GYRO_SENS_MIN = 0.0;
    inline constexpr double GYRO2MOUSE_GYRO_SENS_MAX = 200.0;
    inline constexpr double GYRO2MOUSE_GYRO_SENS_SINGLESTEP = 0.1;
    inline constexpr double GYRO2MOUSE_MIN_GYRO_SENS_DEFAULT = 7.0;
    inline constexpr double GYRO2MOUSE_MAX_GYRO_SENS_DEFAULT = 15.0;

    inline constexpr double GYRO2MOUSE_GYRO_THRESHOLD_MIN = 0.0;
    inline constexpr double GYRO2MOUSE_GYRO_THRESHOLD_MAX = 500.0;
    inline constexpr double GYRO2MOUSE_GYRO_THRESHOLD_SINGLESTEP = 0.1;
    inline constexpr double GYRO2MOUSE_MIN_GYRO_THRESHOLD_DEFAULT = 1.5;
    inline constexpr double GYRO2MOUSE_MAX_GYRO_THRESHOLD_DEFAULT = 15.0;

    inline constexpr float GYRO2MOUSE_GYRO_SMOOTH_THRESHOLD = 5;
    inline constexpr float GYRO2MOUSE_GYRO_CUTOFF_RECOVERY = 5;

    inline constexpr int GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_X = 0;
    inline constexpr int GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Y = 1;
    inline constexpr int GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Z = 2;

    inline constexpr int GYRO2MOUSE_MOUSE_X_INPUT_SOURCE_DEFAULT = 1;
    inline constexpr int GYRO2MOUSE_MOUSE_Y_INPUT_SOURCE_DEFAULT = 0;

    inline constexpr USHORT VIRTUALGAMPAD_VENDORID_X360     = 0x045E;
    inline constexpr USHORT VIRTUALGAMPAD_PRODUCTID_X360    = 0xABCD;

    inline constexpr USHORT VIRTUALGAMPAD_VENDORID_DS4      = 0x054C;
    inline constexpr USHORT VIRTUALGAMPAD_PRODUCTID_DS4     = 0x05C4;
    inline constexpr const char VIRTUALGAMPAD_SERIAL_PREFIX_DS4[] = "c0-13-37-";

    inline constexpr BYTE XINPUT_TRIGGER_MIN     = 0;
    inline constexpr BYTE XINPUT_TRIGGER_MAX     = 255;

    inline constexpr BYTE VJOY_PUSHLEVEL_MIN     = 0;
    inline constexpr BYTE VJOY_PUSHLEVEL_MAX     = 255;

    inline constexpr SHORT XINPUT_THUMB_MIN     = -32768;
    inline constexpr SHORT XINPUT_THUMB_RELEASE = 0;
    inline constexpr SHORT XINPUT_THUMB_MAX     = 32767;

    inline constexpr uint VJOY_STICK_RADIUS_MIN  = 0;
    inline constexpr uint VJOY_STICK_RADIUS_MAX  = 255;

    inline constexpr qreal THUMB_DISTANCE_MAX   = 32767;

    inline constexpr BYTE AUTO_BRAKE_ADJUST_VALUE = 4;
    inline constexpr BYTE AUTO_ACCEL_ADJUST_VALUE = 8;
    inline constexpr BYTE AUTO_BRAKE_DEFAULT = 23 * AUTO_BRAKE_ADJUST_VALUE + 3;
    inline constexpr BYTE AUTO_ACCEL_DEFAULT = 23 * AUTO_ACCEL_ADJUST_VALUE + 7;

    inline constexpr double GRIP_THRESHOLD_MAX = 1000.00000;

    inline constexpr qsizetype FORZA_MOTOR_7_SLED_DATA_LENGTH = 232;
    inline constexpr qsizetype FORZA_MOTOR_7_DASH_DATA_LENGTH = 311;
    inline constexpr qsizetype FORZA_MOTOR_8_DASH_DATA_LENGTH = 331;
    inline constexpr qsizetype FORZA_HORIZON_DATA_LENGTH = 324;

    inline constexpr qsizetype FIRAT_PART_DATA_LENGTH = 232;
    inline constexpr qsizetype SECOND_PART_DATA_LENGTH = 79;
    inline constexpr qsizetype FORZA_HORIZON_BUFFER_OFFSET = 12;

    inline constexpr int AUTO_ADJUST_NONE   = 0b0000;
    inline constexpr int AUTO_ADJUST_BRAKE  = 0b0001;
    inline constexpr int AUTO_ADJUST_ACCEL  = 0b0010;
    inline constexpr int AUTO_ADJUST_BOTH   = 0b0011;
    inline constexpr int AUTO_ADJUST_LT     = 0b0100;
    inline constexpr int AUTO_ADJUST_RT     = 0b1000;

    inline constexpr int VJOY_UPDATE_NONE           = 0;
    inline constexpr int VJOY_UPDATE_BUTTONS        = 1;
    inline constexpr int VJOY_UPDATE_JOYSTICKS      = 2;
    inline constexpr int VJOY_UPDATE_AUTO_BUTTONS   = 3;

    inline constexpr int VIRTUAL_JOYSTICK_SENSITIVITY_MIN = 0;
    inline constexpr int VIRTUAL_JOYSTICK_SENSITIVITY_MAX = 50000;
    inline constexpr int VIRTUAL_JOYSTICK_SENSITIVITY_DEFAULT = 4000;

    inline constexpr int VIRTUAL_GAMEPAD_NUMBER_MIN = 1;
    inline constexpr int VIRTUAL_GAMEPAD_NUMBER_MAX = 4;

    inline constexpr int MOUSE2VJOY_RECENTER_TIMEOUT_MIN = 0;
    inline constexpr int MOUSE2VJOY_RECENTER_TIMEOUT_MAX = 1000;
    inline constexpr int MOUSE2VJOY_RECENTER_TIMEOUT_DEFAULT = 50;

    inline constexpr int VJOY_KEYUP_WAITTIME = 20;

    inline constexpr quint8 VK_GAMEPAD_HOME    = 0x07;
    inline constexpr quint8 VK_CROSSHAIR_NORMAL= 0x0A;
    inline constexpr quint8 VK_CROSSHAIR_TYPEA = 0x0B;
    inline constexpr quint8 VK_GYRO2MOUSE_HOLD = 0x0E;
    inline constexpr quint8 VK_GYRO2MOUSE_MOVE = 0x0F;
    inline constexpr quint8 VK_MOUSE2VJOY_HOLD = 0x3A;
    inline constexpr quint8 VK_KEY2MOUSE_UP    = 0x8A;
    inline constexpr quint8 VK_KEY2MOUSE_DOWN  = 0x8B;
    inline constexpr quint8 VK_KEY2MOUSE_LEFT  = 0x8C;
    inline constexpr quint8 VK_KEY2MOUSE_RIGHT = 0x8D;
    inline constexpr quint8 VK_QKEYMAPPER_FN   = 0x97;

    inline constexpr int INTERCEPTION_RETURN_NORMALSEND = 0;
    inline constexpr int INTERCEPTION_RETURN_BLOCKEDBY_INTERCEPTION = 1;
    inline constexpr int INTERCEPTION_RETURN_BLOCKEDBY_LOWLEVELHOOK = 2;

    inline constexpr unsigned int INTERCEPTION_EXTRA_INFO          = 0xAAAA0000;
    inline constexpr unsigned int INTERCEPTION_EXTRA_INFO_BLOCKED  = 0xBBBB0000;

    inline constexpr ULONG_PTR VIRTUAL_UNICODE_CHAR     = 0xCDCACDC0;
    inline constexpr ULONG_PTR VIRTUAL_CUSTOM_KEYS      = 0x3A3A0000;
    inline constexpr ULONG_PTR VIRTUAL_MOUSE_MOVE       = 0xBFBCBFB0;
    inline constexpr ULONG_PTR VIRTUAL_MOUSE_MOVE_BYKEYS= 0x3F3F3F30;

    inline constexpr UINT SCANCODE_CTRL            = 0x1D;
    inline constexpr UINT SCANCODE_ALT             = 0x38;
    inline constexpr UINT SCANCODE_LSHIFT          = 0x2A;
    inline constexpr UINT SCANCODE_RSHIFT          = 0x36;
    inline constexpr UINT SCANCODE_DIVIDE          = 0x35;
    inline constexpr UINT SCANCODE_NUMLOCK         = 0x45;
    inline constexpr UINT SCANCODE_SNAPSHOT        = 0x54;
    inline constexpr UINT SCANCODE_PRINTSCREEN     = 0x37;
    inline constexpr UINT SCANCODE_LWIN            = 0x5B;
    inline constexpr UINT SCANCODE_RWIN            = 0x5C;
    inline constexpr UINT SCANCODE_APPS            = 0x5D;

    inline constexpr int FIXED_VIRTUAL_KEY_CODE_MIN        = 0x00;
    inline constexpr int FIXED_VIRTUAL_KEY_CODE_MAX        = 0xFF;
    inline constexpr int FIXED_VIRTUAL_KEY_CODE_NONE       = 0x00;

    /* Separator chars */
#ifdef SEPARATOR_CONVERT
    inline constexpr const char OLD_SEPARATOR_PLUS[] = " + ";
    inline constexpr const char OLD_SEPARATOR_NEXTARROW[] = " » ";
#endif

    inline constexpr const char SEPARATOR_PLUS[]                = "+";
    inline constexpr const char SEPARATOR_NEXTARROW[]           = "»";
    inline constexpr const char SEPARATOR_WAITTIME[]            = "⏱";
    inline constexpr const char SEPARATOR_LONGPRESS[]           = "⏲";
    inline constexpr const char SEPARATOR_DOUBLEPRESS[]         = "✖";

    inline constexpr const char SEPARATOR_KEYMAPDATA_LEVEL1[]   = "#";
    inline constexpr const char SEPARATOR_KEYMAPDATA_LEVEL2[]   = "|";

    inline constexpr const int MAPPING_KEYS_MAX = 10000;
    inline constexpr const int KEY_SEQUENCE_MAX = 1000000;

    inline constexpr const int RUNWAIT_TIMEOUT = 15000;

    /* constant chars for QKeyMapper */
    inline constexpr const char PROGRAM_NAME[] = "QKeyMapper";
    inline constexpr const char CHECK_UPDATES_URL_GITHUB[] = "https://api.github.com/repos/Zalafina/QKeyMapper/releases/latest";
    inline constexpr const char CHECK_UPDATES_URL_GITEE[] = "https://gitee.com/api/v5/repos/asukavov/QKeyMapper/releases/latest";
    inline constexpr const char UPDATER_USER_AGENT_X64[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 Edg/131.0.0.0";
    inline constexpr const char UPDATE_DOWNLOAD_DIR[] = "update_files";
    inline constexpr const char SETTINGS_BACKUP_DIR[] = "settings_backup";
    inline constexpr const char ZIPUPDATER_DIR[] = "zipupdater";
    inline constexpr const char ZIPUPDATER_EXE[] = "zipupdater.exe";
    inline constexpr const char ZIPUPDATER_DLL_MSVCP[] = "msvcp140.dll";
    inline constexpr const char ZIPUPDATER_DLL_VCRUNTIME[] = "vcruntime140.dll";

    inline constexpr const char LANGUAGECODE_CHINESE[] = "zh_CN";
    inline constexpr const char LANGUAGECODE_ENGLISH[] = "en";
    inline constexpr const char LANGUAGECODE_ENGLISH_US[] = "en_US";
    inline constexpr const char LANGUAGECODE_JAPANESE[] = "ja_JP";

    inline constexpr const char PREFIX_SHORTCUT[] = "★";
    inline constexpr const char OLD_PREFIX_SHORTCUT[] = "★";
    inline constexpr const char JOIN_DEVICE[]     = "##";
    inline constexpr const char PREFIX_PASSTHROUGH[] = "$";
    inline constexpr const char PREFIX_SEND_DOWN[]    = "↓";
    inline constexpr const char PREFIX_SEND_UP[]      = "↑";
    inline constexpr const char PREFIX_SEND_BOTH[]    = "⇵";
    inline constexpr const char PREFIX_SEND_EXCLUSION[] = "！";

    inline constexpr const char KEYBOARD_MODIFIERS[]  = "KeyboardModifiers";
    inline constexpr const char KEYSEQUENCE_STR[]         = "KeySequence";
    inline constexpr const char KEYSEQUENCE_FINAL_STR[]   = "Final";
    inline constexpr const char HOLDDOWN_STR[]        = "HoldDown";
    inline constexpr const char CLEAR_VIRTUALKEYS[]   = "ClearVirtualKeys";

    // inline constexpr const char PROCESS_UNKNOWN[] = "QKeyMapperUnknown";

    inline constexpr const char DEFAULT_NAME[] = "ForzaHorizon4.exe";
    inline constexpr const char CONFIG_FILENAME[] = "keymapdata.ini";
    inline constexpr const char CONFIG_LATEST_FILENAME[] = "keymapdata_latest.ini";
#ifdef SETTINGSFILE_CONVERT
    inline constexpr const char CONFIG_BACKUP_FILENAME[] = "keymapdata_backup.ini";
#endif

    inline constexpr const char DISPLAYSWITCH_KEY_DEFAULT[]    = "L-Ctrl+`";
    inline constexpr const char MAPPINGSWITCH_KEY_DEFAULT[]    = "L-Ctrl+F6";

    inline constexpr const char KEY_RECORD_START_STR[]    = "F11";
    inline constexpr const char KEY_RECORD_STOP_STR[]     = "F12";

    inline constexpr const char SUCCESS_COLOR[] = "#44bd32";
    inline constexpr const char FAILURE_COLOR[] = "#d63031";
    inline constexpr const char WARNING_COLOR[] = "#f39c12";

    /* General global settings >>> */
    inline constexpr const char LAST_WINDOWPOSITION[] = "LastWindowPosition";
    inline constexpr const char PROGRAM_VERSION[] = "ProgramVersion";
    inline constexpr const char PROGRAM_PLATFORM[] = "ProgramPlatform";
    inline constexpr const char LANGUAGE_INDEX[] = "LanguageIndex";
    inline constexpr const char SHOW_PROCESSLIST[] = "ShowProcessList";
    inline constexpr const char SHOW_NOTES[] = "ShowNotes";
    inline constexpr const char SHOW_CATEGORYS[] = "ShowCategorys";
    inline constexpr const char NOTSHOW_VERSION_UNMATCHED[] = "NotShow_VersionUnmatched";
#ifndef ENABLE_SYSTEMFILTERKEYS_DEFAULT
    inline constexpr const char NOTSHOW_FILTERKEYS_DISABLED[] = "NotShow_FilterKeysDisabled";
#endif
    inline constexpr const char CLOSETO_SYSTEMTRAY[] = "CloseToSystemtray";
    inline constexpr const char NOTIFICATION_POSITION[] = "NotificationPosition";
    inline constexpr const char DISPLAY_SCALE[] = "DisplayScale";
    inline constexpr const char THEME_COLOR[] = "ThemeColor";

    inline constexpr const char IGNOREWINDOWINFO_RULES[] = "IgnoreWindowInfo_Rules";
    inline constexpr const char MACROLIST_UNIVERSAL[] = "MacroList_Universal";

    // MacroList for Export and Import
    inline constexpr const char MACROLIST_EXPORT[] = "MacroList_Export";
    inline constexpr const char MACROLIST_EXPORT_DEFAULT_FILENAME[] = "macrolist_export.ini";
    inline constexpr const char MACROLIST_BACKUP_ACTION_EXPORT[] = "MacroListBackupActionExport";
    inline constexpr const char MACROLIST_BACKUP_ACTION_IMPORT[] = "MacroListBackupActionImport";

    // MacroList data field names for INI storage
    inline constexpr const char MACROLIST_FIELD_MACRONAME[] = "macroName";
    inline constexpr const char MACROLIST_FIELD_MACROCONTENT[] = "macroContent";
    inline constexpr const char MACROLIST_FIELD_MACROCATEGORY[] = "macroCategory";
    inline constexpr const char MACROLIST_FIELD_MACRONOTE[] = "macroNote";

    inline constexpr const char NOTIFICATION_FONTCOLOR[] = "Notification_FontColor";
    inline constexpr const char NOTIFICATION_FONTSIZE[] = "Notification_FontSize";
    inline constexpr const char NOTIFICATION_FONTWEIGHT[] = "Notification_FontWeight";
    inline constexpr const char NOTIFICATION_FONTITALIC[] = "Notification_FontItalic";
    inline constexpr const char NOTIFICATION_DISPLAYDURATION[] = "Notification_DisplayDuration";
    inline constexpr const char NOTIFICATION_FADEINDURATION[] = "Notification_FadeInDuration";
    inline constexpr const char NOTIFICATION_FADEOUTDURATION[] = "Notification_FadeOutDuration";
    inline constexpr const char NOTIFICATION_BACKGROUNDCOLOR[] = "Notification_BackgroundColor";
    inline constexpr const char NOTIFICATION_BORDERRADIUS[] = "Notification_BorderRadius";
    inline constexpr const char NOTIFICATION_PADDING[] = "Notification_Padding";
    inline constexpr const char NOTIFICATION_OPACITY[] = "Notification_Opacity";
    inline constexpr const char NOTIFICATION_X_OFFSET[] = "Notification_X_Offset";
    inline constexpr const char NOTIFICATION_Y_OFFSET[] = "Notification_Y_Offset";

    inline constexpr const char STARTUP_POSITION_INDEX[] = "StartupPosition_Index";
    inline constexpr const char STARTUP_POSITION_SPECIFYPOINT[] = "StartupPosition_SpecifyPoint";

    inline constexpr const char TRAYICON_IDLE[] = "TrayIcon_Idle";
    inline constexpr const char TRAYICON_MONITORING[] = "TrayIcon_Monitoring";
    inline constexpr const char TRAYICON_GLOBAL[] = "TrayIcon_Global";
    inline constexpr const char TRAYICON_MATCHED[] = "TrayIcon_Matched";
    inline constexpr const char UPDATE_SITE[] = "UpdateSite";
    inline constexpr const char SETTINGSELECT[] = "SettingSelect";
    inline constexpr const char AUTO_STARTUP[] = "AutoStartup";
    inline constexpr const char STARTUP_MINIMIZED[] = "StartupMinimized";
    inline constexpr const char STARTUP_AUTOMONITORING[] = "StartupAutoMonitoring";
    inline constexpr const char PLAY_SOUNDEFFECT[] = "PlaySoundEffect";
    inline constexpr const char WINDOWSWITCH_KEYSEQ[] = "WindowSwitch_KeySequence";
    inline constexpr const char VIRTUALGAMEPAD_ENABLE[] = "VirtualGamepadEnable";
    inline constexpr const char VIRTUALGAMEPAD_TYPE[] = "VirtualGamepadType";
    inline constexpr const char VIRTUAL_GAMEPADLIST[] = "VirtualGamdpadList";
    inline constexpr const char MULTI_INPUT_ENABLE[] = "MultiInputEnable";
    inline constexpr const char FILTER_KEYS[] = "FilterKeys";
    inline constexpr const char DISABLED_KEYBOARDLIST[] = "DisabledKeyboardList";
    inline constexpr const char DISABLED_MOUSELIST[] = "DisabledMouseList";
    /* General global settings <<< */

    inline constexpr const char GROUPNAME_GLOBALSETTING[] = "QKeyMapperGlobalSetting";
    inline constexpr const char DISPLAYNAME_GLOBALSETTING[] = "GlobalKeyMapping";

    inline constexpr const char KEYMAPDATA_ORIGINALKEYS[] = "KeyMapData_OriginalKeys";
    inline constexpr const char KEYMAPDATA_MAPPINGKEYS[] = "KeyMapData_MappingKeys";
    inline constexpr const char KEYMAPDATA_MAPPINGKEYS_KEYUP[] = "KeyMapData_MappingKeys_KeyUp";
    inline constexpr const char KEYMAPDATA_NOTE[] = "KeyMapData_Note";
    inline constexpr const char KEYMAPDATA_CATEGORY[] = "KeyMapData_Category";
    inline constexpr const char KEYMAPDATA_BURST[] = "KeyMapData_Burst";
    inline constexpr const char KEYMAPDATA_LOCK[] = "KeyMapData_Lock";
    inline constexpr const char KEYMAPDATA_MAPPINGKEYUNLOCK[] = "KeyMapData_MappingKeyUnlock";
    inline constexpr const char KEYMAPDATA_DISABLEORIGINALKEYUNLOCK[] = "KeyMapData_DisableOriginalKeyUnlock";
    inline constexpr const char KEYMAPDATA_DISABLEFNKEYSWITCH[] = "KeyMapData_DisableFnKeySwitch";
    // inline constexpr const char KEYMAPDATA_POSTMAPPINGKEY[] = "KeyMapData_PostMappingKey";
    inline constexpr const char KEYMAPDATA_SENDMAPPINGKEYMETHOD[] = "KeyMapData_SendMappingKeyMethod";
    inline constexpr const char KEYMAPDATA_FIXEDVKEYCODE[] = "KeyMapData_FixedVirtualKeyCode";
    inline constexpr const char KEYMAPDATA_CHECKCOMBKEYORDER[] = "KeyMapData_CheckCombKeyOrder";
    inline constexpr const char KEYMAPDATA_UNBREAKABLE[] = "KeyMapData_Unbreakable";
    inline constexpr const char KEYMAPDATA_PASSTHROUGH[] = "KeyMapData_PassThrough";
    inline constexpr const char KEYMAPDATA_SENDTIMING[] = "KeyMapData_SendTiming";
    inline constexpr const char KEYMAPDATA_PASTETEXTMODE[] = "KeyMapData_PasteTextMode";
    inline constexpr const char KEYMAPDATA_KEYSEQHOLDDOWN[] = "KeyMapData_KeySeqHoldDown";
    inline constexpr const char KEYMAPDATA_BURSTPRESS_TIME[] = "KeyMapData_BurstPressTime";
    inline constexpr const char KEYMAPDATA_BURSTRELEASE_TIME[] = "KeyMapData_BurstReleaseTime";
    inline constexpr const char KEYMAPDATA_REPEATMODE[] = "KeyMapData_RepeatMode";
    inline constexpr const char KEYMAPDATA_REPEATIMES[] = "KeyMapData_RepeatTimes";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_CENTERCOLOR[] = "KeyMapData_Crosshair_CenterColor";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_CENTERSIZE[] = "KeyMapData_Crosshair_CenterSize";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_CENTEROPACITY[] = "KeyMapData_Crosshair_CenterOpacity";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_CROSSHAIRCOLOR[] = "KeyMapData_Crosshair_CrosshairColor";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_CROSSHAIRWIDTH[] = "KeyMapData_Crosshair_CrosshairWidth";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_CROSSHAIRLENGTH[] = "KeyMapData_Crosshair_CrosshairLength";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_CROSSHAIROPACITY[] = "KeyMapData_Crosshair_CrosshairOpacity";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_SHOWCENTER[] = "KeyMapData_Crosshair_ShowCenter";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_SHOWTOP[] = "KeyMapData_Crosshair_ShowTop";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_SHOWBOTTOM[] = "KeyMapData_Crosshair_ShowBottom";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_SHOWLEFT[] = "KeyMapData_Crosshair_ShowLeft";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_SHOWRIGHT[] = "KeyMapData_Crosshair_ShowRight";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_X_OFFSET[] = "KeyMapData_Crosshair_X_Offset";
    inline constexpr const char KEYMAPDATA_CROSSHAIR_Y_OFFSET[] = "KeyMapData_Crosshair_Y_Offset";
    inline constexpr const char KEY2MOUSE_X_SPEED[] = "Key2Mouse_XSpeed";
    inline constexpr const char KEY2MOUSE_Y_SPEED[] = "Key2Mouse_YSpeed";
    inline constexpr const char KEY2MOUSE_POLLING_INTERVAL[] = "Key2Mouse_PollingInterval";
    inline constexpr const char GAMEPAD_LT_THRESHOLD_PRESS[]    = "Gamepad_LT_Threshold_Press";
    inline constexpr const char GAMEPAD_LT_THRESHOLD_RELEASE[]  = "Gamepad_LT_Threshold_Release";
    inline constexpr const char GAMEPAD_RT_THRESHOLD_PRESS[]    = "Gamepad_RT_Threshold_Press";
    inline constexpr const char GAMEPAD_RT_THRESHOLD_RELEASE[]  = "Gamepad_RT_Threshold_Release";
    inline constexpr const char GAMEPAD_LS_THRESHOLD_PUSH[]     = "Gamepad_LS_Threshold_Push";
    inline constexpr const char GAMEPAD_LS_THRESHOLD_RELEASE[]  = "Gamepad_LS_Threshold_Release";
    inline constexpr const char GAMEPAD_RS_THRESHOLD_PUSH[]     = "Gamepad_RS_Threshold_Push";
    inline constexpr const char GAMEPAD_RS_THRESHOLD_RELEASE[]  = "Gamepad_RS_Threshold_Release";

    inline constexpr const char GYRO2MOUSE_X_SPEED[] = "Gyro2Mouse_XSpeed";
    inline constexpr const char GYRO2MOUSE_Y_SPEED[] = "Gyro2Mouse_YSpeed";
    inline constexpr const char GYRO2MOUSE_MIN_GYRO_X_SENSITIVITY[] = "Gyro2Mouse_Min_Gyro_X_Sensitivity";
    inline constexpr const char GYRO2MOUSE_MIN_GYRO_Y_SENSITIVITY[] = "Gyro2Mouse_Min_Gyro_Y_Sensitivity";
    inline constexpr const char GYRO2MOUSE_MAX_GYRO_X_SENSITIVITY[] = "Gyro2Mouse_Max_Gyro_X_Sensitivity";
    inline constexpr const char GYRO2MOUSE_MAX_GYRO_Y_SENSITIVITY[] = "Gyro2Mouse_Max_Gyro_Y_Sensitivity";
    inline constexpr const char GYRO2MOUSE_MIN_GYRO_THRESHOLD[]     = "Gyro2Mouse_Min_Gyro_Threshold";
    inline constexpr const char GYRO2MOUSE_MAX_GYRO_THRESHOLD[]     = "Gyro2Mouse_Max_Gyro_Threshold";

    inline constexpr const char GYRO2MOUSE_MOUSE_X_SOURCE[]   = "Gyro2Mouse_Mouse_X_Source";
    inline constexpr const char GYRO2MOUSE_MOUSE_Y_SOURCE[]   = "Gyro2Mouse_Mouse_Y_Source";
    inline constexpr const char GYRO2MOUSE_MOUSE_X_REVERT[]   = "Gyro2Mouse_Mouse_X_Revert";
    inline constexpr const char GYRO2MOUSE_MOUSE_Y_REVERT[]   = "Gyro2Mouse_Mouse_Y_Revert";

    inline constexpr const char MOUSE2VJOY_X_SENSITIVITY[] = "Mouse2vJoy_XSensitivity";
    inline constexpr const char MOUSE2VJOY_Y_SENSITIVITY[] = "Mouse2vJoy_YSensitivity";
    inline constexpr const char MOUSE2VJOY_INVERT_X[] = "Mouse2vJoy_InvertX";
    inline constexpr const char MOUSE2VJOY_INVERT_Y[] = "Mouse2vJoy_InvertY";
    inline constexpr const char MOUSE2VJOY_LOCKCURSOR[] = "Mouse2vJoy_LockCursor";
    inline constexpr const char MOUSE2VJOY_DIRECTMODE[] = "Mouse2vJoy_DirectMode";
    inline constexpr const char MOUSE2VJOY_RECENTER_TIMEOUT[] = "Mouse2vJoy_Recenter_Timeout";

    // inline constexpr const char MAPPINGTABLE_LASTTABINDEX[] = "MappingTable_LastTabIndex";
    inline constexpr const char MAPPINGTABLE_LASTTABNAME[] = "MappingTable_LastTabName";
    inline constexpr const char MAPPINGTABLE_TABNAMELIST[] = "MappingTable_TabNameList";
    inline constexpr const char MAPPINGTABLE_TABHOTKEYLIST[] = "MappingTable_TabHotkeyList";
    inline constexpr const char MAPPINGTABLE_TABFONTCOLORLIST[] = "MappingTable_TabFontColorList";
    inline constexpr const char MAPPINGTABLE_TABBGCOLORLIST[] = "MappingTable_TabBackgroundColorList";
    inline constexpr const char MAPPINGTABLE_TABHIDENOTIFICATIONLIST[] = "MappingTable_TabHideNotificationList";
    inline constexpr const char MAPPINGTABLE_TABCUSTOMIMAGE_PATHLIST[] = "MappingTable_TabCustomImage_PathList";
    inline constexpr const char MAPPINGTABLE_TABCUSTOMIMAGE_SHOWPOSITIONLIST[] = "MappingTable_TabCustomImage_ShowPositionList";
    inline constexpr const char MAPPINGTABLE_TABCUSTOMIMAGE_PADDINGLIST[] = "MappingTable_TabCustomImage_PaddingList";
    inline constexpr const char MAPPINGTABLE_TABCUSTOMIMAGE_SHOWASTRAYICONLIST[] = "MappingTable_TabCustomImage_ShowAsTrayIconList";
    inline constexpr const char MAPPINGTABLE_TABCUSTOMIMAGE_SHOWASFLOATINGWINDOWLIST[] = "MappingTable_TabCustomImage_ShowAsFloatingWindowList";
    inline constexpr const char MAPPINGTABLE_TABCUSTOMIMAGE_TRAYICON_PIXELLIST[] = "MappingTable_TabCustomImage_TrayIconPixelList";
    inline constexpr const char MAPPINGTABLE_FLOATINGWINDOW_REFERENCEPOINTLIST[] = "MappingTable_FloatingWindow_ReferencePointList";
    inline constexpr const char MAPPINGTABLE_FLOATINGWINDOW_POSITIONLIST[] = "MappingTable_FloatingWindow_PositionList";
    inline constexpr const char MAPPINGTABLE_FLOATINGWINDOW_SIZELIST[] = "MappingTable_FloatingWindow_SizeList";
    inline constexpr const char MAPPINGTABLE_FLOATINGWINDOW_BGCOLORLIST[] = "MappingTable_FloatingWindow_BackgroundColorList";
    inline constexpr const char MAPPINGTABLE_FLOATINGWINDOW_RADIUSLIST[] = "MappingTable_FloatingWindow_RadiusList";
    inline constexpr const char MAPPINGTABLE_FLOATINGWINDOW_OPACITYLIST[] = "MappingTable_FloatingWindow_OpacityList";
    inline constexpr const char MAPPINGTABLE_FLOATINGWINDOW_MOUSEPASSTHROUGHLIST[] = "MappingTable_FloatingWindow_MousePassThroughList";
    inline constexpr const char MAPPINGTABLE_FLOATINGWINDOW_MOUSEPASSTHROUGHSWITCHKEYLIST[] = "MappingTable_FloatingWindow_MousePassThroughSwitchKeyList";
    inline constexpr const char CLEARALL[] = "KeyMapData_ClearAll";

    inline constexpr const char PROCESSINFO_FILENAME[] = "ProcessInfo_FileName";
    inline constexpr const char PROCESSINFO_WINDOWTITLE[] = "ProcessInfo_WindowTitle";
    inline constexpr const char PROCESSINFO_CLASSNAME[] = "ProcessInfo_ClassName";
    inline constexpr const char PROCESSINFO_FILEPATH[] = "ProcessInfo_FilePath";
    inline constexpr const char PROCESSINFO_CUSTOMICONPATH[] = "ProcessInfo_CustomIconPath";
    inline constexpr const char PROCESSINFO_FILENAME_MATCH_INDEX[] = "ProcessInfo_FileNameMatchIndex";
    inline constexpr const char PROCESSINFO_WINDOWTITLE_MATCH_INDEX[] = "ProcessInfo_WindowTitleMatchIndex";
    inline constexpr const char PROCESSINFO_CLASSNAME_MATCH_INDEX[] = "ProcessInfo_ClassNameMatchIndex";
    inline constexpr const char PROCESSINFO_DESCRIPTION[] = "ProcessInfo_Description";

    inline constexpr const char MACROLIST[] = "MacroList";

    inline constexpr const char DATAPORT_NUMBER[] = "DataPortNumber";
    inline constexpr const char GRIP_THRESHOLD_BRAKE[] = "GripThresholdBrake";
    inline constexpr const char GRIP_THRESHOLD_ACCEL[] = "GripThresholdAccel";
    inline constexpr const char AUTOSTARTMAPPING_CHECKED[] = "AutoStartMappingChecked";
    inline constexpr const char ENABLESYSTEMFILTERKEY_CHECKED[] = "EnableSystemFilterKeyChecked";
    inline constexpr const char SENDTOSAMEWINDOWS_CHECKED[] = "SendToSameWindowsChecked";
    inline constexpr const char ACCEPTVIRTUALGAMEPADINPUT_CHECKED[] = "AcceptVirtualGamdpadInputChecked";
    inline constexpr const char PROCESSICON_AS_TRAYICON_CHECKED[] = "ProcessIconAsTrayIconChecked";
    inline constexpr const char SHOW_WINDOW_POINT_KEY[] = "ShowWindowPointKey";
    inline constexpr const char SHOW_SCREEN_POINT_KEY[] = "ShowScreenPointKey";
    inline constexpr const char MAPPINGSTART_KEY[] = "MappingStartKey";
    inline constexpr const char MAPPINGSTOP_KEY[] = "MappingStopKey";

    inline constexpr const DEVPROPKEY DEVPKEY_Device_HardwareIds = {
        {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 3
    };

#ifdef USE_SAOFONT
    inline constexpr const char SAO_FONTFILENAME[] = ":/sao_ui.otf";
#endif

    inline constexpr const char USBIDS_QRC[] = ":/usb.ids";

    inline constexpr const char SOUNDFILE_START_QRC[] = ":/QKeyMapperStart.wav";
    inline constexpr const char SOUNDFILE_START[] = "QKeyMapperStart.wav";
    inline constexpr const char SOUNDFILE_STOP_QRC[] = ":/QKeyMapperStop.wav";
    inline constexpr const char SOUNDFILE_STOP[] = "QKeyMapperStop.wav";

    inline constexpr const char FONTNAME_ENGLISH[] = "Microsoft YaHei UI";
    inline constexpr const char FONTNAME_CHINESE[] = "NSimSun";

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // QColor supports constexpr in Qt6
    inline constexpr QColor STATUS_ON_COLOR = QColor(214, 149, 69);
#else
    // Fallback for Qt5: QColor is not constexpr
    inline const QColor STATUS_ON_COLOR = QColor(214, 149, 69);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // QColor supports constexpr in Qt6
    inline constexpr QColor PASS_THROUGH_COLOR = QColor(214, 149, 69);
#else
    // Fallback for Qt5: QColor is not constexpr
    inline const QColor PASS_THROUGH_COLOR = QColor(214, 149, 69);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // QColor supports constexpr in Qt6
    inline constexpr QColor HOTKEY_TABBAR_TEXT_COLOR = QColor(0, 168, 138);
#else
    // Fallback for Qt5: QColor is not constexpr
    inline const QColor HOTKEY_TABBAR_TEXT_COLOR = QColor(0, 168, 138);
#endif

    inline constexpr const char REGEX_STRING_EMPTYSTRING[] = "^$";

    inline constexpr const char REGEX_PATTERN_NORMALHOTKEY[] = R"(^(\$)?(.+)$)";

    inline constexpr const char REGEX_PATTERN_TABHOTKEY[] = R"(^(\$)?(💾)?(.+)$)";

    // Pattern that matches SendText() or PasteText() for complete content capture
    // Supports both text sending methods: SendText uses character-by-character input, PasteText uses clipboard
    // Capture groups: 1=function_name(SendText|PasteText), 2=text_content
    // This handles complex nested content including multiple parentheses levels and newlines
    // For proper bracket matching, the application logic should handle bracket balancing
    inline constexpr const char REGEX_PATTERN_SENDTEXT[] = R"(^(SendText|PasteText)\(([\s\S]*)\)$)";

    // Pattern for finding SendText/PasteText parts in a composite string (without anchors)
    // Used when searching for text sending segments within a longer mapping string
    // Capture groups: 1=function_name(SendText|PasteText), 2=text_content
    // Uses non-greedy matching to find individual text sending parts, may need special handling for nested brackets
    inline constexpr const char REGEX_PATTERN_SENDTEXT_FIND[] = R"((SendText|PasteText)\(([\s\S]*?)\))";

    // Use pattern that matches everything between Run( and the last ) for complete content capture
    inline constexpr const char REGEX_PATTERN_RUN[] = R"(^Run\((.+)\)$)";

    // Pattern for finding Run( and the first ) parts in a composite string (non-greedy matching)
    inline constexpr const char REGEX_PATTERN_RUN_FIND[] = R"(Run\((.+?)\))";

    // Use pattern that matches everything between SwitchTab( and the last ) for complete content capture
    inline constexpr const char REGEX_PATTERN_SWITCHTAB[] = R"(^SwitchTab(💾)?\((.+)\)$)";

    // Pattern for finding SwitchTab( and the first ) parts in a composite string (non-greedy matching)
    inline constexpr const char REGEX_PATTERN_SWITCHTAB_FIND[] = R"(SwitchTab(💾)?\((.+?)\))";

    // Pattern for matching Unlock(...) mapping keys
    // Matches: Unlock(L-Ctrl+1), Unlock(F3), Unlock(R✖), Unlock(Y+B⏲500)
    // Does not match: Unlock(R✖300), Unlock(Y+B⏲)
    // Capture groups: (1) = full key string, (2) = base key without suffix, (3) = suffix (✖|⏲number), (4) = number for ⏲
    inline constexpr const char REGEX_PATTERN_UNLOCK[] = R"(^Unlock\((([^✖⏲)]+)(✖|⏲(\d+))?)\)$)";

    // Pattern for finding Unlock( and the first ) parts in a composite string (non-greedy matching)
    inline constexpr const char REGEX_PATTERN_UNLOCK_FIND[] = R"(Unlock\((([^✖⏲)]+?)(✖|⏲(\d+))?)\))";

    // Pattern for matching SetVolume(...) and SetMicVolume(...) mapping keys
    // Valid matches: SetVolume(50), SetVolume🔊(50), SetMicVolume(50), SetMicVolume🎤(50)
    //                SetVolume(+10), SetVolume(-5), SetVolume(Mute), SetVolume(MuteOn), SetVolume(MuteOff)
    // Invalid matches (rejected by validation): SetVolume🎤(...), SetMicVolume🔊(...)
    // Capture groups: (1) = optional 'Mic', (2) = optional notification icon (🔊 or 🎤),
    //                 (3) = optional +/- sign, (4) = numeric value or Mute/MuteOn/MuteOff
    // Note: Icon validation is performed at runtime to ensure semantic correctness:
    //       - SetVolume should use 🔊 (speaker icon), not 🎤 (microphone icon)
    //       - SetMicVolume should use 🎤 (microphone icon), not 🔊 (speaker icon)
    inline constexpr const char REGEX_PATTERN_SETVOLUME[] = R"(^Set(Mic)?Volume(🔊|🎤)?\(([+-]?)(\d+(?:\.\d{1,2})?|Mute|MuteOn|MuteOff)\)$)";

    // Pattern for finding SetVolume(...) and SetMicVolume(...) parts in a composite string
    // Valid matches: SetVolume(50), SetVolume🔊(50), SetMicVolume(50), SetMicVolume🎤(50)
    // Invalid combinations (SetVolume🎤, SetMicVolume🔊) are allowed by regex but rejected by validation
    inline constexpr const char REGEX_PATTERN_SETVOLUME_FIND[] = R"(Set(Mic)?Volume(🔊|🎤)?\([+-]?(?:\d+(?:\.\d{1,2})?|Mute|MuteOn|MuteOff)\))";

    // Pattern for matching Repeat{...}x... mapping keys
    // Matches: Repeat{A}x3, Repeat{B+C»D}x10, Repeat{SendText(Hello)}x5
    // Capture groups: (1) = content inside {}, (2) = repeat count
    inline constexpr const char REGEX_PATTERN_REPEAT[] = R"(^Repeat\{(.+)\}x(\d+)$)";

    // Pattern for finding Repeat{...}x... parts in a composite string
    // Uses non-greedy matching for brace content to handle nested structures
    inline constexpr const char REGEX_PATTERN_REPEAT_FIND[] = R"(Repeat\{[^}]+\}x\d+)";

    // Pattern for matching Macro(...) and UniversalMacro(...) mapping keys
    // Matches: Macro(TestMacro), Macro(MyMacro)x3, UniversalMacro(GlobalMacro), UniversalMacro(GlobalMacro)x5
    // Capture groups: (1) = macro type prefix ("" for Macro, "Universal" for UniversalMacro),
    //                 (2) = macro name inside (), (3) = optional repeat count (x followed by number)
    inline constexpr const char REGEX_PATTERN_MACRO[] = R"(^(Universal)?Macro\(([^)]+)\)(?:x(\d+))?$)";

    // Pattern for finding Macro(...) and UniversalMacro(...) parts in a composite string
    // Uses non-greedy matching for macro name to handle multiple occurrences
    inline constexpr const char REGEX_PATTERN_MACRO_FIND[] = R"((Universal)?Macro\([^)]+\)(?:x\d+)?)";

    // Common edit control class names for searching
    inline constexpr const wchar_t* EDIT_CONTROL_CLASSNAMES[] = {
        L"Edit",            // Standard edit control
        L"RichEdit",        // Rich edit control (old)
        L"RichEdit20W",     // Rich edit 2.0
        L"RichEdit50W",     // Rich edit 5.0+
        L"Scintilla",       // Scintilla editor (Notepad++, etc.)
        nullptr
    };

    // PasteText function configuration
    // Time to hold V key down after Ctrl is pressed (milliseconds)
    // This ensures some applications can properly detect the Ctrl+V key combination
    inline constexpr int PASTE_MODIFIER_KEY_HOLD_TIME_MS = 100;
    inline constexpr int PASTE_KEY_HOLD_TIME_MS = 50;

    inline constexpr int PASTETEXT_RESTORE_WAIT_TIME_MS = 100;

    // Repeat function limits
    inline constexpr int REPEAT_COUNT_MIN = 1;
    inline constexpr int REPEAT_COUNT_MAX = 99999;
    inline constexpr int REPEAT_NESTING_LEVEL_MAX = 5;

    inline constexpr int MAPPINGKEY_LINE_EDIT_MAX_LENGTH = 1000000;

    inline constexpr const char CONFIG_FILE_TOPLEVEL_GROUPNAME[] = "General";
    inline constexpr const char SETTING_BACKUP_ACTION_POPUP_NAME[] = "SettingBackupActionPopup";

    inline const QListWidgetItem::ItemType SETTING_BACKUP_LIST_TYPE_SELECT_ALL = static_cast<QListWidgetItem::ItemType>(QListWidgetItem::UserType + 1);
    inline const QListWidgetItem::ItemType SETTING_BACKUP_LIST_TYPE_TOPLEVEL = static_cast<QListWidgetItem::ItemType>(QListWidgetItem::UserType + 2);
    inline const QListWidgetItem::ItemType SETTING_BACKUP_LIST_TYPE_GLOBALSETTING = static_cast<QListWidgetItem::ItemType>(QListWidgetItem::UserType + 3);

    inline constexpr const int GROUPSELECTWIDGET_ITEM_HEIGHT = 21;
    inline constexpr const int GROUPSELECTWIDGET_HEIGHT_ITEM_COUNT_MIN = 20;

    inline constexpr const char GROUPSELECTWIDGET_SELECT_ALL_PREFIX[] = "☑️ ";

    inline constexpr const char SETTING_BACKUP_ACTION_EXPORT[] = "SettingBackupActionExport";
    inline constexpr const char SETTING_BACKUP_ACTION_IMPORT[] = "SettingBackupActionImport";

    inline constexpr const char SETTING_BACKUP_EXPORT_DEFAULT_FILENAME[] = "setting_export.ini";

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    // QColor supports constexpr in Qt6
    inline constexpr QColor SETTING_BACKUP_IMPORT_EXISTING_GROUP_COLOR = QColor(214, 149, 69);
#else
    // Fallback for Qt5: QColor is not constexpr
    inline const QColor SETTING_BACKUP_IMPORT_EXISTING_GROUP_COLOR = QColor(214, 149, 69);
#endif

    inline constexpr const char ORIKEY_COMBOBOX_NAME[] = "orikeyComboBox";
    inline constexpr const char MAPKEY_COMBOBOX_NAME[] = "mapkeyComboBox";
    inline constexpr const char SETUPDIALOG_ORIKEY_COMBOBOX_NAME[] = "SetupDialog_OriginalKeyListComboBox";
    inline constexpr const char SETUPDIALOG_MAPKEY_COMBOBOX_NAME[] = "SetupDialog_MappingKeyListComboBox";
    inline constexpr const char MACROLIST_MAPKEY_COMBOBOX_NAME[] = "MacroList_MappingKeyListComboBox";

    inline constexpr const char SETUPDIALOG_MAPKEY_LINEEDIT_NAME[]        = "SetupDialog_MappingKeyLineEdit";
    inline constexpr const char SETUPDIALOG_MAPKEY_KEYUP_LINEEDIT_NAME[]  = "SetupDialog_MappingKey_KeyUpLineEdit";

    inline constexpr const char WINDOWSWITCHKEY_LINEEDIT_NAME[] = "windowswitchkeyLineEdit";
    inline constexpr const char MAPPINGSTARTKEY_LINEEDIT_NAME[] = "mappingStartKeyLineEdit";
    inline constexpr const char MAPPINGSTOPKEY_LINEEDIT_NAME[] = "mappingStopKeyLineEdit";

    inline constexpr const char MAPPINGTABLE_TAB_TEXT[] = "Tab";

    inline constexpr const char KEY_NONE_STR[] = "NONE";
    inline constexpr const char KEY_BLOCKED_STR[] = "BLOCKED";
    inline constexpr const char GAMEPAD_HOME_STR[] = "GamepadHome";
    inline constexpr const char QKEYMAPPER_FN_KEY_STR[] = "QKeyMapper-Fn";
    inline constexpr const char BLOCK_KEYBOARD_STR[] = "Block-Keyboard";
    inline constexpr const char BLOCK_KEYBOARD_NOTIFY_STR[] = "Block-Keyboard⌨";
    inline constexpr const char BLOCK_MOUSE_STR[] = "Block-Mouse";
    inline constexpr const char BLOCK_MOUSE_NOTIFY_STR[] = "Block-Mouse🖱";
    inline constexpr const char BLOCK_INPUT_PREFIX[]  = "Block-";

    inline constexpr const char MOUSE_MOVE_PREFIX[]  = "Mouse-Move";
    inline constexpr const char MOUSE_BUTTON_PREFIX[]  = "Mouse-";
    inline constexpr const char MOUSE_WINDOWPOINT_POSTFIX[]  = "_WindowPoint";
    inline constexpr const char MOUSE_SCREENPOINT_POSTFIX[]  = "_ScreenPoint";
    inline constexpr const char MOUSE_L_STR[]  = "Mouse-L";
    inline constexpr const char MOUSE_R_STR[]  = "Mouse-R";
    inline constexpr const char MOUSE_M_STR[]  = "Mouse-M";
    inline constexpr const char MOUSE_X1_STR[] = "Mouse-X1";
    inline constexpr const char MOUSE_X2_STR[] = "Mouse-X2";
    inline constexpr const char MOUSE_L_WINDOWPOINT_STR[]  = "Mouse-L_WindowPoint";
    inline constexpr const char MOUSE_R_WINDOWPOINT_STR[]  = "Mouse-R_WindowPoint";
    inline constexpr const char MOUSE_M_WINDOWPOINT_STR[]  = "Mouse-M_WindowPoint";
    inline constexpr const char MOUSE_X1_WINDOWPOINT_STR[] = "Mouse-X1_WindowPoint";
    inline constexpr const char MOUSE_X2_WINDOWPOINT_STR[] = "Mouse-X2_WindowPoint";
    inline constexpr const char MOUSE_L_SCREENPOINT_STR[]  = "Mouse-L_ScreenPoint";
    inline constexpr const char MOUSE_R_SCREENPOINT_STR[]  = "Mouse-R_ScreenPoint";
    inline constexpr const char MOUSE_M_SCREENPOINT_STR[]  = "Mouse-M_ScreenPoint";
    inline constexpr const char MOUSE_X1_SCREENPOINT_STR[] = "Mouse-X1_ScreenPoint";
    inline constexpr const char MOUSE_X2_SCREENPOINT_STR[] = "Mouse-X2_ScreenPoint";
    inline constexpr const char MOUSE_MOVE_WINDOWPOINT_STR[]  = "Mouse-Move_WindowPoint";
    inline constexpr const char MOUSE_MOVE_SCREENPOINT_STR[]  = "Mouse-Move_ScreenPoint";
    inline constexpr const char MOUSE_POS_PREFIX[]  = "Mouse-Pos";
    inline constexpr const char MOUSE_POS_SAVE_STR[] = "Mouse-PosSave";
    inline constexpr const char MOUSE_POS_RESTORE_STR[] = "Mouse-PosRestore";

    inline constexpr QPoint MOUSE_POS_INVALID = QPoint(-50000, -50000);

    inline constexpr const int SHOW_MODE_NONE                = 0;
    inline constexpr const int SHOW_MODE_SCREEN_MOUSEPOINTS  = 1;
    inline constexpr const int SHOW_MODE_WINDOW_MOUSEPOINTS  = 2;
    inline constexpr const int SHOW_MODE_CROSSHAIR_NORMAL    = 3;
    inline constexpr const int SHOW_MODE_CROSSHAIR_TYPEA     = 4;

    inline constexpr const int PICK_WINDOW_POINT_KEY  = VK_LMENU;
    inline constexpr const int PICK_SCREEN_POINT_KEY  = VK_LCONTROL;

    inline constexpr const char SENDTIMING_STR_NORMAL[]               = "NORMAL";
    inline constexpr const char SENDTIMING_STR_KEYDOWN[]              = "KEYDOWN";
    inline constexpr const char SENDTIMING_STR_KEYUP[]                = "KEYUP";
    inline constexpr const char SENDTIMING_STR_KEYDOWN_AND_KEYUP[]    = "KEYDOWN_AND_KEYUP";
    inline constexpr const char SENDTIMING_STR_NORMAL_AND_KEYUP[]     = "NORMAL_AND_KEYUP";

    inline constexpr const char FUNCTION_KEY_NONE[]  = "None";
    inline constexpr int FUNCTION_KEY_NONE_INDEX = 0;

    inline constexpr const char SHOW_KEY_DEBUGINFO[]            = "Application";
    inline constexpr const char SHOW_POINTS_IN_WINDOW_KEY[]     = "F8";
    inline constexpr const char SHOW_POINTS_IN_SCREEN_KEY[]     = "F9";
    inline constexpr const char SHOW_CAR_ORDINAL_KEY[]          = "Home";

    inline constexpr const char MOUSE_WHEEL_STR[]         = "Mouse-Wheel";
    inline constexpr const char MOUSE_WHEEL_UP_STR[]      = "Mouse-WheelUp";
    inline constexpr const char MOUSE_WHEEL_DOWN_STR[]    = "Mouse-WheelDown";
    inline constexpr const char MOUSE_WHEEL_LEFT_STR[]    = "Mouse-WheelLeft";
    inline constexpr const char MOUSE_WHEEL_RIGHT_STR[]   = "Mouse-WheelRight";

    inline constexpr const char SETTING_DESCRIPTION_FORMAT[] = "%1【%2】";
    inline constexpr const char ORIKEY_WITHNOTE_FORMAT[] = "%1【%2】";

    inline constexpr const char SENDTEXT_STR[]      = "SendText";
    inline constexpr const char PASTETEXT_STR[]     = "PasteText";
    inline constexpr const char RUN_STR[]           = "Run";
    inline constexpr const char SWITCHTAB_STR[]         = "SwitchTab";
    inline constexpr const char SWITCHTAB_SAVE_STR[]    = "SwitchTab💾";
    inline constexpr const char MACRO_STR[]             = "Macro";
    inline constexpr const char UNIVERSAL_MACRO_STR[]   = "UniversalMacro";
    inline constexpr const char REPEAT_STR[]            = "Repeat";
    inline constexpr const char UNLOCK_STR[]            = "Unlock";
    inline constexpr const char SETVOLUME_STR[]             = "SetVolume";
    inline constexpr const char SETVOLUME_NOTIFY_STR[]      = "SetVolume🔊";
    inline constexpr const char SETMICVOLUME_STR[]          = "SetMicVolume";
    inline constexpr const char SETMICVOLUME_NOTIFY_STR[]   = "SetMicVolume🎤";
    inline constexpr const char KEYSEQUENCEBREAK_STR[]  = "KeySequenceBreak";

    inline constexpr const char REPEAT_TEMPLATE_STR[]   = "Repeat{}x5";

    inline constexpr const wchar_t SYSTEM_VERB_PROPERTIES[] = L"properties";

    inline constexpr const char JOY_KEY_PREFIX[]  = "Joy-";
    inline constexpr const char VJOY_KEY_PREFIX[]  = "vJoy-";

    inline constexpr const char VJOY_MOUSE2LS_STR[] = "vJoy-Mouse2LS";
    inline constexpr const char VJOY_MOUSE2RS_STR[] = "vJoy-Mouse2RS";
    inline constexpr const char MOUSE2VJOY_PREFIX[] = "Mouse2vJoy-";
    inline constexpr const char MOUSE2VJOY_HOLD_KEY_STR[] = "Mouse2vJoy-Hold";
    inline constexpr const char MOUSE2VJOY_DIRECT_KEY_STR_DEPRECATED[] = "Mouse2vJoy-Direct";

    inline constexpr const char VJOY_LT_BRAKE_STR[] = "vJoy-Key11(LT)_BRAKE";
    inline constexpr const char VJOY_RT_BRAKE_STR[] = "vJoy-Key12(RT)_BRAKE";
    inline constexpr const char VJOY_LT_ACCEL_STR[] = "vJoy-Key11(LT)_ACCEL";
    inline constexpr const char VJOY_RT_ACCEL_STR[] = "vJoy-Key12(RT)_ACCEL";

    inline constexpr const char VJOY_LS_RADIUS_STR[] = "vJoy-LS-Radius";
    inline constexpr const char VJOY_RS_RADIUS_STR[] = "vJoy-RS-Radius";

    inline constexpr const char JOY_LS2VJOYLS_STR[] = "Joy-LS_2vJoyLS";
    inline constexpr const char JOY_RS2VJOYRS_STR[] = "Joy-RS_2vJoyRS";
    inline constexpr const char JOY_LS2VJOYRS_STR[] = "Joy-LS_2vJoyRS";
    inline constexpr const char JOY_RS2VJOYLS_STR[] = "Joy-RS_2vJoyLS";

    inline constexpr const char JOY_LT2VJOYLT_STR[] = "Joy-Key11(LT)_2vJoyLT";
    inline constexpr const char JOY_RT2VJOYRT_STR[] = "Joy-Key12(RT)_2vJoyRT";

    inline constexpr const char JOY_LS2MOUSE_STR[] = "Joy-LS2Mouse";
    inline constexpr const char JOY_RS2MOUSE_STR[] = "Joy-RS2Mouse";

    inline constexpr const char JOY_GYRO2MOUSE_STR[] = "Joy-Gyro2Mouse";

    inline constexpr const char SENDON_MAPPINGSTART_STR[] = "SendOnMappingStart";
    inline constexpr const char SENDON_SWITCHTAB_STR[]    = "SendOnSwitchTab";

    inline constexpr const char GYRO2MOUSE_PREFIX[] = "Gyro2Mouse-";
    inline constexpr const char GYRO2MOUSE_HOLD_KEY_STR[] = "Gyro2Mouse-Hold";
    inline constexpr const char GYRO2MOUSE_MOVE_KEY_STR[] = "Gyro2Mouse-Move";

    inline constexpr const char KEY2MOUSE_PREFIX[]     = "Key2Mouse-";
    inline constexpr const char KEY2MOUSE_UP_STR[]     = "Key2Mouse-Up";
    inline constexpr const char KEY2MOUSE_DOWN_STR[]   = "Key2Mouse-Down";
    inline constexpr const char KEY2MOUSE_LEFT_STR[]   = "Key2Mouse-Left";
    inline constexpr const char KEY2MOUSE_RIGHT_STR[]  = "Key2Mouse-Right";

    inline constexpr const char CROSSHAIR_PREFIX[]        = "Crosshair-";
    inline constexpr const char CROSSHAIR_NORMAL_STR[]    = "Crosshair-Normal";
    inline constexpr const char CROSSHAIR_TYPEA_STR[]     = "Crosshair-TypeA";

    inline constexpr const char FUNC_PREFIX[]          = "Func-";
    inline constexpr const char FUNC_REFRESH[]         = "Func-Refresh";
    inline constexpr const char FUNC_LOCKSCREEN[]      = "Func-LockScreen";
    inline constexpr const char FUNC_SHUTDOWN[]        = "Func-Shutdown";
    inline constexpr const char FUNC_REBOOT[]          = "Func-Reboot";
    inline constexpr const char FUNC_LOGOFF[]          = "Func-Logoff";
    inline constexpr const char FUNC_SLEEP[]           = "Func-Sleep";
    inline constexpr const char FUNC_HIBERNATE[]       = "Func-Hibernate";

    inline constexpr const char VIRTUAL_GAMEPAD_X360[] = "X360";
    inline constexpr const char VIRTUAL_GAMEPAD_DS4[]  = "DS4";

    inline constexpr const char NO_INPUTDEVICE[]  = "No InputDevice";

    /* Translate const Strings */
    /* Common */
    inline constexpr const char UPDATEBUTTON_STR[] = "Update";
    /* QItemSetupDialog */
    inline constexpr const char ITEMSETUPDIALOG_WINDOWTITLE_STR[] = "Mapping Item Setup";
    inline constexpr const char BURSTCHECKBOX_STR[] = "Burst";
    inline constexpr const char LOCKCHECKBOX_STR[] = "Lock";
    inline constexpr const char MAPPINGKEYUNLOCKCHECKBOX_STR[] = "MappingKeyUnlock";
    inline constexpr const char CHECKCOMBKEYORDERCHECKBOX_STR[] = "CheckCombKeyOrder";
    inline constexpr const char UNBREAKABLECHECKBOX_STR[] = "Unbreakable";
    inline constexpr const char PASSTHROUGHCHECKBOX_STR[] = "PassThrough";
    inline constexpr const char KEYSEQHOLDDOWNCHECKBOX_STR[] = "KeySeqHoldDown";
    inline constexpr const char REPEATBYKEYCHECKBOX_STR[] = "RepeatByKey";
    inline constexpr const char REPEATBYTIMESCHECKBOX_STR[] = "RepeatTimes";
    inline constexpr const char BURSTPRESSLABEL_STR[] = "BurstPress";
    inline constexpr const char BURSTRELEASE_STR[] = "BurstRelease";
    inline constexpr const char ORIGINALKEYLABEL_STR[] = "OriginalKey";
    inline constexpr const char MAPPINGKEYLABEL_STR[] = "MappingKey";
    inline constexpr const char KEYUPMAPPINGLABEL_STR[] = "KeyUpMapping";
    inline constexpr const char ITEMNOTELABEL_STR[] = "Note";
    inline constexpr const char ORIKEYLISTLABEL_STR[] = "OriginalKeyList";
    inline constexpr const char MAPKEYLISTLABEL_STR[] = "MappingKeyList";
    inline constexpr const char RECORDKEYSBUTTON_STR[] = "Record Keys";
    inline constexpr const char CROSSHAIRSETUPBUTTON_STR[] = "CrosshairSetup";
    inline constexpr const char SENDTIMINGLABEL_STR[] = "SendTiming";
    /* QInputDeviceListWindow */
    inline constexpr const char DEVICELIST_WINDOWTITLE_STR[] = "Input Device List";
    inline constexpr const char CONFIRMBUTTON_STR[] = "OK";
    inline constexpr const char CANCELBUTTON_STR[] = "Cancel";
    inline constexpr const char KEYBOARDLABEL_STR[] = "Keyboard";
    inline constexpr const char MOUSELABEL_STR[] = "Mouse";
    /* QTableSetupDialog */
    inline constexpr const char TABLESETUPDIALOG_WINDOWTITLE_STR[] = "Mapping Table Setup";
    inline constexpr const char TABNAMELABEL_STR[] = "TabName";
    inline constexpr const char TABHOTKEYLABEL_STR[] = "TabHotkey";
    inline constexpr const char EXPORTTABLEBUTTON_STR[] = "ExportTable";
    inline constexpr const char IMPORTTABLEBUTTON_STR[] = "ImportTable";
    inline constexpr const char REMOVETABLEBUTTON_STR[] = "RemoveTable";

    /* constant values for QItemSetupDialog */
    inline constexpr const int ITEMSETUP_EDITING_MAPPINGKEY      = 0;
    inline constexpr const int ITEMSETUP_EDITING_KEYUPMAPPINGKEY = 1;

    /* constant values for QInputDeviceListWindow */
    inline constexpr const int DEVICE_TABLE_NUMBER_COLUMN         = 0;
    inline constexpr const int DEVICE_TABLE_DEVICEDESC_COLUMN     = 1;
    inline constexpr const int DEVICE_TABLE_HARDWAREID_COLUMN     = 2;
    inline constexpr const int DEVICE_TABLE_VENDORID_COLUMN       = 3;
    inline constexpr const int DEVICE_TABLE_PRODUCTID_COLUMN      = 4;
    inline constexpr const int DEVICE_TABLE_VENDORSTR_COLUMN      = 5;
    inline constexpr const int DEVICE_TABLE_PRODUCTSTR_COLUMN     = 6;
    inline constexpr const int DEVICE_TABLE_MANUFACTURER_COLUMN   = 7;
    inline constexpr const int DEVICE_TABLE_DISABLE_COLUMN        = 8;

    inline constexpr const int KEYBOARD_TABLE_COLUMN_COUNT    = 9;
    inline constexpr const int MOUSE_TABLE_COLUMN_COUNT       = 9;

    /* Volume Control Constants */
    // Volume control ranges - Windows uses 0.0f to 1.0f (scalar)
    inline constexpr const float VOLUME_MIN_SCALAR           = 0.0f;   // Minimum volume (muted)
    inline constexpr const float VOLUME_MAX_SCALAR           = 1.0f;   // Maximum volume (100%)
    inline constexpr const float VOLUME_MIN_PERCENTAGE       = 0.0f;   // Minimum volume percentage
    inline constexpr const float VOLUME_MAX_PERCENTAGE       = 100.0f; // Maximum volume percentage

    // Volume precision and validation
    inline constexpr const int VOLUME_DECIMAL_PRECISION      = 2;      // Support 2 decimal places
    inline constexpr const float VOLUME_EPSILON              = 0.001f; // Small value for float comparison
    inline constexpr const float VOLUME_FULL                 = 50.0f;  // Volume full value for float comparison

    // Volume control operation types
    inline constexpr const int VOLUME_OPERATION_SET          = 0;      // Set absolute volume
    inline constexpr const int VOLUME_OPERATION_INCREASE     = 1;      // Increase volume by amount
    inline constexpr const int VOLUME_OPERATION_DECREASE     = 2;      // Decrease volume by amount
    inline constexpr const int VOLUME_OPERATION_MUTE_TOGGLE  = 3;      // Toggle mute state
    inline constexpr const int VOLUME_OPERATION_MUTE_ON      = 4;      // Set mute on
    inline constexpr const int VOLUME_OPERATION_MUTE_OFF     = 5;      // Set mute off

    // Volume device types
    inline constexpr const int VOLUME_DEVICE_TYPE_PLAYBACK   = 0;      // Playback device (speakers/headphones)
    inline constexpr const int VOLUME_DEVICE_TYPE_CAPTURE    = 1;      // Capture device (microphone)
}
