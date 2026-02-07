#include "qkeymapper.h"
#include "qkeyrecord.h"
#include "ui_qkeyrecord.h"

using namespace QKeyMapperConstants;

namespace {

struct ParsedMapKey
{
    QString prefix;
    QString key;
    QString wait;
    bool hasMatch = false;
};

ParsedMapKey parseMapKeyToken(const QString &token)
{
    static const QRegularExpression re(QString::fromUtf8(REGEX_PATTERN_MAPKEY));
    ParsedMapKey parsed;
    const QRegularExpressionMatch match = re.match(token);
    if (!match.hasMatch()) {
        return parsed;
    }

    parsed.hasMatch = true;
    parsed.prefix = match.captured(1);
    parsed.key = match.captured(2);

    const QString waitMin = match.captured(3);
    const QString waitMax = match.captured(4);
    const QString waitSingle = match.captured(5);
    if (!waitMin.isEmpty() && !waitMax.isEmpty()) {
        parsed.wait = QString("(%1~%2)").arg(waitMin, waitMax);
    }
    else if (!waitSingle.isEmpty()) {
        parsed.wait = waitSingle;
    }

    return parsed;
}

QString buildMapKeyToken(const QString &prefix, const QString &key, const QString &wait, bool keepWait)
{
    QString result = prefix + key;
    if (keepWait && !wait.isEmpty()) {
        result += QString::fromUtf8(SEPARATOR_WAITTIME) + wait;
    }
    return result;
}

QString removeWaitTimeFromTokenFallback(const QString &token)
{
    QString result = token;
    const QString waitPrefix = QString::fromUtf8(SEPARATOR_WAITTIME);
    const QRegularExpression waitRe(waitPrefix + QStringLiteral("(?:\\(\\d+~\\d+\\)|\\d+)"));
    result.remove(waitRe);
    return result;
}

bool tryMergeComboSequence(const QStringList &tokens, QString *mergedOutput)
{
    if (!mergedOutput) {
        return false;
    }

    if (tokens.size() < 2 || (tokens.size() % 2) != 0) {
        return false;
    }

    const QString prefixDown = QString::fromUtf8(PREFIX_SEND_DOWN);
    const QString prefixUp = QString::fromUtf8(PREFIX_SEND_UP);
    const QString waitPrefix = QString::fromUtf8(SEPARATOR_WAITTIME);
    const int half = tokens.size() / 2;

    QStringList downKeys;
    downKeys.reserve(half);

    bool hasWaitTimeToken = false;
    for (const QString &token : tokens) {
        if (token.contains(waitPrefix)) {
            hasWaitTimeToken = true;
            break;
        }
    }

#ifdef DEBUG_LOGOUT_ON
    if (hasWaitTimeToken) {
        qDebug().nospace().noquote()
            << "[QKeyRecord::postProcessRecordMappingKeys] "
            << "IgnoreWaitTime enabled but wait time tokens remain in record string.";
    }
#endif

    for (int i = 0; i < tokens.size(); ++i) {
        ParsedMapKey parsed = parseMapKeyToken(tokens.at(i));
        if (!parsed.hasMatch) {
            return false;
        }

        if (i < half) {
            if (parsed.prefix != prefixDown) {
                return false;
            }
            downKeys.append(parsed.key);
        }
        else {
            if (parsed.prefix != prefixUp) {
                return false;
            }
            const int downIndex = tokens.size() - 1 - i;
            if (downIndex < 0 || downIndex >= downKeys.size()) {
                return false;
            }
            if (parsed.key != downKeys.at(downIndex)) {
                return false;
            }
        }
    }

    *mergedOutput = downKeys.join(QString::fromUtf8(SEPARATOR_PLUS));
    return true;
}

QStringList collapseAdjacentDownUp(const QStringList &tokens)
{
    QStringList output;
    output.reserve(tokens.size());

    const QString prefixDown = QString::fromUtf8(PREFIX_SEND_DOWN);
    const QString prefixUp = QString::fromUtf8(PREFIX_SEND_UP);

    for (int i = 0; i < tokens.size(); ++i) {
        const QString currentToken = tokens.at(i);
        ParsedMapKey current = parseMapKeyToken(currentToken);

        if (current.hasMatch && i + 1 < tokens.size()) {
            const QString nextToken = tokens.at(i + 1);
            ParsedMapKey next = parseMapKeyToken(nextToken);

            if (next.hasMatch
                && current.prefix == prefixDown
                && next.prefix == prefixUp
                && current.key == next.key) {
                output.append(current.key);
                ++i;
                continue;
            }
        }

        if (current.hasMatch) {
            output.append(current.prefix + current.key);
        }
        else {
            output.append(removeWaitTimeFromTokenFallback(currentToken));
        }
    }

    return output;
}

bool tryMergeComboSequenceAllowMergedInner(const QStringList &tokens, QString *mergedOutput)
{
    if (!mergedOutput) {
        return false;
    }

    if (tokens.size() < 2) {
        return false;
    }

    const QString prefixDown = QString::fromUtf8(PREFIX_SEND_DOWN);
    const QString prefixUp = QString::fromUtf8(PREFIX_SEND_UP);

    enum Stage {
        StageDown,
        StageInner,
        StageUp
    } stage = StageDown;

    QStringList downKeys;
    QStringList innerKeys;
    QStringList upKeys;

    for (const QString &token : tokens) {
        ParsedMapKey parsed = parseMapKeyToken(token);
        if (!parsed.hasMatch) {
            return false;
        }

        if (parsed.prefix == prefixDown) {
            if (stage != StageDown) {
                return false;
            }
            downKeys.append(parsed.key);
            continue;
        }

        if (parsed.prefix == prefixUp) {
            if (downKeys.isEmpty()) {
                return false;
            }
            stage = StageUp;
            upKeys.append(parsed.key);
            continue;
        }

        if (parsed.prefix.isEmpty()) {
            if (downKeys.isEmpty() || stage == StageUp) {
                return false;
            }
            stage = StageInner;
            innerKeys.append(parsed.key);
            continue;
        }

        return false;
    }

    if (downKeys.isEmpty() || upKeys.isEmpty()) {
        return false;
    }

    if (upKeys.size() != downKeys.size()) {
        return false;
    }

    for (int i = 0; i < upKeys.size(); ++i) {
        const int downIndex = downKeys.size() - 1 - i;
        if (upKeys.at(i) != downKeys.at(downIndex)) {
            return false;
        }
    }

    QStringList mergedKeys = downKeys;
    mergedKeys.append(innerKeys);
    *mergedOutput = mergedKeys.join(QString::fromUtf8(SEPARATOR_PLUS));
    return true;
}

bool tryMergeComboSequenceAtTail(const QStringList &tokens, QString *mergedOutput, int *startIndex)
{
    if (!mergedOutput || !startIndex) {
        return false;
    }

    for (int i = 0; i < tokens.size(); ++i) {
        QString merged;
        const QStringList tailTokens = tokens.mid(i);
        if (tryMergeComboSequenceAllowMergedInner(tailTokens, &merged)) {
            *mergedOutput = merged;
            *startIndex = i;
            return true;
        }
    }

    return false;
}

bool tryConsumeComboSequence(const QStringList &tokens, int startIndex, int *endIndex, QString *mergedOutput)
{
    if (!endIndex || !mergedOutput) {
        return false;
    }

    if (startIndex < 0 || startIndex >= tokens.size()) {
        return false;
    }

    const QString prefixDown = QString::fromUtf8(PREFIX_SEND_DOWN);
    const QString prefixUp = QString::fromUtf8(PREFIX_SEND_UP);

    enum Stage {
        StageDown,
        StageInner,
        StageUp
    } stage = StageDown;

    QStringList downKeys;
    QStringList innerKeys;
    QStringList upKeys;

    for (int i = startIndex; i < tokens.size(); ++i) {
        ParsedMapKey parsed = parseMapKeyToken(tokens.at(i));
        if (!parsed.hasMatch) {
            return false;
        }

        if (parsed.prefix == prefixDown) {
            if (stage != StageDown) {
                return false;
            }
            downKeys.append(parsed.key);
            continue;
        }

        if (parsed.prefix.isEmpty()) {
            if (downKeys.isEmpty() || stage == StageUp) {
                return false;
            }
            stage = StageInner;
            innerKeys.append(parsed.key);
            continue;
        }

        if (parsed.prefix == prefixUp) {
            if (downKeys.isEmpty()) {
                return false;
            }
            stage = StageUp;
            upKeys.append(parsed.key);
            if (upKeys.size() > downKeys.size()) {
                return false;
            }
            if (upKeys.size() == downKeys.size()) {
                for (int j = 0; j < upKeys.size(); ++j) {
                    const int downIndex = downKeys.size() - 1 - j;
                    if (upKeys.at(j) != downKeys.at(downIndex)) {
                        return false;
                    }
                }

                QStringList mergedKeys = downKeys;
                mergedKeys.append(innerKeys);
                *mergedOutput = mergedKeys.join(QString::fromUtf8(SEPARATOR_PLUS));
                *endIndex = i + 1;
                return true;
            }
            continue;
        }

        return false;
    }

    return false;
}

bool mergeComboSequencesInTokens(const QStringList &tokens, QStringList *mergedTokens)
{
    if (!mergedTokens) {
        return false;
    }

    mergedTokens->clear();
    mergedTokens->reserve(tokens.size());

    bool changed = false;
    int i = 0;
    while (i < tokens.size()) {
        int endIndex = -1;
        QString mergedCombo;
        if (tryConsumeComboSequence(tokens, i, &endIndex, &mergedCombo)) {
            mergedTokens->append(mergedCombo);
            i = endIndex;
            changed = true;
            continue;
        }

        mergedTokens->append(tokens.at(i));
        ++i;
    }

    return changed;
}

QString postProcessRecordMappingKeys(const QString &recordString, bool mergeKeyActions, bool ignoreWaitTime)
{
    if (recordString.isEmpty()) {
        return recordString;
    }

    const QStringList tokens = recordString.split(QString::fromUtf8(SEPARATOR_NEXTARROW), QKeyMapperQtCompat::SkipEmptyParts);
    if (tokens.isEmpty()) {
        return recordString;
    }

    const QString prefixDown = QString::fromUtf8(PREFIX_SEND_DOWN);
    const QString prefixUp = QString::fromUtf8(PREFIX_SEND_UP);
    const QString noneKey = QString::fromUtf8(KEY_NONE_STR);

    if (mergeKeyActions && ignoreWaitTime) {
        QString mergedCombo;
        if (tryMergeComboSequence(tokens, &mergedCombo)) {
            return mergedCombo;
        }

        const QStringList collapsedTokens = collapseAdjacentDownUp(tokens);
        if (tryMergeComboSequenceAllowMergedInner(collapsedTokens, &mergedCombo)) {
            return mergedCombo;
        }

        QStringList mergedTokens;
        if (mergeComboSequencesInTokens(collapsedTokens, &mergedTokens)) {
            return mergedTokens.join(QString::fromUtf8(SEPARATOR_NEXTARROW));
        }
    }

    QStringList output;
    output.reserve(tokens.size());

    for (int i = 0; i < tokens.size(); ++i) {
        const QString currentToken = tokens.at(i);
        ParsedMapKey current = parseMapKeyToken(currentToken);

        if (mergeKeyActions && current.hasMatch && i + 1 < tokens.size()) {
            const QString nextToken = tokens.at(i + 1);
            ParsedMapKey next = parseMapKeyToken(nextToken);

            if (next.hasMatch
                && current.prefix == prefixDown
                && next.prefix == prefixUp
                && current.key == next.key) {
                if (ignoreWaitTime) {
                    output.append(current.key);
                }
                else {
                    output.append(buildMapKeyToken(QString(), current.key, current.wait, true));
                    if (!next.wait.isEmpty()) {
                        output.append(buildMapKeyToken(QString(), noneKey, next.wait, true));
                    }
                }
                ++i;
                continue;
            }
        }

        if (current.hasMatch) {
            output.append(buildMapKeyToken(current.prefix, current.key, current.wait, !ignoreWaitTime));
        }
        else if (ignoreWaitTime) {
            output.append(removeWaitTimeFromTokenFallback(currentToken));
        }
        else {
            output.append(currentToken);
        }
    }

    return output.join(QString::fromUtf8(SEPARATOR_NEXTARROW));
}

} // namespace

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
    setWindowTitle(tr("Record Keys"));
    ui->recordStartStopButton->setText(tr("Start Record"));
    ui->mergeKeyActionsCheckBox->setText(tr("Merge key actions"));
    ui->ignoreWaitTimeCheckBox->setText(tr("Ignore waittime"));
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
    ui->mergeKeyActionsCheckBox->setFont(customFont);
    ui->ignoreWaitTimeCheckBox->setFont(customFont);
}

void QKeyRecord::updateKeyRecordLineEdit(bool finished)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QKeyRecord::updateKeyRecordLineEdit]" << "recordMappingKeysList ->" << QKeyMapper_Worker::recordMappingKeysList;
#endif
    if (!QKeyMapper_Worker::recordMappingKeysList.isEmpty()) {
        QString recordMappingKeysString = QKeyMapper_Worker::recordMappingKeysList.join(SEPARATOR_NEXTARROW);
        QString displayRecordMappingKeysString = recordMappingKeysString;

        if (finished) {
            const bool mergeKeyActions = ui->mergeKeyActionsCheckBox->isChecked();
            const bool ignoreWaitTime = ui->ignoreWaitTimeCheckBox->isChecked();
            if (mergeKeyActions || ignoreWaitTime) {
                displayRecordMappingKeysString = postProcessRecordMappingKeys(displayRecordMappingKeysString, mergeKeyActions, ignoreWaitTime);
            }
        }

        ui->keyRecordLineEdit->setText(displayRecordMappingKeysString);

        if (finished) {
            QString convertRecordMappingKeysString = displayRecordMappingKeysString.replace(JOY_KEY_PREFIX, VJOY_KEY_PREFIX);
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
        ui->mergeKeyActionsCheckBox->setEnabled(false);
        ui->ignoreWaitTimeCheckBox->setEnabled(false);
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
        ui->mergeKeyActionsCheckBox->setEnabled(true);
        ui->ignoreWaitTimeCheckBox->setEnabled(true);
        QKeyMapper_Worker::keyRecordStop();
        setKeyRecordLabel(KEYRECORD_STATE_STOP);
        QKeyMapper_Worker::collectRecordKeysList(clicked);
        emit updateKeyRecordLineEdit_Signal(true);
        // close();
    }
}

bool QKeyRecord::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}

void QKeyRecord::showEvent(QShowEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QKeyRecord::showEvent]" << "Show Key Record Dialog";
#endif

    s_KeyRecordDiagShow = true;

    ui->mergeKeyActionsCheckBox->setEnabled(true);
    ui->ignoreWaitTimeCheckBox->setEnabled(true);
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
        ui->mergeKeyActionsCheckBox->setEnabled(true);
        ui->ignoreWaitTimeCheckBox->setEnabled(true);
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
