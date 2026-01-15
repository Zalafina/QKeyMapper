#include "qkeymapper.h"
#include "qkeyrecord.h"
#include "ui_qkeyrecord.h"

using namespace QKeyMapperConstants;

QAtomicBool QKeyRecord::s_KeyRecordDiagShow;
QKeyRecord *QKeyRecord::m_instance = Q_NULLPTR;

QKeyRecord::QKeyRecord(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QKeyRecord)
{
    m_instance = this;
    ui->setupUi(this);

    ui->keyRecordLineEdit->setFocusPolicy(Qt::NoFocus);

    ui->keyRecordLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));

    ui->keyRecordLineEdit->setMaxLength(MAPPINGKEY_LINE_EDIT_MAX_LENGTH);

    QObject::connect(this, &QKeyRecord::updateKeyRecordLineEdit_Signal, this, &QKeyRecord::updateKeyRecordLineEdit, Qt::QueuedConnection);
}

QKeyRecord::~QKeyRecord()
{
    delete ui;
}

void QKeyRecord::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    ui->recordStartStopButton->setText(tr("Start Record"));
}

void QKeyRecord::resetFontSize()
{
    // int scale = QKeyMapper::getInstance()->m_UI_Scale;
    QFont customFont;
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        customFont.setFamily(FONTNAME_ENGLISH);
        customFont.setPointSize(9);
    }
    else if (LANGUAGE_JAPANESE == QKeyMapper::getLanguageIndex()) {
        customFont.setFamily(FONTNAME_ENGLISH);
        customFont.setPointSize(9);
    }
    else {
        customFont.setFamily(FONTNAME_ENGLISH);
        customFont.setPointSize(9);

        // customFont.setFamily(FONTNAME_CHINESE);
        // customFont.setBold(true);

        // if (UI_SCALE_4K_PERCENT_150 == scale) {
        //     customFont.setPointSize(11);
        // }
        // else {
        //     customFont.setPointSize(9);
        // }
    }

    ui->keyRecordLabel->setFont(customFont);
    ui->recordStartStopButton->setFont(customFont);
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
            QString convertRecordMappingKeysString = recordMappingKeysString.replace(JOY_KEY_PREFIX, VJOY_KEY_PREFIX);
            QKeyMapper::copyStringToClipboard(convertRecordMappingKeysString);

            QString popupMessage;
            QString popupMessageColor = SUCCESS_COLOR;
            int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;
            popupMessage = tr("Recorded keys have been copied to the clipboard");
            emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        }
    }
    else {
        ui->keyRecordLineEdit->clear();
    }
}

void QKeyRecord::procKeyRecordStart(bool clicked)
{
    Q_UNUSED(clicked);
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

void QKeyRecord::procKeyRecordStop(bool clicked)
{
    if (QKeyMapper_Worker::s_KeyRecording) {
#ifdef DEBUG_LOGOUT_ON
        QString debugmessage = QString("[QKeyRecord::procKeyRecordStop] \"%1\" Key pressed on key record START state, Stop Key Record.").arg(KEY_RECORD_STOP_STR);
        qDebug().nospace().noquote() << "\033[1;34m" << debugmessage << "\033[0m";
#endif
        if (clicked) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[QKeyRecord::procKeyRecordStop] Stop key record by click.";
#endif
        }
        QKeyMapper_Worker::keyRecordStop();
        setKeyRecordLabel(KEYRECORD_STATE_STOP);
        QKeyMapper_Worker::collectRecordKeysList(clicked);
        emit updateKeyRecordLineEdit_Signal(true);
        // close();
    }
}

void QKeyRecord::showEvent(QShowEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QKeyRecord::showEvent]" << "Show Key Record Dialog";
#endif

    s_KeyRecordDiagShow = true;

    QKeyMapper_Worker::keyRecordStop();
    setKeyRecordLabel(KEYRECORD_STATE_STOP);
    QKeyMapper_Worker::recordKeyList.clear();
    QKeyMapper_Worker::recordMappingKeysList.clear();
    ui->keyRecordLineEdit->clear();

    QDialog::showEvent(event);
}

void QKeyRecord::closeEvent(QCloseEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QKeyRecord::closeEvent]" << "Close Key Record Dialog";
#endif

    s_KeyRecordDiagShow = false;

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
        procKeyRecordStart(false);
    }
    else if (event->key() == KEY_RECORD_STOP) {
        procKeyRecordStop(false);
    }

    QDialog::keyPressEvent(event);
}

void QKeyRecord::setKeyRecordLabel(KeyRecordState record_state)
{
    if (KEYRECORD_STATE_START == record_state) {
        QString keyrecord_label = tr("Key recording started, press \"%1\" to stop key recording").arg(KEY_RECORD_STOP_STR);
        ui->keyRecordLabel->setText(keyrecord_label);

        ui->recordStartStopButton->setText(tr("Stop Record"));
    }
    else {
        QString keyrecord_label = tr("Wait to start key recording, press \"%1\" to start key recording").arg(KEY_RECORD_START_STR);
        ui->keyRecordLabel->setText(keyrecord_label);

        ui->recordStartStopButton->setText(tr("Start Record"));
    }
}

void QKeyRecord::on_recordStartStopButton_clicked()
{
    if (QKeyMapper_Worker::s_KeyRecording) {
        procKeyRecordStop(true);
    }
    else {
        procKeyRecordStart(true);
    }
}
