#include "qcrosshairsetupdialog.h"
#include "ui_qcrosshairsetupdialog.h"

QCrosshairSetupDialog::QCrosshairSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QCrosshairSetupDialog)
{
    ui->setupUi(this);
}

QCrosshairSetupDialog::~QCrosshairSetupDialog()
{
    delete ui;
}
