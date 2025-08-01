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

    ui->windowPositionXSpinBox->setValue(FLOATINGICONWINDOW_POSITION_DEFAULT.x());
    ui->windowPositionYSpinBox->setValue(FLOATINGICONWINDOW_POSITION_DEFAULT.y());
    ui->windowWidthSpinBox->setValue(FLOATINGICONWINDOW_SIZE_DEFAULT.width());
    ui->windowHeightSpinBox->setValue(FLOATINGICONWINDOW_SIZE_DEFAULT.height());
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
    ui->windowWidthLabel->setText(tr("Width"));
    ui->windowHeightLabel->setText(tr("Height"));
    ui->windowPositionXLabel->setText(tr("X Position"));
    ui->windowPositionYLabel->setText(tr("Y Position"));
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
        ui->windowWidthSpinBox->setValue(WindowSize.width());
        ui->windowHeightSpinBox->setValue(WindowSize.height());
        ui->windowOpacitySpinBox->setValue(WindowOpacity);
    }

    QDialog::showEvent(event);
}
