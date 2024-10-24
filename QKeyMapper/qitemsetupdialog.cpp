#include "qkeymapper.h"
#include "qitemsetupdialog.h"
#include "ui_qitemsetupdialog.h"
#include "qkeymapper_constants.h"

QItemSetupDialog *QItemSetupDialog::m_instance = Q_NULLPTR;
QStringList QItemSetupDialog::s_valiedOriginalKeyList;
QStringList QItemSetupDialog::s_valiedMappingKeyList;

QItemSetupDialog::QItemSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QItemSetupDialog)
    , m_TabIndex(-1)
    , m_ItemRow(-1)
    , m_OriginalKeyListComboBox(new KeyListComboBox(this))
    , m_MappingKeyListComboBox(new KeyListComboBox(this))
{
    m_instance = this;
    ui->setupUi(this);

    initKeyListComboBoxes();

    ui->originalKeyLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->mappingKeyLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->itemNoteLineEdit->setFocusPolicy(Qt::ClickFocus);

    ui->originalKeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->mappingKeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->itemNoteLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->burstpressSpinBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->burstreleaseSpinBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->repeatTimesSpinBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    m_OriginalKeyListComboBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    m_MappingKeyListComboBox->setFont(QFont(FONTNAME_ENGLISH, 9));

    ui->burstpressSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->burstreleaseSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->repeatTimesSpinBox->setRange(REPEAT_TIMES_MIN, REPEAT_TIMES_MAX);

    // ui->originalKeyLineEdit->setReadOnly(true);
    // ui->mappingKeyLineEdit->setReadOnly(true);
    // ui->originalKeyUpdateButton->setVisible(false);
    // ui->mappingKeyUpdateButton->setVisible(false);

    QObject::connect(ui->originalKeyLineEdit, &QLineEdit::returnPressed, this, &QItemSetupDialog::on_originalKeyUpdateButton_clicked);
    QObject::connect(ui->mappingKeyLineEdit, &QLineEdit::returnPressed, this, &QItemSetupDialog::on_mappingKeyUpdateButton_clicked);
    QObject::connect(ui->itemNoteLineEdit, &QLineEdit::returnPressed, this, &QItemSetupDialog::on_itemNoteUpdateButton_clicked);
}

QItemSetupDialog::~QItemSetupDialog()
{
    delete ui;
}

void QItemSetupDialog::setUILanguagee(int languageindex)
{
    if (LANGUAGE_ENGLISH == languageindex) {
        setWindowTitle(ITEMSETUPDIALOG_WINDOWTITLE_ENGLISH);

        ui->burstCheckBox->setText(BURSTCHECKBOX_ENGLISH);
        ui->lockCheckBox->setText(LOCKCHECKBOX_ENGLISH);
        ui->keyupActionCheckBox->setText(KEYUPACTIONCHECKBOX_ENGLISH);
        ui->passThroughCheckBox->setText(PASSTHROUGHCHECKBOX_ENGLISH);
        ui->keySeqHoldDownCheckBox->setText(KEYSEQHOLDDOWNCHECKBOX_ENGLISH);
        ui->repeatByKeyCheckBox->setText(REPEATBYKEYCHECKBOX_ENGLISH);
        ui->repeatByTimesCheckBox->setText(REPEATBYTIMESCHECKBOX_ENGLISH);
        ui->burstpressLabel->setText(BURSTPRESSLABEL_ENGLISH);
        ui->burstreleaseLabel->setText(BURSTRELEASE_ENGLISH);
        ui->originalKeyLabel->setText(ORIGINALKEYLABEL_ENGLISH);
        ui->mappingKeyLabel->setText(MAPPINGKEYLABEL_ENGLISH);
        ui->itemNoteLabel->setText(ITEMNOTELABEL_ENGLISH);
        ui->orikeyListLabel->setText(ORIKEYLISTLABEL_ENGLISH);
        ui->mapkeyListLabel->setText(MAPKEYLISTLABEL_ENGLISH);
        ui->originalKeyUpdateButton->setText(UPDATEBUTTON_ENGLISH);
        ui->mappingKeyUpdateButton->setText(UPDATEBUTTON_ENGLISH);
        ui->itemNoteUpdateButton->setText(UPDATEBUTTON_ENGLISH);
    }
    else {
        setWindowTitle(ITEMSETUPDIALOG_WINDOWTITLE_CHINESE);

        ui->burstCheckBox->setText(BURSTCHECKBOX_CHINESE);
        ui->lockCheckBox->setText(LOCKCHECKBOX_CHINESE);
        ui->keyupActionCheckBox->setText(KEYUPACTIONCHECKBOX_CHINESE);
        ui->passThroughCheckBox->setText(PASSTHROUGHCHECKBOX_CHINESE);
        ui->keySeqHoldDownCheckBox->setText(KEYSEQHOLDDOWNCHECKBOX_CHINESE);
        ui->repeatByKeyCheckBox->setText(REPEATBYKEYCHECKBOX_CHINESE);
        ui->repeatByTimesCheckBox->setText(REPEATBYTIMESCHECKBOX_CHINESE);
        ui->burstpressLabel->setText(BURSTPRESSLABEL_CHINESE);
        ui->burstreleaseLabel->setText(BURSTRELEASE_CHINESE);
        ui->originalKeyLabel->setText(ORIGINALKEYLABEL_CHINESE);
        ui->mappingKeyLabel->setText(MAPPINGKEYLABEL_CHINESE);
        ui->itemNoteLabel->setText(ITEMNOTELABEL_CHINESE);
        ui->orikeyListLabel->setText(ORIKEYLISTLABEL_CHINESE);
        ui->mapkeyListLabel->setText(MAPKEYLISTLABEL_CHINESE);
        ui->originalKeyUpdateButton->setText(UPDATEBUTTON_CHINESE);
        ui->mappingKeyUpdateButton->setText(UPDATEBUTTON_CHINESE);
        ui->itemNoteUpdateButton->setText(UPDATEBUTTON_CHINESE);
    }
}

void QItemSetupDialog::resetFontSize()
{
    int scale = QKeyMapper::getInstance()->m_UI_Scale;
    QFont customFont;
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        customFont.setFamily(FONTNAME_ENGLISH);
        customFont.setPointSize(9);
    }
    else {
        customFont.setFamily(FONTNAME_CHINESE);
        customFont.setBold(true);

        if (UI_SCALE_4K_PERCENT_150 == scale) {
            customFont.setPointSize(11);
        }
        else {
            customFont.setPointSize(9);
        }
    }

    ui->burstCheckBox->setFont(customFont);
    ui->lockCheckBox->setFont(customFont);
    ui->keyupActionCheckBox->setFont(customFont);
    ui->passThroughCheckBox->setFont(customFont);
    ui->keySeqHoldDownCheckBox->setFont(customFont);
    ui->repeatByKeyCheckBox->setFont(customFont);
    ui->repeatByTimesCheckBox->setFont(customFont);
    ui->burstpressLabel->setFont(customFont);
    ui->burstreleaseLabel->setFont(customFont);
    ui->originalKeyLabel->setFont(customFont);
    ui->mappingKeyLabel->setFont(customFont);
    ui->itemNoteLabel->setFont(customFont);
    ui->orikeyListLabel->setFont(customFont);
    ui->mapkeyListLabel->setFont(customFont);
    ui->originalKeyUpdateButton->setFont(customFont);
    ui->mappingKeyUpdateButton->setFont(customFont);
    ui->itemNoteUpdateButton->setFont(customFont);
}

void QItemSetupDialog::setTabIndex(int tabindex)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::setTabIndex]" << "Tab Index =" << tabindex;
#endif

    m_TabIndex = tabindex;
}

void QItemSetupDialog::setItemRow(int row)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::setItemRow]" << "Item Row =" << row;
#endif

    m_ItemRow = row;
}

QString QItemSetupDialog::getOriginalKeyText()
{
    return getInstance()->ui->originalKeyLineEdit->text();
}

int QItemSetupDialog::getOriginalKeyCursorPosition()
{
    return getInstance()->ui->originalKeyLineEdit->cursorPosition();
}

void QItemSetupDialog::setOriginalKeyText(const QString &new_keytext)
{
    return getInstance()->ui->originalKeyLineEdit->setText(new_keytext);
}

QString QItemSetupDialog::getMappingKeyText()
{
    return getInstance()->ui->mappingKeyLineEdit->text();
}

int QItemSetupDialog::getMappingKeyCursorPosition()
{
    return getInstance()->ui->mappingKeyLineEdit->cursorPosition();
}

void QItemSetupDialog::setMappingKeyText(const QString &new_keytext)
{
    return getInstance()->ui->mappingKeyLineEdit->setText(new_keytext);
}

QString QItemSetupDialog::getCurrentOriKeyListText()
{
    return getInstance()->m_OriginalKeyListComboBox->currentText();
}

QString QItemSetupDialog::getCurrentMapKeyListText()
{
    return getInstance()->m_MappingKeyListComboBox->currentText();
}

bool QItemSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QWidget::event(event);
}

void QItemSetupDialog::closeEvent(QCloseEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::closeEvent]" << "Item Row initialize to -1, Tab Index initialize to -1";
#endif
    m_ItemRow = -1;
    m_TabIndex = -1;

    emit QKeyMapper::getInstance()->setupDialogClosed_Signal();

    QDialog::closeEvent(event);
}

void QItemSetupDialog::showEvent(QShowEvent *event)
{
    if (m_ItemRow >= 0 && m_ItemRow < QKeyMapper::KeyMappingDataList->size()) {
        MAP_KEYDATA keymapdata = QKeyMapper::KeyMappingDataList->at(m_ItemRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QItemSetupDialog::showEvent]" << "Load Key Mapping Data[" << m_ItemRow << "] ->" << keymapdata;
#endif

        /* Load Original Key String */
        QString originalkey_str = keymapdata.Original_Key;
        ui->originalKeyLineEdit->setText(originalkey_str);

        /* Load Mapping Keys String */
        QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
        ui->mappingKeyLineEdit->setText(mappingkeys_str);

        /* Load Note String */
        ui->itemNoteLineEdit->setText(keymapdata.Note);

        /* Load Burst */
        if (true == keymapdata.Burst) {
            ui->burstCheckBox->setChecked(true);
        }
        else {
            ui->burstCheckBox->setChecked(false);
        }

        /* Load Burst press time */
        if (BURST_TIME_MIN <= keymapdata.BurstPressTime && keymapdata.BurstPressTime <= BURST_TIME_MAX) {
            ui->burstpressSpinBox->setValue(keymapdata.BurstPressTime);
        }
        else {
            ui->burstpressSpinBox->setValue(BURST_PRESS_TIME_DEFAULT);
        }

        /* Load Burst release time */
        if (BURST_TIME_MIN <= keymapdata.BurstReleaseTime && keymapdata.BurstReleaseTime <= BURST_TIME_MAX) {
            ui->burstreleaseSpinBox->setValue(keymapdata.BurstReleaseTime);
        }
        else {
            ui->burstreleaseSpinBox->setValue(BURST_RELEASE_TIME_DEFAULT);
        }

        /* Load Lock */
        if (true == keymapdata.Lock) {
            ui->lockCheckBox->setChecked(true);
        }
        else {
            ui->lockCheckBox->setChecked(false);
        }

        /* Load KeyUp Action Status */
        if (true == keymapdata.KeyUp_Action) {
            ui->keyupActionCheckBox->setChecked(true);
            ui->keySeqHoldDownCheckBox->setEnabled(false);
        }
        else {
            ui->keyupActionCheckBox->setChecked(false);
            if (keymapdata.Mapping_Keys.size() > 1) {
                ui->keySeqHoldDownCheckBox->setEnabled(true);
            }
            else {
                ui->keySeqHoldDownCheckBox->setEnabled(false);
            }
        }

        /* Load PassThrough Status */
        if (true == keymapdata.PassThrough) {
            ui->passThroughCheckBox->setChecked(true);
        }
        else {
            ui->passThroughCheckBox->setChecked(false);
        }

        /* Load KeySequenceHoldDown Status */
        if (true == keymapdata.KeySeqHoldDown) {
            ui->keySeqHoldDownCheckBox->setChecked(true);
        }
        else {
            ui->keySeqHoldDownCheckBox->setChecked(false);
        }

        /* Load RepeatMode Status */
        if (REPEAT_MODE_BYKEY == keymapdata.RepeatMode) {
            ui->repeatByKeyCheckBox->setChecked(true);
            ui->repeatByTimesCheckBox->setChecked(false);
        }
        else if (REPEAT_MODE_BYTIMES == keymapdata.RepeatMode) {
            ui->repeatByKeyCheckBox->setChecked(false);
            ui->repeatByTimesCheckBox->setChecked(true);
        }
        else {
            ui->repeatByKeyCheckBox->setChecked(false);
            ui->repeatByTimesCheckBox->setChecked(false);
        }

        /* Load RepeatTimes */
        if (REPEAT_TIMES_MIN <= keymapdata.RepeatTimes && keymapdata.RepeatTimes <= REPEAT_TIMES_MAX) {
            ui->repeatTimesSpinBox->setValue(keymapdata.RepeatTimes);
        }
        else {
            ui->repeatTimesSpinBox->setValue(REPEAT_TIMES_DEFAULT);
        }

        ui->originalKeyLineEdit->setFocus();
        ui->originalKeyLineEdit->clearFocus();

        bool burstEnabled = QKeyMapper::getKeyMappingDataTableItemBurstStatus(m_ItemRow);
        bool lockEnabled = QKeyMapper::getKeyMappingDataTableItemLockStatus(m_ItemRow);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QItemSetupDialog::showEvent] Burst item in row" << m_ItemRow << " enabled =" << burstEnabled;
        qDebug() << "[QItemSetupDialog::showEvent] Lock item in row" << m_ItemRow << " enabled =" << lockEnabled;
#endif

        if (burstEnabled) {
            ui->burstCheckBox->setEnabled(true);
            ui->burstpressSpinBox->setEnabled(true);
            ui->burstreleaseSpinBox->setEnabled(true);
        }
        else {
            ui->burstCheckBox->setEnabled(false);
            ui->burstpressSpinBox->setEnabled(false);
            ui->burstreleaseSpinBox->setEnabled(false);
        }

        if (lockEnabled) {
            ui->lockCheckBox->setEnabled(true);
        }
        else {
            ui->lockCheckBox->setEnabled(false);
        }

        if (keymapdata.Mapping_Keys.size() > 1) {
            ui->keySeqHoldDownCheckBox->setEnabled(true);
            ui->repeatByKeyCheckBox->setEnabled(true);
            ui->repeatByTimesCheckBox->setEnabled(true);
            ui->repeatTimesSpinBox->setEnabled(true);
        }
        else {
            ui->keySeqHoldDownCheckBox->setEnabled(false);
            ui->repeatByKeyCheckBox->setEnabled(false);
            ui->repeatByTimesCheckBox->setEnabled(false);
            ui->repeatTimesSpinBox->setEnabled(false);
        }

        if (keymapdata.KeySeqHoldDown) {
            ui->repeatByKeyCheckBox->setEnabled(false);
            ui->repeatByTimesCheckBox->setEnabled(false);
            ui->repeatTimesSpinBox->setEnabled(false);
        }
    }

    QDialog::showEvent(event);
}

void QItemSetupDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QItemSetupDialog::keyPressEvent]" << "ESC Key Pressed, Item Row & Tab Index initialize to -1 and close SetupDialog.";
#endif
        m_ItemRow = -1;
        m_TabIndex = -1;
        emit QKeyMapper::getInstance()->setupDialogClosed_Signal();
    }

    QDialog::keyPressEvent(event);
}

void QItemSetupDialog::initKeyListComboBoxes()
{
    KeyListComboBox *orikeyComboBox = QKeyMapper::getInstance()->m_orikeyComboBox;
    KeyListComboBox *mapkeyComboBox = QKeyMapper::getInstance()->m_mapkeyComboBox;

    m_OriginalKeyListComboBox->addItem(QString());
    m_OriginalKeyListComboBox->addItem(SEPARATOR_LONGPRESS);
    m_OriginalKeyListComboBox->addItem(SEPARATOR_DOUBLEPRESS);
    for(int i = 1; i < orikeyComboBox->count(); i++) {
        QString text = orikeyComboBox->itemText(i);
        m_OriginalKeyListComboBox->addItem(text);
        s_valiedOriginalKeyList.append(text);
    }

    m_MappingKeyListComboBox->addItem(QString());
    m_MappingKeyListComboBox->addItem(SEPARATOR_WAITTIME);
    m_MappingKeyListComboBox->addItem(PREFIX_SEND_DOWN);
    m_MappingKeyListComboBox->addItem(PREFIX_SEND_UP);
    m_MappingKeyListComboBox->addItem(PREFIX_SEND_BOTH);
    m_MappingKeyListComboBox->addItem(PREFIX_SEND_EXCLUSION);
    for(int i = 1; i < mapkeyComboBox->count(); i++) {
        QString text = mapkeyComboBox->itemText(i);
        m_MappingKeyListComboBox->addItem(text);
        s_valiedMappingKeyList.append(text);
    }
    s_valiedMappingKeyList.append(QKeyMapper_Worker::SpecialOriginalKeysList);

    int left = ui->orikeyListLabel->x() + ui->orikeyListLabel->width() + 5;
    int top = ui->orikeyListLabel->y();
    m_OriginalKeyListComboBox->setObjectName(SETUPDIALOG_ORIKEY_COMBOBOX_NAME);
    m_OriginalKeyListComboBox->setGeometry(QRect(left, top, 160, 22));

    left = ui->mapkeyListLabel->x() + ui->mapkeyListLabel->width() + 5;
    top = ui->mapkeyListLabel->y();
    m_MappingKeyListComboBox->setObjectName(SETUPDIALOG_MAPKEY_COMBOBOX_NAME);
    m_MappingKeyListComboBox->setGeometry(QRect(left, top, 160, 22));
}

void QItemSetupDialog::refreshOriginalKeyRelatedUI()
{
    if (m_ItemRow >= 0 && m_ItemRow < QKeyMapper::KeyMappingDataList->size()) {
        MAP_KEYDATA keymapdata = QKeyMapper::KeyMappingDataList->at(m_ItemRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QItemSetupDialog::refreshOriginalKeyRelatedUI]" << "Load Key Mapping Data[" << m_ItemRow << "] ->" << keymapdata;
#endif
        /* Load Original Key String */
        QString originalkey_str = keymapdata.Original_Key;
        ui->originalKeyLineEdit->setText(originalkey_str);

        /* Load Burst */
        if (true == keymapdata.Burst) {
            ui->burstCheckBox->setChecked(true);
        }
        else {
            ui->burstCheckBox->setChecked(false);
        }

        /* Load Burst press time */
        if (BURST_TIME_MIN <= keymapdata.BurstPressTime && keymapdata.BurstPressTime <= BURST_TIME_MAX) {
            ui->burstpressSpinBox->setValue(keymapdata.BurstPressTime);
        }
        else {
            ui->burstpressSpinBox->setValue(BURST_PRESS_TIME_DEFAULT);
        }

        /* Load Burst release time */
        if (BURST_TIME_MIN <= keymapdata.BurstReleaseTime && keymapdata.BurstReleaseTime <= BURST_TIME_MAX) {
            ui->burstreleaseSpinBox->setValue(keymapdata.BurstReleaseTime);
        }
        else {
            ui->burstreleaseSpinBox->setValue(BURST_RELEASE_TIME_DEFAULT);
        }

        /* Load Lock */
        if (true == keymapdata.Lock) {
            ui->lockCheckBox->setChecked(true);
        }
        else {
            ui->lockCheckBox->setChecked(false);
        }

        /* Load KeyUp Action Status */
        if (true == keymapdata.KeyUp_Action) {
            ui->keyupActionCheckBox->setChecked(true);
            ui->keySeqHoldDownCheckBox->setEnabled(false);
        }
        else {
            ui->keyupActionCheckBox->setChecked(false);
            if (keymapdata.Mapping_Keys.size() > 1) {
                ui->keySeqHoldDownCheckBox->setEnabled(true);
            }
            else {
                ui->keySeqHoldDownCheckBox->setEnabled(false);
            }
        }

        /* Load PassThrough Status */
        if (true == keymapdata.PassThrough) {
            ui->passThroughCheckBox->setChecked(true);
        }
        else {
            ui->passThroughCheckBox->setChecked(false);
        }

        /* Load KeySequenceHoldDown Status */
        if (true == keymapdata.KeySeqHoldDown) {
            ui->keySeqHoldDownCheckBox->setChecked(true);
        }
        else {
            ui->keySeqHoldDownCheckBox->setChecked(false);
        }

        /* Load RepeatMode Status */
        if (REPEAT_MODE_BYKEY == keymapdata.RepeatMode) {
            ui->repeatByKeyCheckBox->setChecked(true);
            ui->repeatByTimesCheckBox->setChecked(false);
        }
        else if (REPEAT_MODE_BYTIMES == keymapdata.RepeatMode) {
            ui->repeatByKeyCheckBox->setChecked(false);
            ui->repeatByTimesCheckBox->setChecked(true);
        }
        else {
            ui->repeatByKeyCheckBox->setChecked(false);
            ui->repeatByTimesCheckBox->setChecked(false);
        }

        /* Load RepeatTimes */
        if (REPEAT_TIMES_MIN <= keymapdata.RepeatTimes && keymapdata.RepeatTimes <= REPEAT_TIMES_MAX) {
            ui->repeatTimesSpinBox->setValue(keymapdata.RepeatTimes);
        }
        else {
            ui->repeatTimesSpinBox->setValue(REPEAT_TIMES_DEFAULT);
        }

        ui->originalKeyLineEdit->setFocus();
        ui->originalKeyLineEdit->clearFocus();

        bool burstEnabled = QKeyMapper::getKeyMappingDataTableItemBurstStatus(m_ItemRow);
        bool lockEnabled = QKeyMapper::getKeyMappingDataTableItemLockStatus(m_ItemRow);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QItemSetupDialog::refreshOriginalKeyRelatedUI] Burst item in row" << m_ItemRow << " enabled =" << burstEnabled;
        qDebug() << "[QItemSetupDialog::refreshOriginalKeyRelatedUI] Lock item in row" << m_ItemRow << " enabled =" << lockEnabled;
#endif

        if (burstEnabled) {
            ui->burstCheckBox->setEnabled(true);
            ui->burstpressSpinBox->setEnabled(true);
            ui->burstreleaseSpinBox->setEnabled(true);
        }
        else {
            ui->burstCheckBox->setEnabled(false);
            ui->burstpressSpinBox->setEnabled(false);
            ui->burstreleaseSpinBox->setEnabled(false);
        }

        if (lockEnabled) {
            ui->lockCheckBox->setEnabled(true);
        }
        else {
            ui->lockCheckBox->setEnabled(false);
        }

        if (keymapdata.Mapping_Keys.size() > 1) {
            ui->keySeqHoldDownCheckBox->setEnabled(true);
            ui->repeatByKeyCheckBox->setEnabled(true);
            ui->repeatByTimesCheckBox->setEnabled(true);
            ui->repeatTimesSpinBox->setEnabled(true);
        }
        else {
            ui->keySeqHoldDownCheckBox->setEnabled(false);
            ui->repeatByKeyCheckBox->setEnabled(false);
            ui->repeatByTimesCheckBox->setEnabled(false);
            ui->repeatTimesSpinBox->setEnabled(false);
        }

        if (keymapdata.KeySeqHoldDown) {
            ui->repeatByKeyCheckBox->setEnabled(false);
            ui->repeatByTimesCheckBox->setEnabled(false);
            ui->repeatTimesSpinBox->setEnabled(false);
        }
    }
}

bool QItemSetupDialog::refreshMappingKeyRelatedUI()
{
    bool value_changed = false;
    if (m_ItemRow >= 0 && m_ItemRow < QKeyMapper::KeyMappingDataList->size()) {
        MAP_KEYDATA keymapdata = QKeyMapper::KeyMappingDataList->at(m_ItemRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QItemSetupDialog::refreshMappingKeyRelatedUI]" << "Load Key Mapping Data[" << m_ItemRow << "] ->" << keymapdata;
#endif

        bool burstEnabled = QKeyMapper::getKeyMappingDataTableItemBurstStatus(m_ItemRow);
        bool lockEnabled = QKeyMapper::getKeyMappingDataTableItemLockStatus(m_ItemRow);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QItemSetupDialog::refreshMappingKeyRelatedUI] Burst item in row" << m_ItemRow << " enabled =" << burstEnabled;
        qDebug() << "[QItemSetupDialog::refreshMappingKeyRelatedUI] Lock item in row" << m_ItemRow << " enabled =" << lockEnabled;
#endif

        if (burstEnabled) {
            ui->burstCheckBox->setEnabled(true);
            ui->burstpressSpinBox->setEnabled(true);
            ui->burstreleaseSpinBox->setEnabled(true);
        }
        else {
            if (keymapdata.Burst) {
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Burst = false;
                keymapdata.Burst = false;
                value_changed = true;
            }
            ui->burstCheckBox->setEnabled(false);
            ui->burstpressSpinBox->setEnabled(false);
            ui->burstreleaseSpinBox->setEnabled(false);
        }

        if (lockEnabled) {
            ui->lockCheckBox->setEnabled(true);
        }
        else {
            if (keymapdata.Lock) {
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Lock = false;
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].LockStatus = false;
                keymapdata.Lock = false;
                value_changed = true;
            }
            ui->lockCheckBox->setEnabled(false);
        }

        if (keymapdata.Mapping_Keys.size() > 1) {
            ui->keySeqHoldDownCheckBox->setEnabled(true);
            ui->repeatByKeyCheckBox->setEnabled(true);
            ui->repeatByTimesCheckBox->setEnabled(true);
            ui->repeatTimesSpinBox->setEnabled(true);
        }
        else {
            if (keymapdata.RepeatMode != REPEAT_MODE_NONE) {
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].RepeatMode = REPEAT_MODE_NONE;
                keymapdata.RepeatMode = REPEAT_MODE_NONE;
                value_changed = true;
            }
            if (keymapdata.KeySeqHoldDown) {
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
                keymapdata.KeySeqHoldDown = false;
                value_changed = true;
            }
            ui->keySeqHoldDownCheckBox->setEnabled(false);
            ui->repeatByKeyCheckBox->setEnabled(false);
            ui->repeatByTimesCheckBox->setEnabled(false);
            ui->repeatTimesSpinBox->setEnabled(false);
        }

        if (keymapdata.KeySeqHoldDown) {
            if (keymapdata.RepeatMode != REPEAT_MODE_NONE) {
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].RepeatMode = REPEAT_MODE_NONE;
                keymapdata.RepeatMode = REPEAT_MODE_NONE;
                value_changed = true;
            }
            ui->repeatByKeyCheckBox->setEnabled(false);
            ui->repeatByTimesCheckBox->setEnabled(false);
            ui->repeatTimesSpinBox->setEnabled(false);
        }

        keymapdata = QKeyMapper::KeyMappingDataList->at(m_ItemRow);

        /* Load Mapping Keys String */
        QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
        ui->mappingKeyLineEdit->setText(mappingkeys_str);

        /* Load Burst */
        if (true == keymapdata.Burst) {
            ui->burstCheckBox->setChecked(true);
        }
        else {
            ui->burstCheckBox->setChecked(false);
        }

        /* Load Burst press time */
        if (BURST_TIME_MIN <= keymapdata.BurstPressTime && keymapdata.BurstPressTime <= BURST_TIME_MAX) {
            ui->burstpressSpinBox->setValue(keymapdata.BurstPressTime);
        }
        else {
            ui->burstpressSpinBox->setValue(BURST_PRESS_TIME_DEFAULT);
        }

        /* Load Burst release time */
        if (BURST_TIME_MIN <= keymapdata.BurstReleaseTime && keymapdata.BurstReleaseTime <= BURST_TIME_MAX) {
            ui->burstreleaseSpinBox->setValue(keymapdata.BurstReleaseTime);
        }
        else {
            ui->burstreleaseSpinBox->setValue(BURST_RELEASE_TIME_DEFAULT);
        }

        /* Load Lock */
        if (true == keymapdata.Lock) {
            ui->lockCheckBox->setChecked(true);
        }
        else {
            ui->lockCheckBox->setChecked(false);
        }

        /* Load KeyUp Action Status */
        if (true == keymapdata.KeyUp_Action) {
            ui->keyupActionCheckBox->setChecked(true);
            ui->keySeqHoldDownCheckBox->setEnabled(false);
        }
        else {
            ui->keyupActionCheckBox->setChecked(false);
            if (keymapdata.Mapping_Keys.size() > 1) {
                ui->keySeqHoldDownCheckBox->setEnabled(true);
            }
            else {
                ui->keySeqHoldDownCheckBox->setEnabled(false);
            }
        }

        /* Load PassThrough Status */
        if (true == keymapdata.PassThrough) {
            ui->passThroughCheckBox->setChecked(true);
        }
        else {
            ui->passThroughCheckBox->setChecked(false);
        }

        /* Load KeySequenceHoldDown Status */
        if (true == keymapdata.KeySeqHoldDown) {
            ui->keySeqHoldDownCheckBox->setChecked(true);
        }
        else {
            ui->keySeqHoldDownCheckBox->setChecked(false);
        }

        /* Load RepeatMode Status */
        if (REPEAT_MODE_BYKEY == keymapdata.RepeatMode) {
            ui->repeatByKeyCheckBox->setChecked(true);
            ui->repeatByTimesCheckBox->setChecked(false);
        }
        else if (REPEAT_MODE_BYTIMES == keymapdata.RepeatMode) {
            ui->repeatByKeyCheckBox->setChecked(false);
            ui->repeatByTimesCheckBox->setChecked(true);
        }
        else {
            ui->repeatByKeyCheckBox->setChecked(false);
            ui->repeatByTimesCheckBox->setChecked(false);
        }

        /* Load RepeatTimes */
        if (REPEAT_TIMES_MIN <= keymapdata.RepeatTimes && keymapdata.RepeatTimes <= REPEAT_TIMES_MAX) {
            ui->repeatTimesSpinBox->setValue(keymapdata.RepeatTimes);
        }
        else {
            ui->repeatTimesSpinBox->setValue(REPEAT_TIMES_DEFAULT);
        }

        ui->originalKeyLineEdit->setFocus();
        ui->originalKeyLineEdit->clearFocus();
    }

    return value_changed;
}

void QItemSetupDialog::on_burstpressSpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->burstpressSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).BurstPressTime) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].BurstPressTime = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_burstpressSpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Burst Press Time -> " << current_value;
#endif
    }
}


void QItemSetupDialog::on_burstreleaseSpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->burstreleaseSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).BurstReleaseTime) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].BurstReleaseTime = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_burstreleaseSpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Burst Release Time -> " << current_value;
#endif
    }
}


void QItemSetupDialog::on_burstCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool burst = ui->burstCheckBox->isChecked();
    if (burst != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Burst) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Burst = burst;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Burst -> " << burst;
#endif
    }
}


void QItemSetupDialog::on_lockCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool lock = ui->lockCheckBox->isChecked();
    if (lock != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Lock) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Lock = lock;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Lock -> " << lock;
#endif
    }
}


void QItemSetupDialog::on_keyupActionCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool keyup_action = ui->keyupActionCheckBox->isChecked();
    if (keyup_action != QKeyMapper::KeyMappingDataList->at(m_ItemRow).KeyUp_Action) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeyUp_Action = keyup_action;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] KeyUpAction -> " << keyup_action;
#endif

        /* Update KeySeqHoldDown Status */
        if (true == keyup_action) {
            ui->keySeqHoldDownCheckBox->setEnabled(false);
        }
        else {
            ui->keySeqHoldDownCheckBox->setEnabled(true);
        }
    }
}


void QItemSetupDialog::on_passThroughCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool passthrough = ui->passThroughCheckBox->isChecked();
    if (passthrough != QKeyMapper::KeyMappingDataList->at(m_ItemRow).PassThrough) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].PassThrough = passthrough;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] PassThrough -> " << passthrough;
#endif
    }
}


void QItemSetupDialog::on_keySeqHoldDownCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool keyseqholddown = ui->keySeqHoldDownCheckBox->isChecked();
    if (keyseqholddown != QKeyMapper::KeyMappingDataList->at(m_ItemRow).KeySeqHoldDown) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = keyseqholddown;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] KeySequenceHoldDown -> " << keyseqholddown;
#endif
    }

    (void)refreshMappingKeyRelatedUI();
}

void QItemSetupDialog::on_originalKeyUpdateButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int tabindex = m_TabIndex;
    static QRegularExpression whitespace_reg(R"(\s+)");
    QString originalKey = ui->originalKeyLineEdit->text();
    originalKey.remove(whitespace_reg);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] OriginalKeyText remove whitespace -> " << originalKey;
#endif

    ValidationResult result = QKeyMapper::validateOriginalKeyString(originalKey, m_ItemRow);

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;
    if (result.isValid) {
        popupMessageColor = "#44bd32";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = "OriginalKey update success.";
        }
        else {
            popupMessage = "原始按键更新成功";
        }

        if ((*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key != originalKey) {
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key = originalKey;
        }

        QKeyMapper::getInstance()->refreshKeyMappingDataTableByTabIndex(tabindex);

        refreshOriginalKeyRelatedUI();
    }
    else {
        popupMessageColor = "#d63031";
        popupMessage = result.errorMessage;
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}

void QItemSetupDialog::on_mappingKeyUpdateButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int tabindex = m_TabIndex;
    static QRegularExpression whitespace_reg(R"(\s+)");
    QString mappingKey = ui->mappingKeyLineEdit->text();
    mappingKey.remove(whitespace_reg);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] MappingKeyText remove whitespace -> " << mappingKey;
#endif

    QStringList mappingKeySeqList = splitMappingKeyString(mappingKey, SPLIT_WITH_NEXT);
    ValidationResult result = QKeyMapper::validateMappingKeyString(mappingKey, mappingKeySeqList, m_ItemRow);

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;
    if (result.isValid) {
        popupMessageColor = "#44bd32";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = "MappingKey update success.";
        }
        else {
            popupMessage = "映射按键更新成功";
        }

        QKeyMapper::updateKeyMappingDataListMappingKeys(m_ItemRow, mappingKey);

        QKeyMapper::getInstance()->refreshKeyMappingDataTableByTabIndex(tabindex);

        (void)refreshMappingKeyRelatedUI();
    }
    else {
        popupMessageColor = "#d63031";
        popupMessage = result.errorMessage;
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}

void QItemSetupDialog::on_repeatByKeyCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int repeatmode = REPEAT_MODE_NONE;
    bool repeatbykey = ui->repeatByKeyCheckBox->isChecked();
    ui->repeatByTimesCheckBox->blockSignals(true);
    if (repeatbykey) {
        repeatmode = REPEAT_MODE_BYKEY;
        ui->repeatByTimesCheckBox->setChecked(false);
    }
    else {
        repeatmode = REPEAT_MODE_NONE;
        ui->repeatByTimesCheckBox->setChecked(false);
    }
    ui->repeatByTimesCheckBox->blockSignals(false);
    if (repeatmode != QKeyMapper::KeyMappingDataList->at(m_ItemRow).RepeatMode) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].RepeatMode = repeatmode;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]("<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << ") RepeatMode -> " << repeatmode;
#endif
    }
}


void QItemSetupDialog::on_repeatByTimesCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int repeatmode = REPEAT_MODE_NONE;
    bool repeatbykey = ui->repeatByTimesCheckBox->isChecked();
    ui->repeatByKeyCheckBox->blockSignals(true);
    if (repeatbykey) {
        repeatmode = REPEAT_MODE_BYTIMES;
        ui->repeatByKeyCheckBox->setChecked(false);
    }
    else {
        repeatmode = REPEAT_MODE_NONE;
        ui->repeatByKeyCheckBox->setChecked(false);
    }
    ui->repeatByKeyCheckBox->blockSignals(false);
    if (repeatmode != QKeyMapper::KeyMappingDataList->at(m_ItemRow).RepeatMode) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].RepeatMode = repeatmode;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]("<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << ") RepeatMode -> " << repeatmode;
#endif
    }
}

void QItemSetupDialog::on_repeatTimesSpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->repeatTimesSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).RepeatTimes) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].RepeatTimes = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_repeatTimesSpinBox_editingFinished]" << " Row[" << m_ItemRow << "]("<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << ") Repeat Times -> " << current_value;
#endif
    }
}

void QItemSetupDialog::on_itemNoteUpdateButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int tabindex = m_TabIndex;
    QString note_str = ui->itemNoteLineEdit->text();

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] Item note -> " << note_str;
#endif

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;

    popupMessageColor = "#44bd32";
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        popupMessage = "Mapping item note update success.";
    }
    else {
        popupMessage = "映射项备注更新成功";
    }

    if ((*QKeyMapper::KeyMappingDataList)[m_ItemRow].Note != note_str) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Note = note_str;
    }

    QKeyMapper::getInstance()->refreshKeyMappingDataTableByTabIndex(tabindex);

    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}
