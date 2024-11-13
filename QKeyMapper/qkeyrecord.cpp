#include "qkeymapper.h"
#include "qkeyrecord.h"
#include "ui_qkeyrecord.h"
#include "qkeymapper_constants.h"

QKeyRecord *QKeyRecord::m_instance = Q_NULLPTR;

QKeyRecord::QKeyRecord(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QKeyRecord)
{
    m_instance = this;
    ui->setupUi(this);
}

QKeyRecord::~QKeyRecord()
{
    delete ui;
}

void QKeyRecord::showEvent(QShowEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QKeyRecord::showEvent]" << "Show Key Record Dialog";
#endif

    QDialog::showEvent(event);
}

void QKeyRecord::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == KEY_RECORD_STARTSTOP) {
        if (QKeyMapper_Worker::s_KeyRecording) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyRecord::keyPressEvent]" << "\"F12\" Key pressed on key record STOP state, Start Key Record.";
#endif
            QKeyMapper_Worker::keyRecordStop();
            close();
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyRecord::keyPressEvent]" << "\"F12\" Key pressed on key record START state, Stop Key Record.";
#endif
            QKeyMapper_Worker::keyRecordStart();
        }
    }

    QDialog::keyPressEvent(event);
}
