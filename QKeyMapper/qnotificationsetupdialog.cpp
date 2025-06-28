#include "qnotificationsetupdialog.h"
#include "ui_qnotificationsetupdialog.h"

QNotificationSetupDialog *QNotificationSetupDialog::m_instance = Q_NULLPTR;

QNotificationSetupDialog::QNotificationSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QNotificationSetupDialog)
{
    m_instance = this;
    ui->setupUi(this);
}

QNotificationSetupDialog::~QNotificationSetupDialog()
{
    delete ui;
}

void QNotificationSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Notification Advanced Setting"));
}

bool QNotificationSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}
