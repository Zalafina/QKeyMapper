#include "qvbuttonpanel.h"
#include "qkeymapper_constants.h"
#include "qkeymapper_qt_compat.h"
#include "qkeymapper.h"

#include <QPainter>
#include <QScrollBar>

using namespace QKeyMapperConstants;

namespace {
QFont::Weight toVButtonQtFontWeight(int fontWeight)
{
    if (fontWeight == VBTNPANEL_FONT_WEIGHT_LIGHT) {
        return QFont::Light;
    }
    if (fontWeight == VBTNPANEL_FONT_WEIGHT_BOLD) {
        return QFont::Bold;
    }
    return QFont::Normal;
}

QString makeVButtonStyleSheet(const QColor &buttonColor, const QColor &pressedColor,
                              const QColor &lockedColor, const QColor &textColor, bool locked)
{
    // Keep tooltip colors synced with the active theme palette.
    static const QString tooltipRule =
        QStringLiteral("QToolTip { background-color: palette(ToolTipBase); color: palette(ToolTipText); }");

    const QColor visibleColor = locked ? lockedColor : buttonColor;
    return QStringLiteral("QToolButton { background-color: %1; color: %2; }"
                          "QToolButton:pressed { background-color: %3; color: %2; }")
               .arg(visibleColor.name(QColor::HexArgb),
                    textColor.name(QColor::HexArgb),
                    pressedColor.name(QColor::HexArgb)) + tooltipRule;
}
} // namespace

QVButtonPanel::QVButtonPanel(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground, true);  // Required for paintEvent rounded corners

    m_bgColor  = VBTNPANEL_BACKGROUND_COLOR_DEFAULT;
    m_btnColor = VBTNPANEL_BUTTON_COLOR_DEFAULT;
    m_txtColor = VBTNPANEL_TEXT_COLOR_DEFAULT;
    m_pressedColor = VBTNPANEL_PRESSED_COLOR_DEFAULT;
    m_lockedColor = VBTNPANEL_LOCKED_COLOR_DEFAULT;

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(m_margin, m_margin, m_margin, m_margin);
    m_mainLayout->setSpacing(m_margin);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    applyScrollAreaTransparencyStyle();

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

void QVButtonPanel::applyScrollAreaTransparencyStyle()
{
    if (!m_scrollArea || !m_scrollArea->viewport()) {
        return;
    }

    m_scrollArea->setObjectName("vbtnScrollArea");
    m_scrollArea->viewport()->setObjectName("vbtnScrollViewport");
    // Limit transparency rules to the scroll area and viewport only.
    m_scrollArea->setStyleSheet(
        "QScrollArea#vbtnScrollArea { background: transparent; }"
        "QWidget#vbtnScrollViewport { background: transparent; }");
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

    // Recalculate once after the panel becomes visible, so scrollbar-based correction
    // uses stable viewport geometry instead of hidden-state transient values.
    buildGrid();
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
            if (entry.FloatingButton_Enable) {
                continue;
            }
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
        applyButtonFont(btn);

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
        applyButtonStyle(m_buttons.at(i), m_lockedKeyNames.contains(m_keyNames.at(i)));
    }
}

void QVButtonPanel::applySettings(int columns, int maxRows, int btnWidth, int btnHeight,
                                   double opacity, bool alwaysOnTop, int margin, int radius, bool dragEnabled,
                                   int btnFontSize, int btnFontWeight)
{
    m_columns     = qMax(1, columns);
    m_maxRows     = qMax(2, maxRows);
    m_btnWidth    = qMax(30, btnWidth);
    m_btnHeight   = qMax(16, btnHeight);
    m_margin      = qBound(0, margin, 50);
    m_radius      = qBound(0, radius, 100);
    m_dragEnabled = dragEnabled;
    m_btnFontSize = qBound(VBTNPANEL_BTNFONTSIZE_MIN, btnFontSize, VBTNPANEL_BTNFONTSIZE_MAX);
    m_btnFontWeight = qBound(VBTNPANEL_FONT_WEIGHT_MIN, btnFontWeight, VBTNPANEL_FONT_WEIGHT_MAX);

    // Update layout spacing/margins
    m_mainLayout->setContentsMargins(m_margin, m_margin, m_margin, m_margin);
    m_mainLayout->setSpacing(m_margin);
    m_gridLayout->setSpacing(m_margin);

    setWindowOpacity(qBound(0.0, opacity, 1.0));

    Qt::WindowFlags flags = windowFlags();
    if (alwaysOnTop) flags |=  Qt::WindowStaysOnTopHint;
    else             flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    for (QToolButton *btn : std::as_const(m_buttons)) {
        applyButtonFont(btn);
    }
}

void QVButtonPanel::applyColors(const QColor &bgColor, const QColor &btnColor, const QColor &txtColor,
                                const QColor &pressedColor, const QColor &lockedColor)
{
    if (bgColor.isValid())  m_bgColor  = bgColor;
    if (btnColor.isValid()) m_btnColor = btnColor;
    if (txtColor.isValid()) m_txtColor = txtColor;
    if (pressedColor.isValid()) m_pressedColor = pressedColor;
    if (lockedColor.isValid()) m_lockedColor = lockedColor;

    update();  // trigger paintEvent to redraw background

    for (int i = 0; i < m_buttons.size(); ++i) {
        applyButtonStyle(m_buttons.at(i), m_lockedKeyNames.contains(m_keyNames.at(i)));
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

    applyButtonStyle(m_buttons.at(idx), isLocked);
}

void QVButtonPanel::onVButtonClearAllLockStates()
{
    m_lockedKeyNames.clear();
    for (int i = 0; i < m_buttons.size(); ++i) {
        applyButtonStyle(m_buttons.at(i), false);
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

    const int numBtns = m_buttons.size();
    const int numRows = (numBtns == 0) ? 1 : ((numBtns + m_columns - 1) / m_columns);
    const int visRows = qMin(numRows, qMax(2, m_maxRows));
    const int gridSpacing = qMax(0, m_gridLayout ? m_gridLayout->spacing() : m_margin);

    // Grid content size should not include trailing spacing after the last column/row.
    int contentWidth = (m_columns * m_btnWidth) + (qMax(0, m_columns - 1) * gridSpacing);
    int contentHeight = (numRows * m_btnHeight) + (qMax(0, numRows - 1) * gridSpacing);

    if (m_gridLayout) {
        // Ensure sizeHint reflects the latest rebuilt button set before final geometry is decided.
        m_gridLayout->invalidate();
        m_gridLayout->activate();
        const QSize layoutHint = m_gridLayout->sizeHint();
        contentWidth = qMax(contentWidth, layoutHint.width());
        contentHeight = qMax(contentHeight, layoutHint.height());
    }

    if (m_gridContainer) {
        // Keep the grid size deterministic so spacing does not drift during dynamic setting switches.
        m_gridContainer->setFixedSize(contentWidth, contentHeight);
        m_gridContainer->updateGeometry();
    }

    const int visibleContentHeight = (visRows * m_btnHeight) + (qMax(0, visRows - 1) * gridSpacing);
    // Keep no-scrollbar geometry strictly symmetric: content + left/right and top/bottom margins.
    const int basePanelW = contentWidth + m_margin * 2;
    const int basePanelH = visibleContentHeight + m_margin * 2;
    int panelW = basePanelW;
    int panelH = basePanelH;
    bool reservedVSpace = false;
    bool reservedHSpace = false;

    // Reserve vertical scrollbar space so it does not overlap the button columns.
    if (numRows > visRows && m_scrollArea) {
        const int sbExtent = m_scrollArea->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_scrollArea);
        panelW += sbExtent;
        reservedVSpace = true;
    }

    setFixedSize(panelW, panelH);

    if (m_scrollArea && isVisible()) {
        const int sbExtent = m_scrollArea->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_scrollArea);

        // Run corrective passes only when visible; hidden-state geometry can be transient at startup.
        for (int pass = 0; pass < 2; ++pass) {
            m_scrollArea->updateGeometry();
            if (m_gridLayout) {
                m_gridLayout->activate();
            }

            const QScrollBar *vbar = m_scrollArea->verticalScrollBar();
            const QScrollBar *hbar = m_scrollArea->horizontalScrollBar();
            const bool needVSpace = vbar && (vbar->maximum() > vbar->minimum());
            const bool needHSpace = hbar && (hbar->maximum() > hbar->minimum());

            const int desiredW = basePanelW + (needVSpace ? sbExtent : 0);
            const int desiredH = basePanelH + (needHSpace ? sbExtent : 0);

            reservedVSpace = needVSpace;
            reservedHSpace = needHSpace;

            if (desiredW == panelW && desiredH == panelH) {
                break;
            }

            panelW = desiredW;
            panelH = desiredH;
            setFixedSize(panelW, panelH);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    const QMargins mainMargins = m_mainLayout ? m_mainLayout->contentsMargins() : QMargins();
    const QSize scrollAreaSize = m_scrollArea ? m_scrollArea->size() : QSize();
    const QRect scrollAreaGeometry = m_scrollArea ? m_scrollArea->geometry() : QRect();
    const QSize viewportSize = (m_scrollArea && m_scrollArea->viewport()) ? m_scrollArea->viewport()->size() : QSize();
    const QRect viewportGeometry = (m_scrollArea && m_scrollArea->viewport()) ? m_scrollArea->viewport()->geometry() : QRect();
    const QSize gridContainerSize = m_gridContainer ? m_gridContainer->size() : QSize();
    const QRect gridContainerGeometry = m_gridContainer ? m_gridContainer->geometry() : QRect();
    const QScrollBar *vbar = m_scrollArea ? m_scrollArea->verticalScrollBar() : nullptr;
    const QScrollBar *hbar = m_scrollArea ? m_scrollArea->horizontalScrollBar() : nullptr;
    const int frameWidth = m_scrollArea ? m_scrollArea->frameWidth() : 0;
    const int viewportExtraW = viewportSize.width() - gridContainerSize.width();
    const int viewportExtraH = viewportSize.height() - gridContainerSize.height();
    const int outerLeft = scrollAreaGeometry.left();
    const int outerTop = scrollAreaGeometry.top();
    const int outerRight = panelW - (scrollAreaGeometry.x() + scrollAreaGeometry.width());
    const int outerBottom = panelH - (scrollAreaGeometry.y() + scrollAreaGeometry.height());
    const int expectedNoScrollW = contentWidth + mainMargins.left() + mainMargins.right();
    const int expectedNoScrollH = visibleContentHeight + mainMargins.top() + mainMargins.bottom();

    qDebug().nospace() << "[QVButtonPanel::buildGrid]"
                       << " btns=" << numBtns
                       << ", columns=" << m_columns
                       << ", rows=" << numRows
                       << ", visRows=" << visRows
                       << ", btnSize=" << m_btnWidth << "x" << m_btnHeight
                       << ", margin=" << m_margin
                       << ", gridSpacing=" << gridSpacing
                       << ", content=" << contentWidth << "x" << contentHeight
                       << ", panel=" << panelW << "x" << panelH
                       << ", reservedV=" << reservedVSpace
                       << ", reservedH=" << reservedHSpace
                       << ", panelVisible=" << isVisible()
                       << ", mainMargins=" << mainMargins
                       << ", scrollAreaSize=" << scrollAreaSize
                       << ", scrollAreaGeometry=" << scrollAreaGeometry
                       << ", viewportSize=" << viewportSize
                       << ", viewportGeometry=" << viewportGeometry
                       << ", gridContainerSize=" << gridContainerSize
                       << ", gridContainerGeometry=" << gridContainerGeometry
                       << ", viewportExtraW=" << viewportExtraW
                       << ", viewportExtraH=" << viewportExtraH
                       << ", vbarVisible=" << (vbar ? vbar->isVisible() : false)
                       << ", hbarVisible=" << (hbar ? hbar->isVisible() : false)
                       << ", vbarRange=" << (vbar ? vbar->minimum() : 0) << ".." << (vbar ? vbar->maximum() : 0)
                       << ", hbarRange=" << (hbar ? hbar->minimum() : 0) << ".." << (hbar ? hbar->maximum() : 0)
                       << ", frameWidth=" << frameWidth
                       << ", outerMargins=" << outerLeft << "/" << outerTop << "/" << outerRight << "/" << outerBottom
                       << ", expectedNoScrollPanel=" << expectedNoScrollW << "x" << expectedNoScrollH;
#endif

    // Force repaint to apply new radius mask
    update();
}

void QVButtonPanel::applyButtonFont(QToolButton *button)
{
    if (!button) {
        return;
    }

    QFont font = button->font();
    font.setPointSize(m_btnFontSize);
    font.setWeight(toVButtonQtFontWeight(m_btnFontWeight));
    button->setFont(font);
}

void QVButtonPanel::applyButtonStyle(QToolButton *button, bool locked)
{
    if (!button) {
        return;
    }

    button->setStyleSheet(makeVButtonStyleSheet(m_btnColor, m_pressedColor, m_lockedColor, m_txtColor, locked));
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
