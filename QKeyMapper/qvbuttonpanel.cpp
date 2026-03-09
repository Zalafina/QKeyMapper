#include "qvbuttonpanel.h"
#include "qkeymapper_constants.h"
#include "qkeymapper_qt_compat.h"
#include "qkeymapper.h"

#include <QPainter>

using namespace QKeyMapperConstants;

QVButtonPanel::QVButtonPanel(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground, true);  // Required for paintEvent rounded corners

    m_bgColor  = VBTNPANEL_BACKGROUND_COLOR_DEFAULT;
    m_btnColor = VBTNPANEL_BUTTON_COLOR_DEFAULT;
    m_txtColor = VBTNPANEL_TEXT_COLOR_DEFAULT;

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(m_margin, m_margin, m_margin, m_margin);
    m_mainLayout->setSpacing(m_margin);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // Use stylesheet-based transparency so Qt style engine doesn't paint a solid background
    m_scrollArea->setStyleSheet("background: transparent;");
    m_scrollArea->viewport()->setStyleSheet("background: transparent;");

    m_gridContainer = new QWidget(m_scrollArea);
    m_gridContainer->setObjectName("vbtnGridContainer");
    m_gridLayout    = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(m_margin);
    m_gridContainer->setLayout(m_gridLayout);
    // Use named selector to avoid cascading to child QToolButtons
    m_gridContainer->setStyleSheet("QWidget#vbtnGridContainer { background: transparent; }");

    m_scrollArea->setWidget(m_gridContainer);
    m_mainLayout->addWidget(m_scrollArea);
    setLayout(m_mainLayout);

    setMinimumSize(60, 40);
}

QVButtonPanel::~QVButtonPanel()
{
}

// ── Event overrides ─────────────────────────────────────────────────────────

void QVButtonPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd) {
        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        // WS_EX_TOOLWINDOW: hide from taskbar (Qt::Tool already sets this, but ensure it)
        // WS_EX_NOACTIVATE: prevent focus steal on click
        // Remove WS_EX_APPWINDOW explicitly: that flag forces taskbar presence
        exStyle = (exStyle | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
#ifdef VBUTTON_PANEL_DEFENSE
        QKeyMapper_Worker::s_vbutton_panel_hwnd = hwnd;
#endif
    }
}

void QVButtonPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // Draw the background with optional rounded corners
    painter.setBrush(m_bgColor);
    painter.setPen(Qt::NoPen);
    if (m_radius > 0) {
        painter.drawRoundedRect(rect(), m_radius, m_radius);
    } else {
        painter.drawRect(rect());
    }
}

// ── Public API ───────────────────────────────────────────────────────────────

void QVButtonPanel::refreshPanel(const QList<MAP_KEYDATA> &dataList)
{
    // Clear existing buttons
    for (QToolButton *btn : std::as_const(m_buttons)) {
        m_gridLayout->removeWidget(btn);
        btn->deleteLater();
    }
    m_buttons.clear();
    m_keyNames.clear();
    // Note: m_lockedKeyNames is intentionally NOT cleared here.
    // Lock state persists across panel rebuilds and highlights are reapplied below.

    static QRegularExpression vbutton_regex(VBUTTON_REGEX_PATTERN);

    // Collect unique enabled VButton entries in order
    QStringList seen;
    for (const MAP_KEYDATA &entry : dataList) {
        if (!entry.Disabled && vbutton_regex.match(entry.Original_Key).hasMatch()) {
            if (!seen.contains(entry.Original_Key)) {
                seen.append(entry.Original_Key);
            }
        }
    }

    // Build buttons
    int row = 0, col = 0;
    for (int i = 0; i < seen.size(); ++i) {
        const QString &keyName = seen.at(i);
        m_keyNames.append(keyName);

        QToolButton *btn = new QToolButton(m_gridContainer);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setAutoRaise(false);
        btn->setText(extractButtonLabel(keyName));
        btn->setFixedSize(m_btnWidth, m_btnHeight);

        // Tooltip: No. + SendTiming + MappingKey + KeyUpMapping
        int findindex = QKeyMapper::findOriKeyInKeyMappingDataList(keyName);
        if (findindex >= 0 && QKeyMapper::KeyMappingDataList && QKeyMapper::getInstance()) {
            const MAP_KEYDATA &entry = QKeyMapper::KeyMappingDataList->at(findindex);
            QString tip = QString("%1 : %2\n").arg(tr("No."), QString::number(findindex + 1));
            tip += QKeyMapper::getInstance()->makeMappingKeyToolTip(entry);
            if (!entry.Note.isEmpty()) {
                tip += QString("\n%1 : %2").arg(tr("Note"), entry.Note);
            }
            btn->setToolTip(tip);
        }

        // Use pressed/released instead of clicked to separate KEY_DOWN from KEY_UP timing
        int capturedIndex = i;
        connect(btn, &QToolButton::pressed,  this, [this, capturedIndex]() { onVButtonPressed(capturedIndex); });
        connect(btn, &QToolButton::released, this, [this, capturedIndex]() { onVButtonReleased(capturedIndex); });

        m_gridLayout->addWidget(btn, row, col);
        m_buttons.append(btn);

        ++col;
        if (col >= m_columns) { col = 0; ++row; }
    }

    buildGrid();

    // Reapply lock highlights for any keys that were locked before the rebuild
    for (int i = 0; i < m_keyNames.size(); ++i) {
        if (m_lockedKeyNames.contains(m_keyNames.at(i))) {
            m_buttons.at(i)->setStyleSheet(
                "QToolButton { background-color: #3b5587; color: white; }");
        } else {
            m_buttons.at(i)->setStyleSheet(
                QString("QToolButton { background-color: %1; color: %2; }")
                    .arg(m_btnColor.name(QColor::HexArgb), m_txtColor.name(QColor::HexArgb)));
        }
    }
}

void QVButtonPanel::applySettings(int columns, int maxRows, int btnWidth, int btnHeight,
                                   double opacity, bool alwaysOnTop, int margin, int radius, bool dragEnabled)
{
    m_columns     = qMax(1, columns);
    m_maxRows     = qMax(2, maxRows);
    m_btnWidth    = qMax(30, btnWidth);
    m_btnHeight   = qMax(16, btnHeight);
    m_margin      = qBound(0, margin, 50);
    m_radius      = qBound(0, radius, 100);
    m_dragEnabled = dragEnabled;

    // Update layout spacing/margins
    m_mainLayout->setContentsMargins(m_margin, m_margin, m_margin, m_margin);
    m_mainLayout->setSpacing(m_margin);
    m_gridLayout->setSpacing(m_margin);

    setWindowOpacity(qBound(0.0, opacity, 1.0));

    Qt::WindowFlags flags = windowFlags();
    if (alwaysOnTop) flags |=  Qt::WindowStaysOnTopHint;
    else             flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
}

void QVButtonPanel::applyColors(const QColor &bgColor, const QColor &btnColor, const QColor &txtColor)
{
    if (bgColor.isValid())  m_bgColor  = bgColor;
    if (btnColor.isValid()) m_btnColor = btnColor;
    if (txtColor.isValid()) m_txtColor = txtColor;

    update();  // trigger paintEvent to redraw background

    for (int i = 0; i < m_buttons.size(); ++i) {
        if (!m_lockedKeyNames.contains(m_keyNames.at(i))) {
            m_buttons.at(i)->setStyleSheet(
                QString("QToolButton { background-color: %1; color: %2; }")
                    .arg(m_btnColor.name(QColor::HexArgb), m_txtColor.name(QColor::HexArgb)));
        }
    }
}

void QVButtonPanel::applyPosition(int referencePoint, int offsetX, int offsetY)
{
    m_referencePoint = qBound(FLOATINGWINDOW_REFERENCEPOINT_MIN,
                               referencePoint,
                               FLOATINGWINDOW_REFERENCEPOINT_MAX);
    m_offsetX = offsetX;
    m_offsetY = offsetY;

    QPoint origin = calculateReferenceOrigin(m_referencePoint);
    move(origin + QPoint(m_offsetX, m_offsetY));

    // Reset debounce cache so the next updatePositionIfWindowRef() call does a full reposition
    m_lastTrackHWND = nullptr;
    m_lastTrackRect = {};
}

// ── Private slots ────────────────────────────────────────────────────────────

void QVButtonPanel::onVButtonPressed(int index)
{
    if (index < 0 || index >= m_keyNames.size()) return;
    const QString &keyName = m_keyNames.at(index);

    // Worker-side triggerVButtonKey handles all Lock/Burst/Normal logic.
    // Visual lock highlight is updated via onVButtonLockStateChanged (signal from worker).
    emit triggerVButtonKey_Signal(keyName, true);
}

void QVButtonPanel::onVButtonReleased(int index)
{
    if (index < 0 || index >= m_keyNames.size()) return;
    const QString &keyName = m_keyNames.at(index);

    // Lock mode: worker suppresses the KEY_UP while locked; panel just always emits.
    emit triggerVButtonKey_Signal(keyName, false);
}

void QVButtonPanel::onVButtonLockStateChanged(const QString &keyName, bool isLocked)
{
    int idx = m_keyNames.indexOf(keyName);

    // Maintain the tracking set regardless of whether the button is currently visible
    if (isLocked) {
        m_lockedKeyNames.insert(keyName);
    } else {
        m_lockedKeyNames.remove(keyName);
    }

    if (idx < 0 || idx >= m_buttons.size()) return;

    if (isLocked) {
        m_buttons.at(idx)->setStyleSheet(
            "QToolButton { background-color: #3b5587; color: white; }");
    } else {
        m_buttons.at(idx)->setStyleSheet(
            QString("QToolButton { background-color: %1; color: %2; }")
                .arg(m_btnColor.name(QColor::HexArgb), m_txtColor.name(QColor::HexArgb)));
    }
}

void QVButtonPanel::onVButtonClearAllLockStates()
{
    m_lockedKeyNames.clear();
    for (int i = 0; i < m_buttons.size(); ++i) {
        if (!m_lockedKeyNames.contains(m_keyNames.at(i))) {
            m_buttons.at(i)->setStyleSheet(
                QString("QToolButton { background-color: %1; color: %2; }")
                    .arg(m_btnColor.name(QColor::HexArgb), m_txtColor.name(QColor::HexArgb)));
        }
    }
}

void QVButtonPanel::updatePositionIfWindowRef()
{
    if (!isVisible()) return;
    if (m_referencePoint < FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPLEFT) return;  // Screen ref: static, no tracking needed
    if (m_dragging) return;  // Don't reposition while the user is dragging

    HWND targetHwnd = QKeyMapper::s_CurrentMappingHWND;
    if (!targetHwnd) return;

    RECT wr = {};
    if (!GetWindowRect(targetHwnd, &wr)) return;

    // Debounce: skip if same window and same position
    if (targetHwnd == m_lastTrackHWND
        && wr.left   == m_lastTrackRect.left
        && wr.top    == m_lastTrackRect.top
        && wr.right  == m_lastTrackRect.right
        && wr.bottom == m_lastTrackRect.bottom) {
        return;
    }

    m_lastTrackHWND = targetHwnd;
    m_lastTrackRect = wr;

    QPoint newPos = calculateReferenceOrigin(m_referencePoint) + QPoint(m_offsetX, m_offsetY);
    if (newPos != pos()) {
        move(newPos);
    }
}

// ── Private helpers ──────────────────────────────────────────────────────────

void QVButtonPanel::buildGrid()
{
    for (QToolButton *btn : std::as_const(m_buttons))
        btn->setFixedSize(m_btnWidth, m_btnHeight);

    int numBtns   = m_buttons.size();
    int numRows   = (numBtns == 0) ? 1 : ((numBtns + m_columns - 1) / m_columns);
    int visRows   = qMin(numRows, qMax(2, m_maxRows));
    int panelW    = m_columns * (m_btnWidth + m_margin) + m_margin * 2 + 2;
    int panelH    = visRows  * (m_btnHeight + m_margin) + m_margin * 2 + 2;
    setFixedSize(panelW, panelH);

    // Force repaint to apply new radius mask
    update();
}

QString QVButtonPanel::extractButtonLabel(const QString &vbuttonKey) const
{
    static QRegularExpression re(VBUTTON_REGEX_PATTERN);
    QRegularExpressionMatch m = re.match(vbuttonKey);
    return m.hasMatch() ? m.captured(1) : vbuttonKey;
}

QPoint QVButtonPanel::calculateReferenceOrigin(int referencePoint) const
{
    // Screen-based reference points
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);

    switch (referencePoint) {
    case FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPLEFT:
        return QPoint(screenRect.left(), screenRect.top());
    case FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPRIGHT:
        return QPoint(screenRect.right(), screenRect.top());
    case FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPCENTER:
        return QPoint(screenRect.center().x(), screenRect.top());
    case FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMLEFT:
        return QPoint(screenRect.left(), screenRect.bottom());
    case FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMRIGHT:
        return QPoint(screenRect.right(), screenRect.bottom());
    case FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMCENTER:
        return QPoint(screenRect.center().x(), screenRect.bottom());
    default:
        break;
    }

    // Window-based reference points: use s_CurrentMappingHWND
    HWND targetHwnd = QKeyMapper::s_CurrentMappingHWND;
    if (!targetHwnd) {
        // Fall back to screen top-left if no target window
        return QPoint(screenRect.left(), screenRect.top());
    }

    RECT wr = {};
    GetWindowRect(targetHwnd, &wr);
    int wl = wr.left, wt = wr.top, ww = wr.right - wr.left, wh = wr.bottom - wr.top;

    switch (referencePoint) {
    case FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPLEFT:
        return QPoint(wl, wt);
    case FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPRIGHT:
        return QPoint(wl + ww, wt);
    case FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPCENTER:
        return QPoint(wl + ww / 2, wt);
    case FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMLEFT:
        return QPoint(wl, wt + wh);
    case FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMRIGHT:
        return QPoint(wl + ww, wt + wh);
    case FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMCENTER:
        return QPoint(wl + ww / 2, wt + wh);
    default:
        break;
    }
    return QPoint(screenRect.left(), screenRect.top());
}

void QVButtonPanel::recalcOffsets()
{
    QPoint origin = calculateReferenceOrigin(m_referencePoint);
    m_offsetX = pos().x() - origin.x();
    m_offsetY = pos().y() - origin.y();
}

// ── Mouse / context menu ──────────────────────────────────────────────────────

void QVButtonPanel::mousePressEvent(QMouseEvent *event)
{
    if (m_dragEnabled && event->button() == Qt::LeftButton) {
        m_dragging   = true;
        m_dragOffset = event->pos();
    }
    QWidget::mousePressEvent(event);
}

void QVButtonPanel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragEnabled && m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(QKeyMapperQtCompat::mouseEventGlobalPos(event) - m_dragOffset);
    }
    QWidget::mouseMoveEvent(event);
}

void QVButtonPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        // Update offsets so that the timer keeps the panel at the dragged position
        recalcOffsets();
    }
    QWidget::mouseReleaseEvent(event);
}

void QVButtonPanel::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *setupAction = menu.addAction(QObject::tr("VButton Panel Setup"));
    QAction *saveAction  = menu.addAction(QObject::tr("Save Setting"));
    QAction *selected    = menu.exec(event->globalPos());
    if (selected == setupAction) {
        if (QKeyMapper *keyMapper = QKeyMapper::getInstance()) {
            keyMapper->openVButtonPanelSetupDialogFromPanelContextMenu();
        }
    } else if (selected == saveAction) {
        QKeyMapper::getInstance()->saveKeyMapSetting();
    }
}
