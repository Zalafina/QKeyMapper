#include "qkeymapper.h"
#include "qfloatingwindowsetupdialog.h"
#include "ui_qfloatingwindowsetupdialog.h"

using namespace QKeyMapperConstants;

QFloatingWindowSetupDialog *QFloatingWindowSetupDialog::m_instance = Q_NULLPTR;

QFloatingWindowSetupDialog::QFloatingWindowSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QFloatingWindowSetupDialog)
    , m_FloatingWindow_BGColorPicker(new ColorPickerWidget(this, "FW_BGColor", COLORPICKER_BUTTON_WIDTH_FLOATINGWINDOW_BGCOLOR))
{
    m_instance = this;
    ui->setupUi(this);

    int background_color_x = 34;
    int background_color_y = 11;
    // Set position for the background color picker
    m_FloatingWindow_BGColorPicker->move(background_color_x, background_color_y);
    m_FloatingWindow_BGColorPicker->raise();
    m_FloatingWindow_BGColorPicker->setShowAlphaChannel(true);
    m_FloatingWindow_BGColorPicker->setColor(FLOATINGWINDOW_BACKGROUND_COLOR_DEFAULT);

    ui->windowPositionXSpinBox->setRange(FLOATINGWINDOW_POSITION_MIN.x(),
                                         FLOATINGWINDOW_POSITION_MAX.x());
    ui->windowPositionYSpinBox->setRange(FLOATINGWINDOW_POSITION_MIN.y(),
                                         FLOATINGWINDOW_POSITION_MAX.y());
    ui->windowPositionXSpinBox->setValue(FLOATINGWINDOW_POSITION_DEFAULT.x());
    ui->windowPositionYSpinBox->setValue(FLOATINGWINDOW_POSITION_DEFAULT.y());

    ui->windowSizeSpinBox->setRange(FLOATINGWINDOW_SIZE_MIN,
                                     FLOATINGWINDOW_SIZE_MAX);
    ui->windowSizeSpinBox->setValue(FLOATINGWINDOW_SIZE_DEFAULT.width());

    ui->windowRadiusSpinBox->setRange(FLOATINGWINDOW_RADIUS_MIN,
                                    FLOATINGWINDOW_RADIUS_MAX);
    ui->windowRadiusSpinBox->setValue(FLOATINGWINDOW_RADIUS_DEFAULT);

    ui->windowOpacitySpinBox->setRange(FLOATINGWINDOW_OPACITY_MIN,
                                       FLOATINGWINDOW_OPACITY_MAX);
    ui->windowOpacitySpinBox->setDecimals(FLOATINGWINDOW_OPACITY_DECIMALS);
    ui->windowOpacitySpinBox->setSingleStep(FLOATINGWINDOW_OPACITY_SINGLESTEP);
    ui->windowOpacitySpinBox->setValue(FLOATINGWINDOW_OPACITY_DEFAULT);

    QObject::connect(m_FloatingWindow_BGColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingWindowSetupDialog::onBackgroundColorChanged);
}

QFloatingWindowSetupDialog::~QFloatingWindowSetupDialog()
{
    delete ui;
}

void QFloatingWindowSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);

    setWindowTitle(tr("Floating Window Setup"));
    m_FloatingWindow_BGColorPicker->setButtonText(tr("BGColor"));
    m_FloatingWindow_BGColorPicker->setWindowTitle(tr("Select Floating Window Background Color"));
    ui->windowSizeLabel->setText(tr("Size"));
    ui->windowPositionXLabel->setText(tr("Position X"));
    ui->windowPositionYLabel->setText(tr("Position Y"));
    ui->windowRadiusLabel->setText(tr("Radius"));
    ui->windowOpacityLabel->setText(tr("Opacity"));
    ui->mousePassThroughCheckBox->setText(tr("MousePassThrough"));
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
            if (QKeyMapper::isSelectColorDialogVisible()) {
            }
            else {
                close();
            }
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
        QColor WindowBGColor = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).FloatingWindow_BackgroundColor;
        int WindowRadius = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).FloatingWindow_Radius;
        double WindowOpacity = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).FloatingWindow_Opacity;
        bool MousePassThrough = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).FloatingWindow_MousePassThrough;

        if (WindowPosition.isNull()) {
            WindowPosition = FLOATINGWINDOW_POSITION_DEFAULT;
        }

        if (WindowSize.isEmpty()) {
            WindowSize = FLOATINGWINDOW_SIZE_DEFAULT;
        }

        if (!WindowBGColor.isValid()) {
            WindowBGColor = FLOATINGWINDOW_BACKGROUND_COLOR_DEFAULT;
        }

        if (WindowRadius < FLOATINGWINDOW_RADIUS_MIN || WindowRadius > FLOATINGWINDOW_RADIUS_MAX) {
            WindowRadius = FLOATINGWINDOW_RADIUS_DEFAULT;
        }

        if (WindowOpacity < FLOATINGWINDOW_OPACITY_MIN || WindowOpacity > FLOATINGWINDOW_OPACITY_MAX) {
            WindowOpacity = FLOATINGWINDOW_OPACITY_DEFAULT;
        }

        m_FloatingWindow_BGColorPicker->setColor(WindowBGColor);
        ui->windowPositionXSpinBox->setValue(WindowPosition.x());
        ui->windowPositionYSpinBox->setValue(WindowPosition.y());
        ui->windowSizeSpinBox->setValue(WindowSize.width());
        ui->windowOpacitySpinBox->setValue(WindowOpacity);
        ui->windowRadiusSpinBox->setValue(WindowRadius);
        ui->mousePassThroughCheckBox->setChecked(MousePassThrough);
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

void QFloatingWindowSetupDialog::onBackgroundColorChanged(QColor &color)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (color != QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).FloatingWindow_BackgroundColor) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote()
            << "[FloatingWindowSetup]" << " TabIndex[" << m_TabIndex
            << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName
            << "] FloatingWindow Background Color -> " << color.name(QColor::HexArgb)
            << ", Alpha: " << color.alpha();
#endif
        QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].FloatingWindow_BackgroundColor = color;
    }
}

void QFloatingWindowSetupDialog::on_windowSizeSpinBox_valueChanged(int value)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote()
        << "[FloatingWindowSetup]" << " TabIndex[" << m_TabIndex
        << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName
        << "] Floating Window Size value changed -> " << value;
#endif

    QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].FloatingWindow_Size = QSize(value, value);
}


void QFloatingWindowSetupDialog::on_windowPositionXSpinBox_valueChanged(int position_x)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote()
        << "[FloatingWindowSetup]" << " TabIndex[" << m_TabIndex
        << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName
        << "] Floating Window Position X changed -> " << position_x;
#endif

    QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].FloatingWindow_Position.setX(position_x);
}


void QFloatingWindowSetupDialog::on_windowPositionYSpinBox_valueChanged(int position_y)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote()
        << "[FloatingWindowSetup]" << " TabIndex[" << m_TabIndex
        << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName
        << "] Floating Window Position Y changed -> " << position_y;
#endif

    QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].FloatingWindow_Position.setY(position_y);
}


void QFloatingWindowSetupDialog::on_windowOpacitySpinBox_valueChanged(double value)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote()
        << "[FloatingWindowSetup]" << " TabIndex[" << m_TabIndex
        << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName
        << "] Floating Window Opacity value changed -> " << value;
#endif

    QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].FloatingWindow_Opacity = value;
}

void QFloatingWindowSetupDialog::on_mousePassThroughCheckBox_stateChanged(int state)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote()
        << "[FloatingWindowSetup]" << " TabIndex[" << m_TabIndex
        << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName
        << "] Floating Window Mouse PassThrough state changed -> " << (Qt::CheckState)state;
#endif

    if (Qt::Checked == state) {
        QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].FloatingWindow_MousePassThrough = true;
    }
    else {
        QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].FloatingWindow_MousePassThrough = false;
    }
}

void QFloatingWindowSetupDialog::on_windowRadiusSpinBox_valueChanged(int value)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote()
        << "[FloatingWindowSetup]" << " TabIndex[" << m_TabIndex
        << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName
        << "] Floating Window Radius changed -> " << value;
#endif

    QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].FloatingWindow_Radius = value;
}
