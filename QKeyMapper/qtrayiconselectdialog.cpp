#include "qtrayiconselectdialog.h"
#include "ui_qtrayiconselectdialog.h"

QTrayIconSelectDialog::QTrayIconSelectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTrayIconSelectDialog)
{
    ui->setupUi(this);
}

QTrayIconSelectDialog::~QTrayIconSelectDialog()
{
    delete ui;
}

void QTrayIconSelectDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Select Systemtray Icon"));

    ui->idleStateTrayIconSelectLabel->setText(tr("Idle"));
    ui->monitingStateTrayIconSelectLabel->setText(tr("Monitoring"));
    ui->globalStateTrayIconSelectLabel->setText(tr("Global"));
    ui->matchedStateTrayIconSelectLabel->setText(tr("Matched"));
}

bool QTrayIconSelectDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}
