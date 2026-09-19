#include "qpointpickerdialog.h"
#include "qkeymapper.h"
#include "qkeymapper_worker.h"
#include "qkeymapper_constants.h"

namespace {

const QColor PICKER_THEME_COLOR_DARK(112, 161, 255);
const QColor PICKER_THEME_COLOR_LIGHT(46, 134, 222);

QCursor createCrosshairCursor(const QColor &color)
{
    const int size = 31;
    const int center = 15;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. Outer high-contrast shadow outline for readability on both light and dark backgrounds
    QPen shadowPen(QColor(0, 0, 0, 200), 2.8);
    painter.setPen(shadowPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPoint(center, center), 7, 7);
    painter.drawLine(center, 1, center, center - 3);
    painter.drawLine(center, center + 3, center, size - 2);
    painter.drawLine(1, center, center - 3, center);
    painter.drawLine(center + 3, center, size - 2, center);

    // 2. Main crosshair lines in specified theme color
    QPen colorPen(color, 1.6);
    painter.setPen(colorPen);
    painter.drawEllipse(QPoint(center, center), 7, 7);
    painter.drawLine(center, 2, center, center - 3);
    painter.drawLine(center, center + 3, center, size - 3);
    painter.drawLine(2, center, center - 3, center);
    painter.drawLine(center + 3, center, size - 3, center);

    // 3. Center pinpoint dot
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPoint(center, center), 1, 1);

    return QCursor(pixmap, center, center);
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
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        update();
        QKeyMapper_Worker::s_PickPointDragActive.storeRelease(1);
        QApplication::setOverrideCursor(createCrosshairCursor(m_isDark ? PICKER_THEME_COLOR_DARK : PICKER_THEME_COLOR_LIGHT));
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
            emit dragMoved(QPoint(pt.x, pt.y));
        }
        event->accept();
        return;
    }
    QFrame::mouseMoveEvent(event);
}

void PointPickerDragTool::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isDragging && event->button() == Qt::LeftButton) {
        finishDrag(true);
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void PointPickerDragTool::keyPressEvent(QKeyEvent *event)
{
    if (m_isDragging && event->key() == Qt::Key_Escape) {
        finishDrag(false);
        event->accept();
        return;
    }
    QFrame::keyPressEvent(event);
}

void PointPickerDragTool::hideEvent(QHideEvent *event)
{
    if (m_isDragging) {
        finishDrag(false);
    }
    QFrame::hideEvent(event);
}

void PointPickerDragTool::finishDrag(bool commit)
{
    if (m_isDragging) {
        m_isDragging = false;
        update();
        releaseMouse();
        QApplication::restoreOverrideCursor();
        QKeyMapper_Worker::s_PickPointDragActive.storeRelease(0);

        POINT pt = {0, 0};
        GetCursorPos(&pt);
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
    QFont monoFont(QStringLiteral("Consolas"), 9);
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

    coordLayout->addWidget(m_currentCoordLabel, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    coordLayout->addWidget(m_currentCoordValueLabel, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    coordLayout->addWidget(m_pickedCoordLabel, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
    coordLayout->addWidget(m_pickedCoordEdit, 1, 1, Qt::AlignVCenter);

    contentLayout->addLayout(coordLayout);
    mainLayout->addLayout(contentLayout);

    connect(m_dragTool, &PointPickerDragTool::dragStarted, this, &QPointPickerDialog::onDragStarted);
    connect(m_dragTool, &PointPickerDragTool::dragMoved, this, &QPointPickerDialog::onDragMoved);
    connect(m_dragTool, &PointPickerDragTool::dragFinished, this, &QPointPickerDialog::onDragFinished);

    setFixedSize(260, 82);
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
    if (!m_windowRadio->isChecked()) {
        m_targetInfoLabel->clear();
        m_lastTargetHWND = NULL;
        return;
    }
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
        m_targetInfoLabel->setText(tr("Target: %1").arg(fileName));
    } else {
        m_targetInfoLabel->setText(tr("Target: (No matched window)"));
    }
}

void QPointPickerDialog::onUpdateCurrentCoord()
{
    POINT pt;
    if (!GetCursorPos(&pt)) {
        return;
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
    emit pointPicked(pickedPt, isWindowMode);
}

void QPointPickerDialog::syncPickedPoint(const QPoint &point)
{
    m_pickedCoordEdit->setText(QString("%1,%2").arg(point.x()).arg(point.y()));
}

void QPointPickerDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
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
            "  border: 1px solid rgb(46, 134, 222);"
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
