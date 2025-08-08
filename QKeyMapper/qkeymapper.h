#ifndef QKEYMAPPER_H
#define QKEYMAPPER_H

#include <QDialog>
#include <QDebug>
#include <QMetaEnum>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QScreen>
#include <QWindow>
#include <QTimer>
#include <QTimerEvent>
#include <QThread>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QHash>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QtWin>
#endif
#include <QOperatingSystemVersion>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QTableWidget>
#include <QPropertyAnimation>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QStandardItemModel>
#include <QValidator>
#include <QDir>
#include <QProcess>
#include <QFileDialog>
#include <QDateTime>
#include <QTranslator>
#include <QLibraryInfo>
#include <QPainter>
#include <cmath>
#include <windows.h>
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#include <Gdiplus.h>
#include <process.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#include <Psapi.h>
#include <tchar.h>
#include <string.h>
#include <dwmapi.h>
#include <Shlobj.h>
#include <powrprof.h>
#include <setupapi.h>
#include <interception.h>
#include <libusb.h>
#include <QSimpleUpdater.h>
//#include <QKeyEvent>
//#include <QProcess>
//#include <QTextCodec>

#include "qkeymapper_worker.h"
#include "qinputdevicelistwindow.h"
#include "qgyro2mouseoptiondialog.h"
#include "qitemsetupdialog.h"
#include "qtablesetupdialog.h"
#include "qtrayiconselectdialog.h"
#include "qnotificationsetupdialog.h"

struct InputDeviceInfo
{
    QString hardwareid;
    QString devicedesc;
    ushort vendorid;
    ushort productid;
    QString VendorStr;
    QString ManufacturerStr;
    QString ProductStr;
};

struct InputDevice
{
    InterceptionDevice device;
    InputDeviceInfo deviceinfo;
    QAtomicBool disabled;
};

struct USBDeviceInfo {
    QString vendorName;
    QString productName;
};

#include "interception_worker.h"

namespace Ui {
class QKeyMapper;
}

typedef struct
{
    QString FileName;
    QString PID;
    QString WindowTitle;
    QString FilePath;
    QIcon   WindowIcon;
}MAP_PROCESSINFO;

struct MousePoint_Info
{
    QString ori_key;
    QString map_key;
    int x;
    int y;

    MousePoint_Info() : ori_key(), map_key(), x(-1), y(-1) {}

#ifdef DEBUG_LOGOUT_ON
    friend QDebug operator<<(QDebug debug, const MousePoint_Info& info)
    {
        QDebugStateSaver saver(debug);
        debug.nospace() << "\nMousePoint_Info[" << "Ori:"<< info.ori_key << ", Map:" << info.map_key << ", X:" << info.x << ", Y:" << info.y << "]";
        return debug;
    }
#endif
};

struct ValidationResult
{
    bool isValid;
    QString errorMessage;
};

struct PopupNotificationOptions {
    QString color = QKeyMapperConstants::NOTIFICATION_COLOR_NORMAL_DEFAULT_STR;             // Text color
    int displayDuration = QKeyMapperConstants::NOTIFICATION_DISPLAY_DURATION_DEFAULT;       // Display duration (ms)
    int position = QKeyMapperConstants::NOTIFICATION_POSITION_TOP_RIGHT;                    // Display position (e.g. NOTIFICATION_POSITION_TOP_RIGHT)
    int size = QKeyMapperConstants::NOTIFICATION_FONT_SIZE_DEFAULT;                         // Font size
    QColor backgroundColor = QKeyMapperConstants::NOTIFICATION_BACKGROUND_COLOR_DEFAULT;    // Background color
    double windowOpacity = QKeyMapperConstants::NOTIFICATION_OPACITY_DEFAULT;               // Window opacity (0.0~1.0)
    int padding = QKeyMapperConstants::NOTIFICATION_PADDING_DEFAULT;                        // Padding
    int borderRadius = QKeyMapperConstants::NOTIFICATION_BORDER_RADIUS_DEFAULT;             // Border radius
    int fontWeight = QFont::Bold;                                                           // Font weight
    bool fontItalic = false;                                                                // Italic font
    int fadeInDuration = QKeyMapperConstants::NOTIFICATION_FADEIN_DURATION_DEFAULT;         // Fade-in animation duration (ms)
    int fadeOutDuration = QKeyMapperConstants::NOTIFICATION_FADEOUT_DURATION_DEFAULT;       // Fade-out animation duration (ms)
    int xOffset = QKeyMapperConstants::NOTIFICATION_X_OFFSET_DEFAULT;                       // X offset
    int yOffset = QKeyMapperConstants::NOTIFICATION_Y_OFFSET_DEFAULT;                       // Y offset

    QString iconPath;                                                                       // Optional icon path (PNG/ICO)
    int iconPosition = QKeyMapperConstants::TAB_CUSTOMIMAGE_POSITION_DEFAULT;               // Icon position
    int iconPadding = 0;                                                                    // Padding between icon and text
};

struct FloatingWindowOptions {
    QPoint position = QKeyMapperConstants::FLOATINGWINDOW_POSITION_DEFAULT;                 // Window position
    int referencePoint = QKeyMapperConstants::FLOATINGWINDOW_REFERENCEPOINT_DEFAULT;        // Window position reference point
    QSize size = QKeyMapperConstants::FLOATINGWINDOW_SIZE_DEFAULT;                          // Window size (square)
    QColor backgroundColor = QKeyMapperConstants::FLOATINGWINDOW_BACKGROUND_COLOR_DEFAULT;  // Background color
    double windowOpacity = QKeyMapperConstants::FLOATINGWINDOW_OPACITY_DEFAULT;             // Window opacity (0.1~1.0)
    bool mousePassThrough = QKeyMapperConstants::FLOATINGWINDOW_MOUSE_PASSTHROUGH_DEFAULT;  // Mouse pass-through
    int borderRadius = QKeyMapperConstants::FLOATINGWINDOW_RADIUS_DEFAULT;                  // Border radius
    QString iconPath;                                                                       // Icon file path (.ico/.png)
    int iconPadding = 0;                                                                    // Padding between icon and window border
};

struct Gamepad_Info
{
    int instance_id;
    QString name;
    ushort vendorid;
    ushort productid;
    QString serial;
    bool isvirtual;
    int gyro_enabled;
    QString info_string;

    Gamepad_Info() : instance_id(-1), name(), vendorid(0), productid(0), serial(), isvirtual(false) {}

#ifdef DEBUG_LOGOUT_ON
    friend QDebug operator<<(QDebug debug, const Gamepad_Info& info)
    {
        QDebugStateSaver saver(debug);
        debug.nospace() << "\nGamepad_Info["
                        << "InstanceID:" << info.instance_id
                        << ", Name:" << info.name
                        << ", VendorID:" << info.vendorid
                        << ", ProductID:" << info.productid
                        << ", Serial:" << info.serial
                        << ", IsVirtual:" << (info.isvirtual ? "true" : "false")
                        << "]";
        return debug;
    }
#endif
};

class KeyMappingTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit KeyMappingTabWidget(QWidget *parent = Q_NULLPTR) : QTabWidget(parent) {
        setMovable(true);  // Enable tab moving
        // Connect the tab bar's moved signal to our slot
        connect(tabBar(), &QTabBar::tabMoved, this, &KeyMappingTabWidget::onTabMoved, Qt::UniqueConnection);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTabMoved(int from, int to);

signals:
    void tabOrderChanged(int from, int to);
};

class KeyMappingDataTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit KeyMappingDataTableWidget(QWidget *parent = Q_NULLPTR)
        : QTableWidget(parent), m_DraggedTopRow(-1), m_DraggedBottomRow(-1) {}

    // Category filtering methods
    void setCategoryFilter(const QString &category);
    void clearCategoryFilter();
    QStringList getAvailableCategories() const;
    void setCategoryColumnVisible(bool visible);
    bool isCategoryColumnVisible() const;

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;

private:
    void updateRowVisibility();

private:
    int m_DraggedTopRow;
    int m_DraggedBottomRow;
    bool m_CategoryColumnVisible = false;  // Category column visibility
public:
    QString m_CategoryFilter = QString();  // Current category filter
};

struct KeyMappingTab_Info
{
    QString TabName;
    QString TabHotkey;
    QColor TabFontColor;
    QColor TabBackgroundColor;
    QString TabCustomImage_Path;
    int TabCustomImage_ShowPosition;
    int TabCustomImage_Padding;
    bool TabCustomImage_ShowAsTrayIcon;
    bool TabCustomImage_ShowAsFloatingWindow;
    QSize TabCustomImage_TrayIconPixel;
    QPoint FloatingWindow_Position;
    int FloatingWindow_ReferencePoint;
    QSize FloatingWindow_Size;
    QColor FloatingWindow_BackgroundColor;
    int FloatingWindow_Radius;
    double  FloatingWindow_Opacity;
    bool FloatingWindow_MousePassThrough;
    QString FloatingWindow_MousePassThroughSwitchKey;
    KeyMappingDataTableWidget *KeyMappingDataTable;
    QList<MAP_KEYDATA> *KeyMappingData;
};

// Structure to store icon information with quality metrics
struct IconInfo {
    HICON hIcon;
    int size;
    int bitCount; // Color depth (bits per pixel)

    IconInfo(HICON icon, int sz, int bits) : hIcon(icon), size(sz), bitCount(bits) {}
};

// Structure to pass data to the enumeration callback
struct IconEnumData {
    QList<IconInfo> icons;  // icon handle, size, and bit count
    HMODULE hModule;
    bool onlyFirstGroup = false;     // Flag to indicate if we only want the first icon group
    bool firstGroupProcessed = false; // Flag to track if first group has been processed
    LPWSTR firstGroupName = nullptr; // Name of the first icon group
};

// Icon group structure: GRPICONDIR followed by GRPICONDIRENTRY array
// Use proper memory alignment for PE resource structures
#pragma pack(push, 2)
struct GRPICONDIR {
    WORD idReserved;
    WORD idType;
    WORD idCount;
};

struct GRPICONDIRENTRY {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID;  // This is the icon resource ID
};
#pragma pack(pop)

class StyledDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    StyledDelegate(QWidget *parent = Q_NULLPTR) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class KeyListComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit KeyListComboBox(QWidget *parent = Q_NULLPTR) : QComboBox(parent)
    {
        if (parent != Q_NULLPTR){
            m_KeyMapper_ptr = parent;
        }
    }

protected:
    void keyPressEvent(QKeyEvent *keyevent) override;
    void mousePressEvent(QMouseEvent *event) override;
    void showPopup(void) override;

private:
    QWidget *m_KeyMapper_ptr;
};

class QPopupNotification : public QWidget
{
    Q_OBJECT

public:
    explicit QPopupNotification(QWidget *parent = Q_NULLPTR);

public slots:
    void showPopupNotification(const QString &message, const PopupNotificationOptions &options);
    void hideNotification();

private:
    QFrame *m_BackgroundFrame;  // Background frame
    QLabel *m_IconLabel;        // Label for the icon
    QLabel *m_TextLabel;        // Label for the text message
    QHBoxLayout *m_Layout;      // Layout to hold icon and text
    QTimer m_Timer;
    QPropertyAnimation *m_StartAnimation;
    QPropertyAnimation *m_StopAnimation;
    PopupNotificationOptions m_CurrentPopupOptions;
};

class QFloatingIconWindow : public QWidget
{
    Q_OBJECT

public:
    explicit QFloatingIconWindow(QWidget *parent = Q_NULLPTR);
    ~QFloatingIconWindow();

public:
    void showFloatingWindow(const FloatingWindowOptions &options);
    void hideFloatingWindow();
    void updateWindowSettings(const FloatingWindowOptions &options);
    void setMousePassThroughEnabled(bool enabled);

    // Getters for current state
    QPoint getCurrentPosition() const { return m_CurrentOptions.position; }
    QSize getCurrentSize() const { return size(); }
    double getCurrentOpacity() const { return m_CurrentOpacity; }
    FloatingWindowOptions getCurrentOptions() const { return m_CurrentOptions; }

#ifdef DEBUG_LOGOUT_ON
    static QString getReferencePointName(int referencePoint);
#endif

public slots:
    void onWindowPositionChanged(const QPoint &newPosition);
    void onWindowSizeChanged(const QSize &newSize);
    void onWindowOpacityChanged(double newOpacity);
    // void onWindowMousePassThroughChanged(bool enabled);
    void onSwitchFloatingWindowMousePassThrough(void);

signals:
    void windowPositionChanged(const QPoint &newPosition);
    void windowSizeChanged(const QSize &newSize);
    void windowOpacityChanged(double newOpacity);
    // void windowMousePassThroughChanged(bool enabled);
    // void windowSettingsChanged(const FloatingWindowOptions &newOptions);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private:
    void updateWindowStyle();
    void loadIconFromPath(const QString &iconPath);
    void updateIconDisplay();
    void updateCursorForPosition(const QPoint &pos);
    QRect getResizeHandleRect() const;
    bool isInResizeHandle(const QPoint &pos) const;
    void enableMousePassThrough(void);
    void disableMousePassThrough(void);

    // Reference point related helper functions
    bool isWindowReferencePoint(int referencePoint) const;
    QPoint calculateAbsolutePosition(const QPoint &relativePosition, int referencePoint) const;
    QPoint calculateRelativePosition(const QPoint &physicalPosition, int referencePoint) const;
    bool getWindowClientRect(HWND hwnd, QRect &clientRect) const;
    QPoint getScreenReferencePoint(int referencePoint) const;
    QPoint getWindowReferencePoint(int referencePoint, const QRect &clientRect) const;

    // Coordinate conversion helper functions
    QPoint qtToPhysicalCoordinates(const QPoint &qtPoint) const;
    QPoint physicalToQtCoordinates(const QPoint &physicalPoint) const;
    QPoint getPhysicalCursorPosition() const;

public:
    // static method
    static bool isValidReferencePoint(int referencePoint);

    // Public method to update floating window position based on current reference point
    void updatePositionForCurrentWindow();

private:
    QLabel *m_IconLabel;                        // Label to display the icon
    QIcon m_LoadedIcon;                         // Cached icon object
    FloatingWindowOptions m_CurrentOptions;     // Current window options
    double m_CurrentOpacity;                    // Current opacity value
    bool m_MousePassThrough;                    // Flag for mouse pass-through mode

    // Mouse interaction state
    bool m_Dragging;                            // Window dragging state
    bool m_Resizing;                            // Window resizing state
    QPoint m_DragStartPosition;                 // Start position for dragging
    QSize m_ResizeStartSize;                    // Start size for resizing
    QPoint m_ResizeStartMousePos;               // Mouse position when resize started

    // Reference point tracking (for window following)
    HWND m_LastMappingHWND;                     // Last window HWND used for positioning
    QRect m_LastWindowClientRect;               // Last window client rect for position tracking

    static constexpr int RESIZE_HANDLE_SIZE = 8;            // Size of resize handle area
    static constexpr double WHEEL_OPACITY_STEP = 0.02;      // Wheel up&down change opacity step
};

class SystrayMenu : public QMenu
{
    Q_OBJECT

public:
    explicit SystrayMenu(QWidget *parent = Q_NULLPTR) : QMenu(parent)
    , m_MenuItem_Pressed(QKeyMapperConstants::SYSTRAY_MENU_ITEM_PRESSED_NONE)
    {}

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int m_MenuItem_Pressed;
};

class CustomMessageBox : public QDialog {
    Q_OBJECT

public:
    enum IconType {
        Information,
        Warning,
        Critical,
        Question
    };

    CustomMessageBox(QWidget *parent = nullptr, QString message = QString(), QString checkbox_message = QString(), IconType iconType = Warning)
        : QDialog(parent) {
        setWindowTitle(QKeyMapperConstants::PROGRAM_NAME);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // Top Section: Icon and Message
        QHBoxLayout *topLayout = new QHBoxLayout();
        QLabel *iconLabel = new QLabel(this);
        QPixmap iconPixmap;
        switch (iconType) {
        case Information:
            iconPixmap = style()->standardPixmap(QStyle::SP_MessageBoxInformation);
            break;
        case Warning:
            iconPixmap = style()->standardPixmap(QStyle::SP_MessageBoxWarning);
            break;
        case Critical:
            iconPixmap = style()->standardPixmap(QStyle::SP_MessageBoxCritical);
            break;
        case Question:
            iconPixmap = style()->standardPixmap(QStyle::SP_MessageBoxQuestion);
            break;
        }
        iconLabel->setPixmap(iconPixmap.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        topLayout->addWidget(iconLabel);

        // Add a vertical layout for messageLabel and bottom elements
        QVBoxLayout *messageAndControlsLayout = new QVBoxLayout();

        QLabel *messageLabel = new QLabel(message, this);
        messageAndControlsLayout->addWidget(messageLabel);

        // Horizontal layout for CheckBox and OK button
        QHBoxLayout *controlsLayout = new QHBoxLayout();
        checkBox = new QCheckBox(checkbox_message, this);
        controlsLayout->addWidget(checkBox);
        controlsLayout->addStretch(); // Spacer between CheckBox and OK button
        QPushButton *okButton = new QPushButton(tr("OK"), this);
        controlsLayout->addWidget(okButton);

        messageAndControlsLayout->addLayout(controlsLayout);

        topLayout->addLayout(messageAndControlsLayout);
        mainLayout->addLayout(topLayout);

        // Connect the button
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    }

    bool isCheckBoxChecked() const {
        return checkBox->isChecked();
    }

private:
    QCheckBox *checkBox;
};

#if 0
class KeySequenceEditOnlyOne : public QKeySequenceEdit
{
    Q_OBJECT

public:
    explicit KeySequenceEditOnlyOne(QWidget *parent = Q_NULLPTR) :
        QKeySequenceEdit(parent),
        m_DefaultKeySequence(),
        m_LastKeySequence()
    {
    }

    void setDefaultKeySequence(const QString &keysequencestr);
    QString defaultKeySequence();
    void setLastKeySequence(const QString &keysequencestr);
    QString lastKeySequence();

signals:
    void keySeqEditChanged_Signal(const QKeySequence &keysequence);

protected:
    virtual void keyPressEvent(QKeyEvent* pEvent);

private:
    QString m_DefaultKeySequence;
    QString m_LastKeySequence;
};
#endif

class QKeyMapper : public QDialog
{
    Q_OBJECT

public:
    explicit QKeyMapper(QWidget *parent = Q_NULLPTR);
    ~QKeyMapper();

    static QKeyMapper *getInstance()
    {
        return m_instance;
    }

    #define QT_KEY_EXTENDED         (0x01000000U)
    #define QT_KEY_L_SHIFT          (0x00000001U)
    #define QT_KEY_L_CTRL           (0x00000002U)
    #define QT_KEY_L_ALT            (0x00000004U)
    #define QT_KEY_L_WIN            (0x00000008U)
    #define QT_KEY_R_SHIFT          (0x00000010U)
    #define QT_KEY_R_CTRL           (0x00000020U)
    #define QT_KEY_R_ALT            (0x00000040U)
    #define QT_KEY_R_WIN            (0x00000080U)

    // Colors for 50% opacity
    #define MOUSE_L_COLOR           RGB(255, 0, 0)      /* Red */
    #define MOUSE_R_COLOR           RGB(0, 255, 0)      /* Green */
    #define MOUSE_M_COLOR           RGB(255, 255, 0)    /* Yellow */
    #define MOUSE_X1_COLOR          RGB(0, 0, 255)      /* Blue */
    #define MOUSE_X2_COLOR          RGB(255, 0, 255)    /* Purple */
    #define MOUSE_MOVE_COLOR        RGB(0, 210, 211)    /* Jade Dust */
    // #define MOUSE_L_COLOR           RGB(232, 131, 124)
    // #define MOUSE_R_COLOR           RGB(90, 165, 250)
    // #define MOUSE_M_COLOR           RGB(124, 205, 76)
    // #define MOUSE_X1_COLOR          RGB(239, 199, 75)
    // #define MOUSE_X2_COLOR          RGB(153, 138, 249)

    enum KeyMapStatus
    {
        KEYMAP_IDLE = 0U,
        KEYMAP_CHECKING,
        KEYMAP_MAPPING_GLOBAL,
        KEYMAP_MAPPING_MATCHED
    };
    Q_ENUM(KeyMapStatus)

    enum MappingStartMode
    {
        MAPPINGSTART_BUTTONCLICK = 0U,
        MAPPINGSTART_HOTKEY,
        MAPPINGSTART_LOADSETTING
    };
    Q_ENUM(MappingStartMode)

public slots:
    void WindowStateChangedProc(void);
    // void cycleCheckProcessProc(void);
    void matchForegroundWindow(void);
#ifndef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
    void checkGlobalSettingSwitchTimeout(void);
#endif
    void cycleRefreshProcessInfoTableProc(void);
    void updateHWNDListProc(void);

public:
    void changeLanguage(const QString &langCode);
    void setKeyHook(HWND hWnd);
    void setKeyUnHook(void);
    void setKeyMappingRestart(void);
    void startWinEventHook(void);
    void stopWinEventHook(void);

    void setMapProcessInfo(const QString &filename, const QString &windowtitle, const QString &pid, const QString &filepath, const QIcon &windowicon);

    QPushButton* getOriListSelectKeyboardButton(void) const;
    QPushButton* getOriListSelectMouseButton(void) const;
    QPushButton* getOriListSelectGamepadButton(void) const;
    QPushButton* getOriListSelectFunctionButton(void) const;
    QPushButton* getMapListSelectKeyboardButton(void) const;
    QPushButton* getMapListSelectMouseButton(void) const;
    QPushButton* getMapListSelectGamepadButton(void) const;
    QPushButton* getMapListSelectFunctionButton(void) const;

    static QString getExeFileDescription(void);
    static QString getExeProductVersion(void);
    static QString getPlatformString(void);
    static bool IsFilterKeysEnabled(void);

    static void getProcessInfoFromPID(DWORD processID, QString &processPathStr);
    static void getProcessInfoFromHWND(HWND hWnd, QString &processPathStr);
    static QString getProcessPathFromPID(DWORD dwProcessId);
    static HWND getHWND_byPID(DWORD dwProcessID);
    static BOOL CALLBACK enumIconGroupsProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
    static BOOL CALLBACK enumIconsProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam);
    static QIcon extractIconFromExecutable(const QString &filePath, int targetSize = QKeyMapperConstants::DEFAULT_ICON_WIDTH);
    static QIcon extractBestIconFromExecutable(const QString &filePath, int targetSize = QKeyMapperConstants::BEST_ICON_SIZE);
    // static QIcon extractAllBestIconsFromExecutable(const QString &filePath);
    static BOOL IsAltTabWindow(HWND hWnd);
    static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
    // static BOOL CALLBACK EnumChildWindowsProc(HWND hWnd, LPARAM lParam);
    static BOOL DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath);
    static BOOL CALLBACK EnumWindowsBgProc(HWND hWnd, LPARAM lParam);
    static void collectWindowsHWND(const QString& WindowText, HWND hWnd);
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
    static int findOriKeyInKeyMappingDataList(const QString &keyname, QList<MAP_KEYDATA> *keyMappingDataListToCheck = Q_NULLPTR);
    static int findOriKeyInKeyMappingDataList_RemoveMultiInput(const QString &keyname);
    // static int findOriKeyInCertainKeyMappingDataList(const QString &keyname, QList<MAP_KEYDATA> *keyMappingDataListToCheck);
    static int findOriKeyInKeyMappingDataList(const QString &keyname, bool& removemultiinput);
    static int findOriKeyInKeyMappingDataList_ForAddMappingData(const QString &keyname);
    static int findOriKeyInKeyMappingDataList_ForDoublePress(const QString &keyname);
    // static int findOriKeyInKeyMappingDataListGlobal(const QString &keyname);
    static int findMapKeyInKeyMappingDataList(const QString &keyname);
    static ValidationResult validateOriginalKeyString(const QString &originalkeystr, int update_rowindex);
    static ValidationResult validateSingleOriginalKey(const QString &orikey, int update_rowindex);
    static ValidationResult validateSingleOriginalKeyWithoutTimeSuffix(const QString &orikey, int update_rowindex);
    static ValidationResult validateMappingKeyString(const QString &mappingkeystr, const QStringList &mappingkeyseqlist, int update_rowindex);
    static ValidationResult validateSingleMappingKey(const QString &mapkey);
    static bool checkOriginalkeyStr(const QString &originalkeystr);
    static bool checkMappingkeyStr(QString &mappingkeystr);
    static void collectMappingTableTabHotkeys(void);
    static QString getOriginalKeyStringWithoutSuffix(const QString &originalkeystr);
    static void copyStringToClipboard(const QString &string);
    static bool backupFile(const QString &sourceFile, const QString &backupFile);
    static QString escapeSendTextForSaving(const QString &text);
    static QString unescapeSendTextForLoading(const QString &text);

    // unused enum all process function >>>
    // static void EnumProcessFunction(void);
    // unused enum all process function <<<

    static void DrawMousePoints(HWND hwnd, HDC hdc, int showMode);
    static LRESULT CALLBACK MousePointsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void InitializeGDIPlus(void);
    void ShutdownGDIPlus(void);
    void initBlankIconForComboBox(void);
    HWND createTransparentWindow(void);
    void resizeTransparentWindow(HWND hwnd, int x, int y, int width, int height);
    void destroyTransparentWindow(HWND hwnd);
    static void clearTransparentWindow(HWND hwnd, HDC hdc);
    static void DrawCrosshair(HWND hwnd, HDC hdc, int showParam);
    static LRESULT CALLBACK CrosshairWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    HWND createCrosshairWindow(void);
    void resizeCrosshairWindow(HWND hwnd, int x, int y, int width, int height);
    void destroyCrosshairWindow(HWND hwnd);
    static void clearCrosshairWindow(HWND hwnd, HDC hdc);
    static QPoint getMousePointFromLabelString(QString &labelstr);

    static bool getStartupMinimizedStatus(void);
    // static bool getDisableWinKeyStatus(void);
    static int getLanguageIndex(void);
    static int getCurrentSettingSelectIndex(void);
    static bool getKeyMappingDataTableItemBurstStatus(int rowindex);
    static bool getKeyMappingDataTableItemLockStatus(int rowindex);
    static QString getCurrentOriKeyText(void);
    static QString getCurrentOriCombinationKeyText(void);
    static QString getSendTextString(void);
    static void setCurrentOriCombinationKeyText(const QString &newcombinationkeytext);
    static int getMatchProcessNameIndex(void);
    static int getMatchWindowTitleIndex(void);
    // static int getBurstPressTime(void);
    // static int getBurstReleaseTime(void);
    static int getJoystick2MouseSpeedX(void);
    static int getJoystick2MouseSpeedY(void);
    static double getGyro2MouseXSpeed(void);
    static double getGyro2MouseYSpeed(void);
    static double getGyro2MouseMinThreshold(void);
    static double getGyro2MouseMaxThreshold(void);
    static double getGyro2MouseMinXSensitivity(void);
    static double getGyro2MouseMinYSensitivity(void);
    static double getGyro2MouseMaxXSensitivity(void);
    static double getGyro2MouseMaxYSensitivity(void);
    static int getvJoyXSensitivity(void);
    static int getvJoyYSensitivity(void);
    static int getvJoyRecenterTimeout(void);
    static QString getVirtualGamepadType(void);
    static bool getvJoyLockCursorStatus(void);
    static bool getvJoyDirectModeStatus(void);
    static int getGlobalSettingAutoStart(void);
    static bool checkGlobalSettingAutoStart(void);
    static int getDataPortNumber(void);
    static double getBrakeThreshold(void);
    static double getAccelThreshold(void);
    static bool getSendToSameTitleWindowsStatus(void);
    static bool getAcceptVirtualGamepadInputStatus(void);
    static bool getProcessIconAsTrayIconStatus(void);
    static bool isTabTextDuplicate(const QString &tabName);
    static bool isTabTextDuplicateInStringList(const QString &tabName, const QStringList &tabNameList);
    static bool validateCombinationKey(QString &input);
    static int tabIndexToSwitchByTabHotkey(const QString &hotkey_string, bool *isSame = nullptr);
    static bool exportKeyMappingDataToFile(int tabindex, const QString &filename);
    static bool importKeyMappingDataFromFile(int tabindex, const QString &filename);
    static void updateKeyMappingDataListMappingKeys(int rowindex, const QString &mappingkeystr);
    static void updateKeyMappingDataListKeyUpMappingKeys(int rowindex, const QString &mappingkeystr);
    static bool validateSendTimingByKeyMapData(const MAP_KEYDATA &keymapdata);
    static ValidationResult updateWithZipUpdater(const QString &update_filepath);
    static bool isSelectColorDialogVisible(void);

    static QIcon setTabCustomImage(int tabindex, QString &imagepath);
    static void clearTabCustomImage(int tabindex);

signals:
    void HotKeyDisplaySwitchActivated_Signal(const QString &hotkey_string);
    // void HotKeyMappingSwitchActivated_Signal(const QString &hotkey_string);
    void HotKeyMappingStart_Signal(const QString &hotkey_string);
    void HotKeyMappingStop_Signal(const QString &hotkey_string);
    void HotKeyMappingTableSwitchTab_Signal(const QString &hotkey_string);
    void checkOSVersionMatched_Signal(void);
    void checkFilterKeysEnabled_Signal(void);
    void updateLockStatus_Signal(void);
    void updateMousePointLabelDisplay_Signal(const QPoint &point);
    void showMousePoints_Signal(int showpoints_trigger);
    void showCarOrdinal_Signal(qint32 car_ordinal);
    void showCrosshairStart_Signal(int rowindex, const QString &crosshair_keystr);
    void showCrosshairStop_Signal(int rowindex, const QString &crosshair_keystr);
    void switchFloatingWindowMousePassThrough_Signal(void);
#ifdef VIGEM_CLIENT_SUPPORT
    void updateViGEmBusStatus_Signal(void);
    void updateVirtualGamepadListDisplay_Signal(void);
#endif
    void updateMultiInputStatus_Signal(void);
    void updateInputDeviceSelectComboBoxes_Signal(void);
    void updateGamepadSelectComboBox_Signal(int instance_id);
    void keyMappingTableDragDropMove_Signal(int top_row, int bottom_row, int dragged_to);
    void setupDialogClosed_Signal(void);
    void showPopupMessage_Signal(const QString &message, const QString &color, int displayDuration);
    void updateKeyComboBoxWithJoystickKey_Signal(const QString &joystick_keystring);

protected:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

public slots:
    void on_keymapButton_clicked();

    void MappingSwitch(MappingStartMode startmode);

    void checkOSVersionMatched(void);

    void checkFilterKeysEnabled(void);

    void updateLockStatusDisplay(void);

    void updateMousePointLabelDisplay(const QPoint &point);

    void showMousePoints(int showpoints_trigger);

    void showPopupMessage(const QString &message, const QString &color, int displayDuration);

    void showCarOrdinal(qint32 car_ordinal);

    void showCrosshairStart(int rowindex, const QString &crosshair_keystr);

    void showCrosshairStop(int rowindex, const QString &crosshair_keystr);

    void onKeyMappingTabWidgetTabBarDoubleClicked(int index);

    void keyMappingTabWidgetCurrentChanged(int index);

    void onKeyMappingTabWidgetTabOrderChanged(int from, int to);

    void keyMappingTableDragDropMove(int top_row, int bottom_row, int dragged_to);

    void keyMappingTableItemDoubleClicked(QTableWidgetItem *item);

    // void keyMappingTableCellDoubleClicked(int row, int column);

    void setupDialogClosed(void);

    void onUpdateDownloadFinished(const QString &url, const QString &filepath);

#ifdef SINGLE_APPLICATION
    void otherInstancesStarted(void);
#endif

    // void updateShortcutsMap(void);
    // void freeShortcuts(void);
    // void HotKeyForMappingActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers);
    // void HotKeyForMappingReleased(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers);

    // void HotKeyActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers);
    // void HotKeyStartStopActivated(const QString &keyseqstr, const Qt::KeyboardModifiers &modifiers);

    void HotKeyDisplaySwitchActivated(const QString &hotkey_string);
    void HotKeyMappingSwitchActivated(const QString &hotkey_string);
    void HotKeyMappingStart(const QString &hotkey_string);
    void HotKeyMappingStop(const QString &hotkey_string);
    void HotKeyMappingTableSwitchTab(const QString &hotkey_string);
    void switchKeyMappingTabIndex(int index);
    bool addTabToKeyMappingTabWidget(const QString& customTabName = QString());
    bool copyCurrentTabToKeyMappingTabWidget(void);
    int removeTabFromKeyMappingTabWidget(int tabindex);
    void moveTabInKeyMappingTabWidget(int from, int to);
    int copySelectedKeyMappingDataToCopiedList(void);
    int insertKeyMappingDataFromCopiedList(void);
    void updateKeyComboBoxWithJoystickKey(const QString &joystick_keystring);

private slots:
    void onHotKeyLineEditEditingFinished(void);

    // void onWindowSwitchKeySequenceChanged(const QKeySequence &keysequence);
    // void onWindowSwitchKeySequenceEditingFinished();
    // void onMappingSwitchKeySequenceChanged(const QKeySequence &keysequence);
    // void onMappingSwitchKeySequenceEditingFinished();
    // void onOriginalKeySequenceChanged(const QKeySequence &keysequence);
    // void onOriginalKeySequenceEditingFinished();

    void SystrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    void onTrayIconMenuShowHideAction(void);

    void onTrayIconMenuQuitAction(void);

    void cellChanged_slot(int row, int col);

    void OrikeyComboBox_currentTextChangedSlot(const QString &text);

    void MapkeyComboBox_currentTextChangedSlot(const QString &text);

    void on_savemaplistButton_clicked();

    void on_processinfoTable_doubleClicked(const QModelIndex &index);

    void on_addmapdataButton_clicked();

public slots:
    void selectedItemsMoveUp();

    void selectedItemsMoveDown();

    void on_addTabButton_clicked();

    void on_deleteSelectedButton_clicked();

    void on_clearallButton_clicked();

private slots:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void on_settingselectComboBox_textActivated(const QString &text);
#else
    void on_settingselectComboBox_currentTextChanged(const QString &text);
#endif

    void on_removeSettingButton_clicked();

    void on_autoStartupCheckBox_stateChanged(int state);

    void on_languageComboBox_currentIndexChanged(int index);

    void on_enableVirtualJoystickCheckBox_stateChanged(int state);

    void on_installViGEmBusButton_clicked();

    // void on_uninstallViGEmBusButton_clicked();

    // void on_soundEffectCheckBox_stateChanged(int state);

    void on_installInterceptionButton_clicked();

    void on_multiInputDeviceListButton_clicked();

    void on_multiInputEnableCheckBox_stateChanged(int state);

    void on_virtualGamepadNumberSpinBox_valueChanged(int number);

    void on_filterKeysCheckBox_stateChanged(int state);

    void keyMappingTabl_ItemSelectionChanged();

    // void on_startupMinimizedCheckBox_stateChanged(int state);

    void on_autoStartMappingCheckBox_stateChanged(int state);

    void on_processListButton_toggled(bool checked);

    void on_showNotesButton_toggled(bool checked);

    void on_showCategoryButton_toggled(bool checked);

    void on_checkUpdateButton_clicked();

    void on_Gyro2MouseAdvancedSettingButton_clicked();

    void on_selectTrayIconButton_clicked();

    void on_notificationAdvancedSettingButton_clicked();

    void on_oriList_SelectKeyboardButton_toggled(bool checked);

    void on_oriList_SelectMouseButton_toggled(bool checked);

    void on_oriList_SelectGamepadButton_toggled(bool checked);

    void on_oriList_SelectFunctionButton_toggled(bool checked);

    void on_mapList_SelectKeyboardButton_toggled(bool checked);

    void on_mapList_SelectMouseButton_toggled(bool checked);

    void on_mapList_SelectGamepadButton_toggled(bool checked);

    void on_mapList_SelectFunctionButton_toggled(bool checked);

    void on_CategoryFilterComboBox_currentIndexChanged(int index);

    void on_CategoryFilterComboBox_currentTextChanged(const QString &text);

    void on_restoreProcessPathButton_clicked();

    void on_processinfoTable_clicked(const QModelIndex &index);

    void on_keyboardSelectComboBox_currentIndexChanged(int index);

    void on_mouseSelectComboBox_currentIndexChanged(int index);

    void on_gamepadSelectComboBox_currentIndexChanged(int index);

private:
    // Helper methods for saving/restoring category filter state
    QString getCurrentCategoryFilter() const;
    bool isCategoryFilterVisible() const;
    void restoreCategoryFilterState(const QString& filter, bool showState);

    // CategoryFilterStateGuard helper class for RAII-style filter state management
    class CategoryFilterStateGuard {
    public:
        CategoryFilterStateGuard(QKeyMapper* parent);
        ~CategoryFilterStateGuard();
    private:
        QKeyMapper* m_parent;
        QString m_savedFilter;
        bool m_savedShowState;
    };

    // void initHotKeySequence(void);
    void initProcessInfoTable(void);
    void initSysTrayIcon(void);
    void initPopupMessage(void);
    void initPushLevelSlider(void);
    void initWindowInfoMatchComboBoxes(void);
    void updateSysTrayIconMenuText(void);
    void refreshProcessInfoTable(void);
    void setProcessInfoTable(QList<MAP_PROCESSINFO> &processinfolist);
    void updateProcessInfoDisplay(void);
    void showQKeyMapperWindowToTop(void);
    void switchShowHide(void);
    void forceHide(void);
    void hideProcessList(void);
    void showProcessList(void);
    void setKeyMappingTabWidgetWideMode(void);
    void setKeyMappingTabWidgetNarrowMode(void);
    bool isCloseToSystemtray(bool force_showdialog);

public:
    void updateSystemTrayDisplay(void);
    void showInformationPopup(const QString &message);
    void showWarningPopup(const QString &message);
    void showFailurePopup(const QString &message);
    void showNotificationPopup(const QString &message, const PopupNotificationOptions &options);
    void showNotificationPopup(const QString &message);
    void showFloatingIconWindow(const FloatingWindowOptions &options);
    void hideFloatingIconWindow();
    void updateFloatingIconWindow(const FloatingWindowOptions &options);
    FloatingWindowOptions getCurrentFloatingWindowOptions() const;
    void showFloatingIconWindowFromSettings(void);
    void initSelectColorDialog(void);
    void setParentForSelectColorDialog(QWidget *parent);
    bool showMessageBoxWithCheckbox(QWidget *parent, QString message, QString checkbox_message, CustomMessageBox::IconType icontype);
    bool isMappingDataTableFiltered(void);
    void onCategoryFilterChanged(int index);
    void updateCategoryFilterComboBox(void);
    void updateCategoryFilterByShowCategoryState(void);

private:
    void initKeyMappingTabWidget(void);
    void clearKeyMappingTabWidget(void);
    void exitDeleteKeyMappingTabWidget(void);
    void disconnectKeyMappingDataTableConnection(void);
    void updateKeyMappingDataTableConnection(void);
public:
    void resizeKeyMappingDataTableColumnWidth(KeyMappingDataTableWidget *mappingDataTable);
    void resizeAllKeyMappingTabWidgetColumnWidth(void);
private:
    void clearLockStatusDisplay(void);
    void initQSimpleUpdater(void);
    void initKeysCategoryMap(void);
    void initAddKeyComboBoxes(void);
    void initInputDeviceSelectComboBoxes(void);
    void initCategoryFilterControls(void);
    void initKeyboardSelectComboBox(void);
    void initMouseSelectComboBox(void);
    void initWindowSwitchKeyLineEdit(void);
    void initMappingSwitchKeyLineEdit(void);
    void initGyro2MouseSpinBoxes(void);
    // void updateWindowSwitchKeySeq(const QKeySequence &keysequence);
    // void updateMappingSwitchKeySeq(const QKeySequence &keysequence);
    void updateWindowSwitchKeyString(const QString &keystring);
    void updateMappingStartKeyString(const QString &keystring);
    void updateMappingStopKeyString(const QString &keystring);
    // void initOriginalKeySeqEdit(void);
    void initCombinationKeyLineEdit(void);
    void updateOriginalKeyListComboBox(void);
    void updateMappingKeyListComboBox(void);
    void setKeyMappingTabWidgetCurrentIndex(int index);
    void forceSwitchKeyMappingTabWidgetIndex(int index);
public:
    void refreshKeyMappingDataTableByTabIndex(int tabindex);
    void refreshKeyMappingDataTable(KeyMappingDataTableWidget *mappingDataTable, QList<MAP_KEYDATA> *mappingDataList);
    void updateKeyMappingTabWidgetTabDisplay(int tabindex);
private:
    void refreshAllKeyMappingTagWidget(void);
    void updateMousePointsList(void);
    void reloadUILanguage(void);
    void setUILanguage(int languageindex);
    // void setUILanguage_Chinese(void);
    // void setUILanguage_English(void);
    void resetFontSize(void);
    void sessionLockStateChanged(bool locked);

#ifdef SETTINGSFILE_CONVERT
    bool checkSettingsFileNeedtoConvert(void);
    void convertSettingsFile(void);
#endif
    // int checkAutoStartSaveSettings(const QString &executablename, const QString &windowtitle);
    QString matchAutoStartSaveSettings(const QString &processpath, const QString &windowtitle);
    // int checkSaveSettings(const QString &executablename, const QString &windowtitle);
    QString matchSavedSettings(const QString &processpath, const QString &windowtitle);
    bool readSaveSettingData(const QString &group, const QString &key, QVariant &settingdata);

public:
    void saveKeyMapSetting(void);
private:
    QString loadKeyMapSetting(const QString &settingtext);

    void loadFontFile(const QString fontfilename, int &returnback_fontid, QString &fontname);
#ifdef USE_SAOFONT
    void setControlCustomFont(const QString &fontname);
#endif
    void setControlFontEnglish(void);
    void setControlFontChinese(void);
    void setControlFontJapanese(void);

    void changeControlEnableStatus(bool status);

    void extractSoundFiles();
    void playStartSound();
    void playStopSound();
    void mappingStartNotification();
    void mappingStopNotification();
    void mappingTabSwitchNotification(bool isSame);
    void switchToWindowInfoTab(void);

    void closeSelectColorDialog(void);
    void showInputDeviceListWindow(void);
    void closeInputDeviceListWindow(void);
    void showGyro2MouseAdvancedSettingWindow(void);
    void closeGyro2MouseAdvancedSettingDialog(void);
    void showTrayIconSelectDialog(void);
    void closeTrayIconSelectDialog(void);
    void showNotificationSetupDialog(void);
    void closeNotificationSetupDialog(void);
    void showItemSetupDialog(int tabindex, int row);
    void closeItemSetupDialog(void);
    void closeCrosshairSetupDialog(void);
    void closeFloatingWindowSetupDialog(void);

    void showTableSetupDialog(int tabindex);
    void closeTableSetupDialog(void);

    int installInterceptionDriver(void);
    int uninstallInterceptionDriver(void);

#ifdef VIGEM_CLIENT_SUPPORT
    int installViGEmBusDriver(void);
    int uninstallViGEmBusDriver(void);
public slots:
    void updateViGEmBusStatus(void);
    void updateVirtualGamepadListDisplay(void);
    Q_INVOKABLE void reconnectViGEmClient(void);
#endif

public slots:
    void updateMultiInputStatus(void);
    void updateInputDeviceSelectComboBoxes(void);
    void updateGamepadSelectComboBox(int instance_id);
    void updateKeyMappingTabWidgetTabName(int tabindex, const QString &tabname);
    void updateKeyMappingTabInfoHotkey(int tabindex, const QString &tabhotkey);

public:
    static bool s_isDestructing;
    static HWINEVENTHOOK s_WinEventHook;
    static int s_GlobalSettingAutoStart;
#ifdef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
    static uint s_CycleCheckLoopCount;
#endif
    static HWND s_CurrentMappingHWND;
    static QList<MAP_PROCESSINFO> static_ProcessInfoList;
    static QList<HWND> s_hWndList;
    static QList<HWND> s_last_HWNDList;
    static QList<KeyMappingTab_Info> s_KeyMappingTabInfoList;
    static int s_KeyMappingTabWidgetCurrentIndex;
    static int s_KeyMappingTabWidgetLastIndex;
    // static QList<MAP_KEYDATA> KeyMappingDataList;
    static QList<MAP_KEYDATA> *KeyMappingDataList;
    static QList<MAP_KEYDATA> *lastKeyMappingDataList;
    // static QList<MAP_KEYDATA> KeyMappingDataListGlobal;
    static QList<MousePoint_Info> ScreenMousePointsList;
    static QList<MousePoint_Info> WindowMousePointsList;
    // static QHash<QString, QHotkey*> ShortcutsMap;
    static QString s_WindowSwitchKeyString;
    static QString s_MappingStartKeyString;
    static QString s_MappingStopKeyString;
    static QHash<QString, QList<int>> s_MappingTableTabHotkeyMap;
    static qreal s_UI_scale_value;
    static QList<MAP_KEYDATA> s_CopiedMappingData;
    static QHash<int, QStringList> s_OriginalKeysCategoryMap;
    static QHash<int, QStringList> s_MappingKeysCategoryMap;
    static QIcon s_Icon_Blank;
    static int m_UI_Scale;
    KeyMapStatus m_KeyMapStatus;

private:
    static QKeyMapper *m_instance;
    static QString DEFAULT_TITLE;
    Ui::QKeyMapper *ui;
    QPoint m_LastWindowPosition;
#ifdef CYCLECHECKTIMER_ENABLED
    QTimer m_CycleCheckTimer;
#endif
#ifndef USE_CYCLECHECKTIMER_FOR_GLOBAL_SETTING
    QTimer m_CheckGlobalSettingSwitchTimer;
#endif
    QTimer m_ProcessInfoTableRefreshTimer;
public:
    MAP_PROCESSINFO m_MapProcessInfo;
    QSystemTrayIcon *m_SysTrayIcon;
    SystrayMenu *m_SysTrayIconMenu;
    QAction *m_TrayIconMenu_ShowHideAction;
    QAction *m_TrayIconMenu_QuitAction;
private:
    QLabel* m_PopupMessageLabel;
    QPropertyAnimation* m_PopupMessageAnimation;
#ifdef USE_SAOFONT
    int m_SAO_FontFamilyID;
    QString m_SAO_FontName;
#endif
    KeyMappingTabWidget *m_KeyMappingTabWidget;
    KeyMappingDataTableWidget *m_KeyMappingDataTable;
    StyledDelegate *m_ProcessInfoTableDelegate;
    // StyledDelegate *m_KeyMappingDataTableDelegate;
public:
    KeyListComboBox *m_orikeyComboBox;
    KeyListComboBox *m_mapkeyComboBox;
    QColorDialog *m_SelectColorDialog;
    QMap<int, Gamepad_Info> m_GamepadInfoMap;
    QStringList m_SettingSelectListWithoutDescription;
    bool loadSetting_flag;
private:
    // KeySequenceEditOnlyOne *m_windowswitchKeySeqEdit;
    // KeySequenceEditOnlyOne *m_mappingswitchKeySeqEdit;
    // KeySequenceEditOnlyOne *m_originalKeySeqEdit;
    // QHotkey *m_HotKey_ShowHide;
    // QHotkey *m_HotKey_StartStop;
    QTranslator *m_qt_Translator;
#ifdef USE_QTRANSLATOR
    QTranslator *m_custom_Translator;
#endif
    HWND m_TransParentHandle;
    int m_TransParentWindowInitialX;
    int m_TransParentWindowInitialY;
    int m_TransParentWindowInitialWidth;
    int m_TransParentWindowInitialHeight;
    HWND m_CrosshairHandle;
    int m_CrosshairWindowInitialX;
    int m_CrosshairWindowInitialY;
    int m_CrosshairWindowInitialWidth;
    int m_CrosshairWindowInitialHeight;
    ULONG_PTR m_GdiplusToken;
    bool m_TrayMenuQuit = false;
    QInputDeviceListWindow *m_deviceListWindow;
    QGyro2MouseOptionDialog *m_Gyro2MouseOptionDialog;
    QTrayIconSelectDialog *m_TrayIconSelectDialog;
    QNotificationSetupDialog *m_NotificationSetupDialog = Q_NULLPTR;
    QItemSetupDialog *m_ItemSetupDialog;
    QTableSetupDialog *m_TableSetupDialog;
    QPopupNotification *m_PopupNotification;
    QFloatingIconWindow *m_FloatingIconWindow;
};

class ScopedTrayUpdater {
private:
    QKeyMapper* mapper;
public:
    explicit ScopedTrayUpdater(QKeyMapper* m) : mapper(m) {}
    ~ScopedTrayUpdater() {
        if (mapper) {
            mapper->updateSystemTrayDisplay();
        }
    }
};

#endif // QKEYMAPPER_H
