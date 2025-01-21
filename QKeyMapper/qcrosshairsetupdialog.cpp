#include "qcrosshairsetupdialog.h"
#include "ui_qcrosshairsetupdialog.h"

QCrosshairSetupDialog::QCrosshairSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QCrosshairSetupDialog)
    , m_CenterColorPicker(new ColorPickerWidget(this))
    , m_CrosshairColorPicker(new ColorPickerWidget(this))
{
    ui->setupUi(this);

    // Set position for the center color picker
    m_CenterColorPicker->move(50, 50);

    // Set position for the crosshair color picker
    m_CrosshairColorPicker->move(50, 150);
}

QCrosshairSetupDialog::~QCrosshairSetupDialog()
{
    delete ui;
}
