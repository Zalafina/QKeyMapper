#pragma once

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

    inline constexpr int VIRTUALGAMEPADTYPECOMBOBOX_X = 610;

    inline constexpr int PROCESSINFO_TABLE_COLUMN_COUNT = 3;
    inline constexpr int KEYMAPPINGDATA_TABLE_COLUMN_COUNT = 5;

    inline constexpr int INITIAL_WINDOW_POSITION = -1;

    inline constexpr int PROCESS_NAME_COLUMN = 0;
    inline constexpr int PROCESS_PID_COLUMN = 1;
    inline constexpr int PROCESS_TITLE_COLUMN = 2;

    inline constexpr int PROCESS_NAME_COLUMN_WIDTH_MAX = 200;

    inline constexpr int ORIGINAL_KEY_COLUMN = 0;
    inline constexpr int MAPPING_KEY_COLUMN = 1;
    inline constexpr int BURST_MODE_COLUMN = 2;
    inline constexpr int LOCK_COLUMN = 3;
    inline constexpr int CATEGORY_COLUMN = 4;

    inline constexpr int CATEGORY_FILTER_ALL_INDEX = 0;

    inline constexpr int KEY_TYPE_COMMON       = 0;
    inline constexpr int KEY_TYPE_KEYBOARD     = 1;
    inline constexpr int KEY_TYPE_MOUSE        = 2;
    inline constexpr int KEY_TYPE_GAMEPAD      = 3;
    inline constexpr int KEY_TYPE_FUNCTION     = 4;

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

    inline constexpr int BEST_ICON_SIZE = 256;
    inline constexpr int DEFAULT_ICON_WIDTH = 48;
    inline constexpr int DEFAULT_ICON_HEIGHT = 48;

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

    inline constexpr int UPDATE_SITE_GITHUB = 0;
    inline constexpr int UPDATE_SITE_GITEE = 1;

    inline constexpr int KEYPRESS_TYPE_LONGPRESS   = 0;
    inline constexpr int KEYPRESS_TYPE_DOUBLEPRESS = 1;

    inline constexpr int GLOBALSETTING_INDEX = 1;

    inline constexpr int WINDOWINFO_MATCH_INDEX_IGNORE     = 0;
    inline constexpr int WINDOWINFO_MATCH_INDEX_EQUALS     = 1;
    inline constexpr int WINDOWINFO_MATCH_INDEX_CONTAINS   = 2;
    inline constexpr int WINDOWINFO_MATCH_INDEX_STARTSWITH = 3;
    inline constexpr int WINDOWINFO_MATCH_INDEX_ENDSWITH   = 4;
    inline constexpr int WINDOWINFO_MATCH_INDEX_DEFAULT    = WINDOWINFO_MATCH_INDEX_CONTAINS;
    inline constexpr int WINDOWINFO_MATCH_INDEX_MIN        = WINDOWINFO_MATCH_INDEX_IGNORE;
    inline constexpr int WINDOWINFO_MATCH_INDEX_MAX        = WINDOWINFO_MATCH_INDEX_ENDSWITH;

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

    inline constexpr int FLOATINGICONWINDOW_OPACITY_DECIMALS = 2;
    inline constexpr double FLOATINGICONWINDOW_OPACITY_SINGLESTEP = 0.01;
    inline constexpr double FLOATINGICONWINDOW_OPACITY_MIN = 0.1;
    inline constexpr double FLOATINGICONWINDOW_OPACITY_MAX = 1.0;
    inline constexpr double FLOATINGICONWINDOW_OPACITY_DEFAULT = 1.0;

    inline constexpr const char NOTIFICATION_COLOR_GLOBAL_DEFAULT_STR[] = "#26de81";
    inline constexpr const char NOTIFICATION_COLOR_NORMAL_DEFAULT_STR[] = "#d6a2e8";

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    inline constexpr QColor NOTIFICATION_COLOR_GLOBAL_DEFAULT = QColor(38, 222, 129);
    inline constexpr QColor NOTIFICATION_COLOR_NORMAL_DEFAULT = QColor(214, 162, 232);
    inline constexpr QColor NOTIFICATION_BACKGROUND_COLOR_DEFAULT = QColor(0,0,0,120);
#else
    inline const QColor NOTIFICATION_COLOR_GLOBAL_DEFAULT = QColor(38, 222, 129);
    inline const QColor NOTIFICATION_COLOR_NORMAL_DEFAULT = QColor(214, 162, 232);
    inline const QColor NOTIFICATION_BACKGROUND_COLOR_DEFAULT = QColor(0,0,0,120);
#endif
}
