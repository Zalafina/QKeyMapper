#include "qkeymapper.h"
#include "qcrosshairsetupdialog.h"
#include "ui_qcrosshairsetupdialog.h"

QCrosshairSetupDialog *QCrosshairSetupDialog::m_instance = Q_NULLPTR;

QCrosshairSetupDialog::QCrosshairSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QCrosshairSetupDialog)
    , m_CenterColorPicker(new ColorPickerWidget(this))
    , m_CrosshairColorPicker(new ColorPickerWidget(this))
{
    m_instance = this;
    ui->setupUi(this);

    QStyle* windowsStyle = QStyleFactory::create("windows");
    ui->centerGroupBox->setStyle(windowsStyle);
    ui->crosshairGroupBox->setStyle(windowsStyle);
    ui->offsetGroupBox->setStyle(windowsStyle);

    int x_offset = 5;
    int y_offset = 10;
    QRect centerWidgetGeometry = ui->centerGroupBox->geometry();
    int center_color_x = centerWidgetGeometry.x();
    int center_color_y = centerWidgetGeometry.y();
    center_color_x += x_offset;
    center_color_y += y_offset;
    // Set position for the center color picker
    m_CenterColorPicker->move(center_color_x, center_color_y);

    QRect crosshairWidgetGeometry = ui->crosshairGroupBox->geometry();
    int crosshair_color_x = crosshairWidgetGeometry.x();
    int crosshair_color_y = crosshairWidgetGeometry.y();
    crosshair_color_x += x_offset;
    crosshair_color_y += y_offset;
    // Set position for the crosshair color picker
    m_CrosshairColorPicker->move(crosshair_color_x, crosshair_color_y);

    m_CenterColorPicker->raise();
    m_CrosshairColorPicker->raise();
}

QCrosshairSetupDialog::~QCrosshairSetupDialog()
{
    delete ui;
}

void QCrosshairSetupDialog::setUILanguage(int languageindex)
{
    m_CenterColorPicker->setUILanguage(languageindex);
    m_CrosshairColorPicker->setUILanguage(languageindex);
}
