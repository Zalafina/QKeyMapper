#include "qitemsetupdialog.h"
#include "ui_qitemsetupdialog.h"

QItemSetupDialog::QItemSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QItemSetupDialog)
    , m_ItemRow(-1)
{
    ui->setupUi(this);
}

QItemSetupDialog::~QItemSetupDialog()
{
    delete ui;
}

void QItemSetupDialog::setItemRow(int row)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::setItemRow]" << "Item Row =" << row;
#endif

    m_ItemRow = row;
}

void QItemSetupDialog::closeEvent(QCloseEvent *event)
{
    m_ItemRow = -1;

    QDialog::closeEvent(event);
}
