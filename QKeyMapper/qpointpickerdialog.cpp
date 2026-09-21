#include "qpointpickerdialog.h"
#include "qkeymapper.h"
#include "qkeymapper_worker.h"
#include "qkeymapper_constants.h"
#include <QDebug>
#include <QClipboard>
#include <QKeyEvent>

namespace {

const QColor PICKER_THEME_COLOR_DARK(112, 161, 255);
const QColor PICKER_THEME_COLOR_LIGHT(46, 134, 222);

HCURSOR createWin32CrosshairCursor(const QColor &color, qreal dpr)
{
    // Compute physical cursor dimension matching target DPI
    // Windows standard cursor: 32x32 at 100%, 48x48 at 150%, 64x64 at 200%
    const int physicalSize = qMax(32, qRound(32.0 * dpr));
    const int center = physicalSize / 2;

    QImage img(physicalSize, physicalSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal scale = static_cast<qreal>(physicalSize) / 32.0;

    // Use solid crisp pen width aligned with physical scaling
    const qreal penWidth = qMax(1.8, 1.8 * scale);
    QPen pen(color, penWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const qreal radius = 9.0 * scale;
    painter.drawEllipse(QPointF(center, center), radius, radius);

    const qreal innerGap = 3.0 * scale;
    const qreal outerLen = 13.0 * scale;
    painter.drawLine(QPointF(center, center - outerLen), QPointF(center, center - innerGap));
    painter.drawLine(QPointF(center, center + innerGap), QPointF(center, center + outerLen));
    painter.drawLine(QPointF(center - outerLen, center), QPointF(center - innerGap, center));
    painter.drawLine(QPointF(center + innerGap, center), QPointF(center + outerLen, center));

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(center, center), qMax(1.0, 1.2 * scale), qMax(1.0, 1.2 * scale));

    painter.end();

    // Create 32-bit ARGB DIBSection matching QImage format
    BITMAPV5HEADER bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size = sizeof(bi);
    bi.bV5Width = physicalSize;
    bi.bV5Height = -physicalSize; // Top-down DIB
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask   = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask  = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    HDC hdc = GetDC(NULL);
    void *lpBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS, &lpBits, NULL, 0);
    ReleaseDC(NULL, hdc);

    if (!hBitmap || !lpBits) {
        if (hBitmap) {
            DeleteObject(hBitmap);
        }
        return NULL;
    }

    memcpy(lpBits, img.constBits(), physicalSize * physicalSize * 4);

    // Create empty 1-bpp monochrome mask required by CreateIconIndirect
    HBITMAP hMonoMask = CreateBitmap(physicalSize, physicalSize, 1, 1, NULL);

    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = FALSE; // FALSE indicates cursor, TRUE indicates icon
    ii.xHotspot = center;
    ii.yHotspot = center;
    ii.hbmMask = hMonoMask;
    ii.hbmColor = hBitmap;

    HCURSOR hCursor = CreateIconIndirect(&ii);

    DeleteObject(hBitmap);
    DeleteObject(hMonoMask);

    return hCursor;
}

qreal getDprAtPhysicalPoint(const POINT &pt)
{
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (hMon != NULL) {
        typedef HRESULT (WINAPI *GetDpiForMonitorFunc)(HMONITOR, int, UINT*, UINT*);
        static GetDpiForMonitorFunc pGetDpiForMonitor = []() -> GetDpiForMonitorFunc {
            HMODULE hShcore = LoadLibraryW(L"shcore.dll");
            if (hShcore != NULL) {
                return reinterpret_cast<GetDpiForMonitorFunc>(GetProcAddress(hShcore, "GetDpiForMonitor"));
            }
            return nullptr;
        }();

        if (pGetDpiForMonitor != nullptr) {
            UINT dpiX = 96;
            UINT dpiY = 96;
            if (SUCCEEDED(pGetDpiForMonitor(hMon, 0 /* MDT_EFFECTIVE_DPI */, &dpiX, &dpiY)) && dpiX > 0) {
                return static_cast<qreal>(dpiX) / 96.0;
            }
        }
    }

    if (QGuiApplication::primaryScreen() != nullptr) {
        return QGuiApplication::primaryScreen()->devicePixelRatio();
    }
    return 1.0;
}

} // namespace

PointPickerDragTool::PointPickerDragTool(QWidget *parent)
    : QFrame(parent)
{
    setFixedSize(42, 42);
    setCursor(Qt::PointingHandCursor);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);
    setLineWidth(1);
    setFocusPolicy(Qt::NoFocus);
    setTheme(false);
}

PointPickerDragTool::~PointPickerDragTool()
{
    cancelDrag();
    if (m_hNativeCursor != NULL) {
        ::DestroyIcon(m_hNativeCursor);
        m_hNativeCursor = NULL;
    }
}

void PointPickerDragTool::setTheme(bool isDark)
{
    m_isDark = isDark;
    if (isDark) {
        setStyleSheet(QStringLiteral(
            "PointPickerDragTool {"
            "  border: 1px solid rgb(85, 85, 85);"
            "  border-radius: 4px;"
            "  background-color: rgb(45, 45, 45);"
            "}"
            "PointPickerDragTool:hover {"
            "  background-color: rgb(55, 55, 55);"
            "  border-color: rgb(112, 161, 255);"
            "}"
        ));
    } else {
        setStyleSheet(QStringLiteral(
            "PointPickerDragTool {"
            "  border: 1px solid rgb(200, 200, 200);"
            "  border-radius: 4px;"
            "  background-color: rgb(245, 246, 248);"
            "}"
            "PointPickerDragTool:hover {"
            "  background-color: rgb(235, 240, 248);"
            "  border-color: rgb(46, 134, 222);"
            "}"
        ));
    }
    update();
}

void PointPickerDragTool::cancelDrag()
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[PointPickerDragTool::cancelDrag] m_isDragging =" << m_isDragging;
#endif
    finishDrag(false);
}

void PointPickerDragTool::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int cx = width() / 2;
    const int cy = height() / 2;

    QColor crossColor = m_isDark ? PICKER_THEME_COLOR_DARK : PICKER_THEME_COLOR_LIGHT;
    if (m_isDragging) {
        crossColor.setAlpha(110);
    }

    // Outer circle
    QPen pen(crossColor, 1.8);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPoint(cx, cy), 9, 9);

    // Cross lines
    painter.drawLine(cx, cy - 13, cx, cy - 3);
    painter.drawLine(cx, cy + 3, cx, cy + 13);
    painter.drawLine(cx - 13, cy, cx - 3, cy);
    painter.drawLine(cx + 3, cy, cx + 13, cy);

    // Center dot
    painter.setPen(Qt::NoPen);
    painter.setBrush(crossColor);
    painter.drawEllipse(QPoint(cx, cy), 1, 1);
}

void PointPickerDragTool::mousePressEvent(QMouseEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[PointPickerDragTool::mousePressEvent] button =" << event->button()
             << "m_isDragging was =" << m_isDragging
             << "physical primary down ="
             << ((GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON) & 0x8000) != 0);
#endif
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        update();
        QKeyMapper_Worker::s_PickPointDragActive.storeRelease(1);

        POINT pt = {0, 0};
        GetCursorPos(&pt);
        m_currentDpr = getDprAtPhysicalPoint(pt);

        if (m_hNativeCursor != NULL) {
            ::DestroyIcon(m_hNativeCursor);
            m_hNativeCursor = NULL;
        }
        m_hNativeCursor = createWin32CrosshairCursor(m_isDark ? PICKER_THEME_COLOR_DARK : PICKER_THEME_COLOR_LIGHT, m_currentDpr);
        if (m_hNativeCursor != NULL) {
            ::SetCursor(m_hNativeCursor);
        }

        grabMouse();
        emit dragStarted();
        event->accept();
        return;
    } else if (event->button() == Qt::RightButton && m_isDragging) {
        finishDrag(false);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void PointPickerDragTool::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            int vKey = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
            bool isPhysicalDown = ((GetAsyncKeyState(vKey) & 0x8000) != 0);
            if (!isPhysicalDown) {
#if defined(DEBUG_LOGOUT_ON) && defined(POINTPICKER_VERBOSE_LOG)
                qDebug() << "[PointPickerDragTool::mouseMoveEvent] Physical button released outside, finishing drag at pos ("
                         << pt.x << "," << pt.y << ")";
#endif
                finishDrag(true);
                event->accept();
                return;
            }
#if defined(DEBUG_LOGOUT_ON) && defined(POINTPICKER_VERBOSE_LOG)
            qDebug() << "[PointPickerDragTool::mouseMoveEvent] move pos = (" << pt.x << "," << pt.y << ")";
#endif
            qreal currentDpr = getDprAtPhysicalPoint(pt);
            if (currentDpr > 0.0 && !qFuzzyCompare(currentDpr, m_currentDpr)) {
                m_currentDpr = currentDpr;
                if (m_hNativeCursor != NULL) {
                    ::DestroyIcon(m_hNativeCursor);
                }
                m_hNativeCursor = createWin32CrosshairCursor(m_isDark ? PICKER_THEME_COLOR_DARK : PICKER_THEME_COLOR_LIGHT, m_currentDpr);
            }
            if (m_hNativeCursor != NULL) {
                ::SetCursor(m_hNativeCursor);
            }
            emit dragMoved(QPoint(pt.x, pt.y));
        }
        event->accept();
        return;
    }
    QFrame::mouseMoveEvent(event);
}

void PointPickerDragTool::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[PointPickerDragTool::mouseReleaseEvent] button =" << event->button()
             << "m_isDragging =" << m_isDragging
             << "physical primary down ="
             << ((GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON) & 0x8000) != 0);
#endif
    if (m_isDragging && event->button() == Qt::LeftButton) {
        finishDrag(true);
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void PointPickerDragTool::keyPressEvent(QKeyEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[PointPickerDragTool::keyPressEvent] key =" << event->key()
             << "m_isDragging =" << m_isDragging;
#endif
    if (m_isDragging && event->key() == Qt::Key_Escape) {
        finishDrag(false);
        event->accept();
        return;
    }
    QFrame::keyPressEvent(event);
}

void PointPickerDragTool::hideEvent(QHideEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[PointPickerDragTool::hideEvent] m_isDragging =" << m_isDragging;
#endif
    if (m_isDragging) {
        finishDrag(false);
    }
    QFrame::hideEvent(event);
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
bool PointPickerDragTool::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool PointPickerDragTool::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
    MSG *msg = static_cast<MSG *>(message);
    if (msg != nullptr && m_isDragging && m_hNativeCursor != NULL) {
        if (msg->message == WM_SETCURSOR) {
            ::SetCursor(m_hNativeCursor);
            *result = TRUE;
            return true;
        }
#ifdef DEBUG_LOGOUT_ON
        if (msg->message != WM_SETCURSOR) {
            qDebug() << "[PointPickerDragTool::nativeEvent] message = 0x" << QString::number(msg->message, 16)
                     << "wParam =" << msg->wParam << "lParam =" << msg->lParam;
        }
#endif
    }
    return QFrame::nativeEvent(eventType, message, result);
}

void PointPickerDragTool::finishDrag(bool commit)
{
    POINT pt = {0, 0};
    GetCursorPos(&pt);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[PointPickerDragTool::finishDrag] commit =" << commit
             << "m_isDragging was =" << m_isDragging
             << "at pos (" << pt.x << "," << pt.y << ")";
#endif
    if (m_isDragging) {
        m_isDragging = false;
        update();
        releaseMouse();
        if (m_hNativeCursor != NULL) {
            ::DestroyIcon(m_hNativeCursor);
            m_hNativeCursor = NULL;
        }
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        QKeyMapper_Worker::s_PickPointDragActive.storeRelease(0);

        emit dragFinished(commit, QPoint(pt.x, pt.y));
    }
}

QPointPickerDialog::QPointPickerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFont(QFont(QKeyMapperConstants::FONTNAME_ENGLISH, 9));
    setupUi();

    m_coordTimer = new QTimer(this);
    connect(m_coordTimer, &QTimer::timeout, this, &QPointPickerDialog::onUpdateCurrentCoord);

    applyTheme();
    retranslateUi();
}

QPointPickerDialog::~QPointPickerDialog()
{
    if (m_dragTool) {
        m_dragTool->cancelDrag();
    }
    if (m_coordTimer) {
        m_coordTimer->stop();
    }
}

void QPointPickerDialog::setVisible(bool visible)
{
    if (visible && m_hasUserMoved) {
        if (isPositionValidOnScreens(m_lastUserPos, size())) {
            m_isRestoringPos = true;
            move(m_lastUserPos);
            m_isRestoringPos = false;
        } else {
            // Position is invalid on current displays; fallback to default
            m_hasUserMoved = false;
        }
    }
    QDialog::setVisible(visible);
}

bool QPointPickerDialog::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
            return true; // Completely suppress Tab focus navigation
        }
        if (ke->key() == Qt::Key_Escape) {
            if (m_dragTool && m_dragTool->isDragging()) {
                m_dragTool->cancelDrag();
                return true; // Cancel drag without closing the dialog
            }
        }
    }
    return QDialog::event(e);
}

bool QPointPickerDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pickedCoordEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->matches(QKeySequence::Copy) ||
            (ke->key() == Qt::Key_C && (ke->modifiers() & Qt::ControlModifier))) {
            if (!m_pickedCoordEdit->hasSelectedText()) {
                QString coordText = m_pickedCoordEdit->text().trimmed();
                if (!coordText.isEmpty()) {
                    QClipboard *clipboard = QGuiApplication::clipboard();
                    if (clipboard != nullptr) {
                        clipboard->setText(coordText);
                    }
                    if (QKeyMapper::getInstance() != Q_NULLPTR) {
                        QKeyMapper::getInstance()->showInformationPopup(tr("Copied: %1").arg(coordText), this->geometry(), 1500);
                    }
                    return true;
                }
            }
            return false; // If text is selected, pass through to native QLineEdit copy
        }
    }
    return QDialog::eventFilter(watched, event);
}

void QPointPickerDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // 1. First row: Mode radio buttons and Target info label
    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->setSpacing(8);
    m_screenRadio = new QRadioButton(this);
    m_windowRadio = new QRadioButton(this);
    m_screenRadio->setChecked(true);
    m_screenRadio->setFocusPolicy(Qt::NoFocus);
    m_windowRadio->setFocusPolicy(Qt::NoFocus);
    modeLayout->addWidget(m_screenRadio);
    modeLayout->addWidget(m_windowRadio);
    modeLayout->addSpacing(4);

    m_targetInfoLabel = new QLabel(this);
    m_targetInfoLabel->setFocusPolicy(Qt::NoFocus);
    modeLayout->addWidget(m_targetInfoLabel);
    modeLayout->addStretch();
    mainLayout->addLayout(modeLayout);

    connect(m_screenRadio, &QRadioButton::toggled, this, &QPointPickerDialog::onModeChanged);
    connect(m_windowRadio, &QRadioButton::toggled, this, &QPointPickerDialog::onModeChanged);

    // 2. Second row: Left is Sunken Crosshair Drag Tool, Right is Coord Labels
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(8);

    m_dragTool = new PointPickerDragTool(this);
    contentLayout->addWidget(m_dragTool, 0, Qt::AlignBottom);

    QGridLayout *coordLayout = new QGridLayout();
    coordLayout->setSpacing(4);
    coordLayout->setContentsMargins(0, 0, 0, 0);
    coordLayout->setColumnStretch(0, 0);
    coordLayout->setColumnStretch(1, 1);

    m_currentCoordLabel = new QLabel(this);
    m_currentCoordLabel->setFocusPolicy(Qt::NoFocus);
    m_currentCoordLabel->setFixedHeight(16);

    m_currentCoordValueLabel = new QLabel(this);
    m_currentCoordValueLabel->setFocusPolicy(Qt::NoFocus);
    m_currentCoordValueLabel->setFixedHeight(16);
    QFont monoFont(QStringLiteral("Consolas"), 11);
    monoFont.setStyleHint(QFont::Monospace);
    m_currentCoordValueLabel->setFont(monoFont);

    m_pickedCoordLabel = new QLabel(this);
    m_pickedCoordLabel->setFocusPolicy(Qt::NoFocus);

    m_pickedCoordEdit = new QLineEdit(this);
    m_pickedCoordEdit->setReadOnly(true);
    m_pickedCoordEdit->setFocusPolicy(Qt::ClickFocus);
    m_pickedCoordEdit->setFixedHeight(22);
    m_pickedCoordEdit->setFont(monoFont);
    m_pickedCoordEdit->setPlaceholderText(QStringLiteral("0,0"));
    m_pickedCoordEdit->installEventFilter(this);

    coordLayout->addWidget(m_currentCoordLabel, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    coordLayout->addWidget(m_currentCoordValueLabel, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    coordLayout->addWidget(m_pickedCoordLabel, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
    coordLayout->addWidget(m_pickedCoordEdit, 1, 1, Qt::AlignVCenter);

    contentLayout->addLayout(coordLayout);
    mainLayout->addLayout(contentLayout);

    connect(m_dragTool, &PointPickerDragTool::dragStarted, this, &QPointPickerDialog::onDragStarted);
    connect(m_dragTool, &PointPickerDragTool::dragMoved, this, &QPointPickerDialog::onDragMoved);
    connect(m_dragTool, &PointPickerDragTool::dragFinished, this, &QPointPickerDialog::onDragFinished);

    setFixedSize(220, 82);
}

void QPointPickerDialog::retranslateUi()
{
    setWindowTitle(tr("Point Picker"));
    m_screenRadio->setText(tr("Screen"));
    m_windowRadio->setText(tr("Window"));
    m_dragTool->setToolTip(tr("Hold & drag to target then release\n(Right-click to cancel)"));
    m_currentCoordLabel->setText(tr("Current:"));
    m_pickedCoordLabel->setText(tr("Picked:"));
    updateTargetWindowInfo();
    m_lastCoord = QPoint(-99999, -99999);
    onUpdateCurrentCoord();
}

void QPointPickerDialog::onModeChanged()
{
    updateTargetWindowInfo();
    m_lastCoord = QPoint(-99999, -99999);
    onUpdateCurrentCoord();
}

void QPointPickerDialog::updateTargetWindowInfo()
{
    QString fullText;
    if (m_windowRadio->isChecked()) {
        HWND hwnd = QKeyMapper::s_CurrentMappingHWND;
        m_lastTargetHWND = hwnd;
        if (hwnd != NULL && IsWindow(hwnd)) {
            QString processPath;
            QKeyMapper::getProcessInfoFromHWND(hwnd, processPath);
            QString fileName = QFileInfo(processPath).fileName();
            if (fileName.isEmpty() && QKeyMapper::getInstance() != Q_NULLPTR) {
                fileName = QKeyMapper::getInstance()->m_MapProcessInfo.FileName;
            }
            if (fileName.isEmpty()) {
                fileName = QStringLiteral("HWND 0x%1").arg(reinterpret_cast<quintptr>(hwnd), 0, 16);
            }
            fullText = tr("Target: %1").arg(fileName);
        } else {
            fullText = tr("Target: (No matched window)");
        }
    } else {
        m_lastTargetHWND = NULL;
    }

    const int MIN_DIALOG_WIDTH = 220;
    const int MAX_DIALOG_WIDTH = 360;
    const int DIALOG_HEIGHT = 82;

    const int margins = 16; // 8 left + 8 right
    const int radiosWidth = m_screenRadio->sizeHint().width() + m_windowRadio->sizeHint().width() + 8 /*spacing*/ + 4 /*addSpacing*/;

    // Minimum width required by Row 2 (drag tool, labels, coordinate edit)
    const int col0Width = qMax(m_currentCoordLabel->sizeHint().width(), m_pickedCoordLabel->sizeHint().width());
    const int row2Width = margins + 42 /*dragTool*/ + 8 /*spacing*/ + col0Width + 4 /*spacing*/ + 105 /*edit box & coord*/;
    const int baseMinWidth = qMax(MIN_DIALOG_WIDTH, row2Width);

    int targetWidth = baseMinWidth;

    if (fullText.isEmpty()) {
        m_targetInfoLabel->clear();
        m_targetInfoLabel->setToolTip(QString());
    } else {
        QFontMetrics fm(m_targetInfoLabel->font());
        int textWidth = fm.horizontalAdvance(fullText);
        int neededWidth = margins + radiosWidth + 8 /*spacing*/ + textWidth;

        targetWidth = qBound(baseMinWidth, neededWidth, MAX_DIALOG_WIDTH);
        int availableLabelWidth = targetWidth - margins - radiosWidth - 8;

        if (textWidth > availableLabelWidth) {
            QString elidedText = fm.elidedText(fullText, Qt::ElideMiddle, availableLabelWidth);
            m_targetInfoLabel->setText(elidedText);
        } else {
            m_targetInfoLabel->setText(fullText);
        }
        m_targetInfoLabel->setToolTip(fullText);
    }

    if (width() != targetWidth) {
        setFixedSize(targetWidth, DIALOG_HEIGHT);

        // Screen edge guard: prevent overflowing right edge of current monitor
        QScreen *screen = this->screen();
        if (screen == nullptr) {
            screen = QGuiApplication::primaryScreen();
        }
        if (screen != nullptr) {
            QRect avail = screen->availableGeometry();
            if (this->x() + targetWidth > avail.right()) {
                int newX = qMax(avail.left(), avail.right() - targetWidth);
                move(newX, this->y());
            }
        }
        if (m_hasUserMoved && !isMinimized() && !isMaximized()) {
            m_lastUserPos = this->pos();
        }
    }
}

void QPointPickerDialog::onUpdateCurrentCoord()
{
    POINT pt;
    if (!GetCursorPos(&pt)) {
        return;
    }

    // Auto-heal defense: If drag is active but physical mouse button has been released outside
    // (e.g., swallowed by third-party screenshot tools like PixPin or window switching),
    // cleanly finish the drag and restore normal state within 50ms.
    if (m_dragTool != nullptr && m_dragTool->isDragging()) {
        int vKey = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
        bool isDown = ((GetAsyncKeyState(vKey) & 0x8000) != 0);
        if (!isDown) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QPointPickerDialog::onUpdateCurrentCoord] 50ms Timer: physical button released outside, auto-finishing drag at pos ("
                     << pt.x << "," << pt.y << ")";
#endif
            m_dragTool->finishDrag(true);
            return;
        }
    }

    bool isWindowMode = m_windowRadio->isChecked();
    HWND currentTargetHwnd = isWindowMode ? QKeyMapper::s_CurrentMappingHWND : NULL;

    // Detect target window change while in window mode
    if (isWindowMode && currentTargetHwnd != m_lastTargetHWND) {
        updateTargetWindowInfo();
    }

    if (pt.x == m_lastCoord.x() && pt.y == m_lastCoord.y()
        && isWindowMode == m_lastModeWasWindow
        && currentTargetHwnd == m_lastTargetHWND) {
        return;
    }
    m_lastCoord = QPoint(pt.x, pt.y);
    m_lastModeWasWindow = isWindowMode;

    if (isWindowMode) {
        HWND hwnd = QKeyMapper::s_CurrentMappingHWND;
        if (hwnd != NULL && IsWindow(hwnd)) {
            POINT clientPt = pt;
            if (ScreenToClient(hwnd, &clientPt)) {
                m_currentCoordValueLabel->setText(QStringLiteral("X:%1, Y:%2").arg(clientPt.x).arg(clientPt.y));
            } else {
                m_currentCoordValueLabel->setText(QStringLiteral("N/A"));
            }
        } else {
            m_currentCoordValueLabel->setText(QStringLiteral("N/A"));
        }
    } else {
        m_currentCoordValueLabel->setText(QStringLiteral("X:%1, Y:%2").arg(pt.x).arg(pt.y));
    }
}

void QPointPickerDialog::onDragStarted()
{
    // Visual feedback handled by PointPickerDragTool::paintEvent
}

void QPointPickerDialog::onDragMoved(const QPoint &screenPt)
{
    Q_UNUSED(screenPt);
    onUpdateCurrentCoord();
}

void QPointPickerDialog::onDragFinished(bool commit, const QPoint &screenPt)
{
    if (!commit) {
        return;
    }

    bool isWindowMode = m_windowRadio->isChecked();
    QPoint pickedPt;

    if (isWindowMode) {
        HWND hwnd = QKeyMapper::s_CurrentMappingHWND;
        if (hwnd == NULL || !IsWindow(hwnd)) {
            return;
        }
        POINT clientPt;
        clientPt.x = screenPt.x();
        clientPt.y = screenPt.y();
        if (!ScreenToClient(hwnd, &clientPt)) {
            return;
        }
        pickedPt = QPoint(clientPt.x, clientPt.y);
    } else {
        pickedPt = screenPt;
    }

    m_pickedCoordEdit->setText(QString("%1,%2").arg(pickedPt.x()).arg(pickedPt.y()));
    m_pickedCoordEdit->setFocus(Qt::OtherFocusReason);
    emit pointPicked(pickedPt, isWindowMode);
}

void QPointPickerDialog::syncPickedPoint(const QPoint &point)
{
    m_pickedCoordEdit->setText(QString("%1,%2").arg(point.x()).arg(point.y()));
}

void QPointPickerDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (!m_initialShowCompleted) {
        QTimer::singleShot(0, this, [this]() {
            m_initialShowCompleted = true;
        });
    }
    QKeyMapper_Worker::s_point_picker_hwnd = reinterpret_cast<HWND>(winId());
    applyTheme();
    m_coordTimer->start(50);
    updateTargetWindowInfo();

    if (QKeyMapper::getInstance() != Q_NULLPTR) {
        QString labelStr = QKeyMapper::getInstance()->getPointDisplayLabelText();
        QPoint pt = QKeyMapper::getMousePointFromLabelString(labelStr);
        if (pt.x() != -1 || pt.y() != -1) {
            syncPickedPoint(pt);
        }
    }
    emit visibilityChanged(true);
}

void QPointPickerDialog::hideEvent(QHideEvent *event)
{
    if (m_isUserMoving) {
        m_isUserMoving = false;
    }
    if (m_hasUserMoved && !isMinimized() && !isMaximized()) {
        m_lastUserPos = this->pos();
    }
    if (m_dragTool) {
        m_dragTool->cancelDrag();
    }
    if (m_coordTimer) {
        m_coordTimer->stop();
    }
    QKeyMapper_Worker::s_point_picker_hwnd = Q_NULLPTR;
    emit visibilityChanged(false);
    QDialog::hideEvent(event);
}

void QPointPickerDialog::closeEvent(QCloseEvent *event)
{
    if (m_isUserMoving) {
        m_isUserMoving = false;
    }
    if (m_hasUserMoved && !isMinimized() && !isMaximized()) {
        m_lastUserPos = this->pos();
    }
    if (m_dragTool) {
        m_dragTool->cancelDrag();
    }
    if (m_coordTimer) {
        m_coordTimer->stop();
    }
    QKeyMapper_Worker::s_point_picker_hwnd = Q_NULLPTR;
    emit visibilityChanged(false);
    QDialog::closeEvent(event);
}

void QPointPickerDialog::moveEvent(QMoveEvent *event)
{
    QDialog::moveEvent(event);
    if (!m_initialShowCompleted || m_isRestoringPos || !isVisible()) {
        return;
    }
    if (!isMinimized() && !isMaximized()) {
        if (event->pos() != event->oldPos()) {
            m_hasUserMoved = true;
            m_lastUserPos = event->pos();
        }
    }
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
bool QPointPickerDialog::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool QPointPickerDialog::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
    MSG *msg = static_cast<MSG *>(message);
    if (msg != nullptr) {
        if (msg->message == WM_ENTERSIZEMOVE) {
            m_isUserMoving = true;
            m_posBeforeMove = this->pos();
        } else if (msg->message == WM_EXITSIZEMOVE) {
            if (m_isUserMoving) {
                m_isUserMoving = false;
                if (!isMinimized() && !isMaximized()) {
                    QPoint currentPos = this->pos();
                    if (currentPos != m_posBeforeMove) {
                        m_hasUserMoved = true;
                        m_lastUserPos = currentPos;
                    }
                }
            }
        }
#ifdef DEBUG_LOGOUT_ON
        if (msg->message == WM_CAPTURECHANGED) {
            qDebug() << "[QPointPickerDialog::nativeEvent] WM_CAPTURECHANGED received! newCaptureHWND ="
                     << (HWND)msg->lParam << "m_dragTool isDragging ="
                     << (m_dragTool ? m_dragTool->isDragging() : false);
        } else if (msg->message == WM_ACTIVATE) {
            qDebug() << "[QPointPickerDialog::nativeEvent] WM_ACTIVATE received! state ="
                     << LOWORD(msg->wParam) << "otherHWND =" << (HWND)msg->lParam
                     << "m_dragTool isDragging ="
                     << (m_dragTool ? m_dragTool->isDragging() : false);
        }
#endif
    }
    return QDialog::nativeEvent(eventType, message, result);
}

bool QPointPickerDialog::isPositionValidOnScreens(const QPoint &pos, const QSize &size) const
{
    QRect windowRect(pos, size);
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (screen == nullptr) {
            continue;
        }
        QRect availableGeo = screen->availableGeometry();
        QRect intersection = availableGeo.intersected(windowRect);
        if (intersection.width() >= 30 && intersection.height() >= 20) {
            return true;
        }
    }
    return false;
}

void QPointPickerDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void QPointPickerDialog::applyTheme()
{
    bool isDark = false;
    if (QKeyMapper::getInstance() != Q_NULLPTR) {
        int palette = QKeyMapper::getInstance()->getCurrentUIPalette();
        if (palette == QKeyMapperConstants::UI_PALETTE_CUSTOMDARK) {
            isDark = true;
        } else if (palette == QKeyMapperConstants::UI_PALETTE_CUSTOMLIGHT) {
            isDark = false;
        } else if (palette == QKeyMapperConstants::UI_PALETTE_SYSTEMDEFAULT) {
            isDark = QKeyMapper::isWindowsDarkMode();
        }
    } else {
        isDark = QKeyMapper::isWindowsDarkMode();
    }
    applyTheme(isDark);
}

void QPointPickerDialog::applyTheme(bool isDark)
{
    if (m_dragTool != nullptr) {
        m_dragTool->setTheme(isDark);
    }

    if (isDark) {
        m_targetInfoLabel->setStyleSheet(QStringLiteral("color: rgb(176, 176, 176);"));
        setStyleSheet(QStringLiteral(
            "QRadioButton {"
            "  color: rgb(208, 210, 212);"
            "}"
            "QRadioButton::indicator {"
            "  width: 11px;"
            "  height: 11px;"
            "  border-radius: 6px;"
            "  border: 1px solid rgb(120, 120, 120);"
            "  background-color: rgb(60, 60, 60);"
            "  margin-right: 2px;"
            "}"
            "QRadioButton::indicator:hover {"
            "  border: 1px solid rgb(173, 208, 255);"
            "}"
            "QRadioButton::indicator:checked {"
            "  border: 1px solid rgb(112, 161, 255);"
            "  background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,"
            "      stop:0 rgb(112, 161, 255), stop:0.32 rgb(112, 161, 255),"
            "      stop:0.42 transparent, stop:1 transparent);"
            "}"
            "QLineEdit {"
            "  border: 1px solid rgb(108, 108, 108);"
            "  border-radius: 2px;"
            "  background-color: rgb(60, 60, 60);"
            "  color: rgb(208, 210, 212);"
            "  padding: 0 4px;"
            "}"
            "QLineEdit:focus {"
            "  border: 1px solid rgb(112, 161, 255);"
            "}"
        ));
    } else {
        m_targetInfoLabel->setStyleSheet(QStringLiteral("color: rgb(85, 85, 85);"));
        setStyleSheet(QStringLiteral(
            "QRadioButton {"
            "  color: rgb(33, 33, 33);"
            "}"
            "QRadioButton::indicator {"
            "  width: 11px;"
            "  height: 11px;"
            "  border-radius: 6px;"
            "  border: 1px solid rgb(160, 160, 160);"
            "  background-color: rgb(255, 255, 255);"
            "  margin-right: 2px;"
            "}"
            "QRadioButton::indicator:hover {"
            "  border: 1px solid rgb(46, 134, 222);"
            "}"
            "QRadioButton::indicator:checked {"
            "  border: 1px solid rgb(46, 134, 222);"
            "  background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,"
            "      stop:0 rgb(46, 134, 222), stop:0.32 rgb(46, 134, 222),"
            "      stop:0.42 transparent, stop:1 transparent);"
            "}"
            "QLineEdit {"
            "  border: 1px solid rgb(190, 190, 190);"
            "  border-radius: 2px;"
            "  background-color: rgb(255, 255, 255);"
            "  color: rgb(33, 33, 33);"
            "  padding: 0 4px;"
            "}"
            "QLineEdit:focus {"
            "  border: 1px solid rgb(46, 134, 222);"
            "}"
        ));
    }
}
