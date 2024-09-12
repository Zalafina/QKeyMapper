#include "qtablesetupdialog.h"
#include "ui_qtablesetupdialog.h"

QTableSetupDialog::QTableSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTableSetupDialog)
{
    ui->setupUi(this);
}

QTableSetupDialog::~QTableSetupDialog()
{
    delete ui;
}
