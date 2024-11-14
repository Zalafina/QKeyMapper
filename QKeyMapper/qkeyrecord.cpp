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

    ui->keyRecordLineEdit->setFocusPolicy(Qt::NoFocus);

    ui->keyRecordLabel->setFont(QFont(FONTNAME_ENGLISH, 11));
    ui->keyRecordLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));

    QObject::connect(this, &QKeyRecord::updateKeyRecordLineEdit_Signal, this, &QKeyRecord::updateKeyRecordLineEdit);
}

QKeyRecord::~QKeyRecord()
{
    delete ui;
}

void QKeyRecord::updateKeyRecordLineEdit(bool finished)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QKeyRecord::updateKeyRecordLineEdit]" << "recordMappingKeysList ->" << QKeyMapper_Worker::recordMappingKeysList;
#endif
    if (!QKeyMapper_Worker::recordMappingKeysList.isEmpty()) {
        QString recordMappingKeysString = QKeyMapper_Worker::recordMappingKeysList.join(SEPARATOR_NEXTARROW);
        ui->keyRecordLineEdit->setText(recordMappingKeysString);

        if (finished) {
            QKeyMapper::copyStringToClipboard(recordMappingKeysString);
        }
    }
}

void QKeyRecord::procKeyRecordStart()
{
    if (false == QKeyMapper_Worker::s_KeyRecording) {
#ifdef DEBUG_LOGOUT_ON
        QString debugmessage = QString("[QKeyRecord::procKeyRecordStart] \"%1\" Key pressed on key record STOP state, Start Key Record.").arg(KEY_RECORD_START_STR);
        qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
        setKeyRecordLabel(KEYRECORD_STATE_START);
        QKeyMapper_Worker::keyRecordStart();
        ui->keyRecordLineEdit->clear();
    }
}

void QKeyRecord::procKeyRecordStop()
{
    if (QKeyMapper_Worker::s_KeyRecording) {
#ifdef DEBUG_LOGOUT_ON
        QString debugmessage = QString("[QKeyRecord::procKeyRecordStop] \"%1\" Key pressed on key record START state, Stop Key Record.").arg(KEY_RECORD_STOP_STR);
        qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
        QKeyMapper_Worker::keyRecordStop();
        setKeyRecordLabel(KEYRECORD_STATE_STOP);
        QKeyMapper_Worker::collectRecordKeysList();
        emit updateKeyRecordLineEdit_Signal(true);
        // close();
    }
}

void QKeyRecord::showEvent(QShowEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QKeyRecord::showEvent]" << "Show Key Record Dialog";
#endif

    QKeyMapper_Worker::keyRecordStop();
    setKeyRecordLabel(KEYRECORD_STATE_STOP);
    QKeyMapper_Worker::recordKeyList.clear();
    QKeyMapper_Worker::recordMappingKeysList.clear();
    ui->keyRecordLineEdit->clear();

    QDialog::showEvent(event);
}

void QKeyRecord::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    if (QKeyMapper_Worker::s_KeyRecording) {
#ifdef DEBUG_LOGOUT_ON
        QString debugmessage = QString("[QKeyRecord::closeEvent] Key Record Dialog closed on key record START state.");
        qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
        QKeyMapper_Worker::keyRecordStop();
        setKeyRecordLabel(KEYRECORD_STATE_STOP);
        QKeyMapper_Worker::recordKeyList.clear();
        QKeyMapper_Worker::recordMappingKeysList.clear();
        ui->keyRecordLineEdit->clear();
    }
}

void QKeyRecord::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == KEY_RECORD_START) {
        procKeyRecordStart();
    }
    else if (event->key() == KEY_RECORD_STOP) {
        procKeyRecordStop();
    }

    QDialog::keyPressEvent(event);
}

void QKeyRecord::setKeyRecordLabel(KeyRecordState record_state)
{
    if (KEYRECORD_STATE_START == record_state) {
        QString keyrecord_label;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            keyrecord_label = QString("Key recording started, press \"%1\" to stop key recording.").arg(KEY_RECORD_STOP_STR);
        }
        else {
            keyrecord_label = QString("按键录制中, 按\"%1\"键停止按键录制").arg(KEY_RECORD_STOP_STR);
        }
        ui->keyRecordLabel->setText(keyrecord_label);
    }
    else {
        QString keyrecord_label;
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            keyrecord_label = QString("Wait to start key recording, press \"%1\" to start key recording.").arg(KEY_RECORD_START_STR);
        }
        else {
            keyrecord_label = QString("等待开始按键录制, 按\"%1\"键开始按键录制").arg(KEY_RECORD_START_STR);
        }
        ui->keyRecordLabel->setText(keyrecord_label);
    }
}
