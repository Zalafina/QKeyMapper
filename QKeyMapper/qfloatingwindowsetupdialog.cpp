#include "qkeymapper.h"
// #include "qkeymapper_constants.h"
#include "qfloatingwindowsetupdialog.h"
#include "ui_qfloatingwindowsetupdialog.h"

QFloatingWindowSetupDialog *QFloatingWindowSetupDialog::m_instance = Q_NULLPTR;

QFloatingWindowSetupDialog::QFloatingWindowSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QFloatingWindowSetupDialog)
{
    m_instance = this;
    ui->setupUi(this);

    ui->windowPositionXSpinBox->setRange(FLOATINGICONWINDOW_POSITION_MIN.x(),
                                         FLOATINGICONWINDOW_POSITION_MAX.x());
    ui->windowPositionYSpinBox->setRange(FLOATINGICONWINDOW_POSITION_MIN.y(),
                                         FLOATINGICONWINDOW_POSITION_MAX.y());
    ui->windowPositionXSpinBox->setValue(FLOATINGICONWINDOW_POSITION_DEFAULT.x());
    ui->windowPositionYSpinBox->setValue(FLOATINGICONWINDOW_POSITION_DEFAULT.y());

    ui->windowSizeSpinBox->setRange(FLOATINGICONWINDOW_SIZE_MIN,
                                     FLOATINGICONWINDOW_SIZE_MAX);
    ui->windowSizeSpinBox->setValue(FLOATINGICONWINDOW_SIZE_DEFAULT.width());

    ui->windowOpacitySpinBox->setRange(FLOATINGICONWINDOW_OPACITY_MIN,
                                       FLOATINGICONWINDOW_OPACITY_MAX);
    ui->windowOpacitySpinBox->setDecimals(FLOATINGICONWINDOW_OPACITY_DECIMALS);
    ui->windowOpacitySpinBox->setSingleStep(FLOATINGICONWINDOW_OPACITY_SINGLESTEP);
    ui->windowOpacitySpinBox->setValue(FLOATINGICONWINDOW_OPACITY_DEFAULT);
}

QFloatingWindowSetupDialog::~QFloatingWindowSetupDialog()
{
    delete ui;
}

void QFloatingWindowSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);

    setWindowTitle(tr("Floating Window Setup"));
    ui->windowSizeLabel->setText(tr("Size"));
    ui->windowPositionXLabel->setText(tr("Position X"));
    ui->windowPositionYLabel->setText(tr("Position Y"));
    ui->windowOpacityLabel->setText(tr("Opacity"));
}

void QFloatingWindowSetupDialog::resetFontSize()
{
    QFont customFont = QFont(FONTNAME_ENGLISH, 9);
    this->setFont(customFont);
}

void QFloatingWindowSetupDialog::setTabIndex(int tabindex)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QFloatingWindowSetupDialog::setTabIndex]" << "Tab Index =" << tabindex;
#endif

    m_TabIndex = tabindex;
}

bool QFloatingWindowSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}

void QFloatingWindowSetupDialog::closeEvent(QCloseEvent *event)
{
    m_TabIndex = -1;

    QDialog::closeEvent(event);
}

void QFloatingWindowSetupDialog::showEvent(QShowEvent *event)
{
    if (0 <= m_TabIndex && m_TabIndex < QKeyMapper::s_KeyMappingTabInfoList.size()) {
        QPoint WindowPosition = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).FloatingWindow_Position;
        QSize WindowSize = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).FloatingWindow_Size;
        double WindowOpacity = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).FloatingWindow_Opacity;

        if (WindowPosition.isNull()) {
            WindowPosition = FLOATINGICONWINDOW_POSITION_DEFAULT;
        }

        if (WindowSize.isEmpty()) {
            WindowSize = FLOATINGICONWINDOW_SIZE_DEFAULT;
        }

        if (WindowOpacity < FLOATINGICONWINDOW_OPACITY_MIN || WindowOpacity > FLOATINGICONWINDOW_OPACITY_MAX) {
            WindowOpacity = FLOATINGICONWINDOW_OPACITY_DEFAULT;
        }

        ui->windowPositionXSpinBox->setValue(WindowPosition.x());
        ui->windowPositionYSpinBox->setValue(WindowPosition.y());
        ui->windowSizeSpinBox->setValue(WindowSize.width());
        ui->windowOpacitySpinBox->setValue(WindowOpacity);
    }

    QDialog::showEvent(event);
}

void QFloatingWindowSetupDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

void QFloatingWindowSetupDialog::on_windowSizeSpinBox_valueChanged(int value)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[FloatingWindowSetup] Floating Window Size value changed ->" << value;
#endif

    int tabindex = m_TabIndex;
    QKeyMapper::s_KeyMappingTabInfoList[tabindex].FloatingWindow_Size = QSize(value, value);
}


void QFloatingWindowSetupDialog::on_windowPositionXSpinBox_valueChanged(int position_x)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[FloatingWindowSetup] Floating Window Position X changed ->" << position_x;
#endif

    int tabindex = m_TabIndex;
    QKeyMapper::s_KeyMappingTabInfoList[tabindex].FloatingWindow_Position.setX(position_x);
}


void QFloatingWindowSetupDialog::on_windowPositionYSpinBox_valueChanged(int position_y)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[FloatingWindowSetup] Floating Window Position Y changed ->" << position_y;
#endif

    int tabindex = m_TabIndex;
    QKeyMapper::s_KeyMappingTabInfoList[tabindex].FloatingWindow_Position.setY(position_y);
}


void QFloatingWindowSetupDialog::on_windowOpacitySpinBox_valueChanged(double value)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[FloatingWindowSetup] Floating Window Opacity value changed ->" << value;
#endif

    int tabindex = m_TabIndex;
    QKeyMapper::s_KeyMappingTabInfoList[tabindex].FloatingWindow_Opacity = value;
}
