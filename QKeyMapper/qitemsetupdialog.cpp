#include "qkeymapper.h"
#include "qitemsetupdialog.h"
#include "ui_qitemsetupdialog.h"
#include "qkeymapper_constants.h"

QItemSetupDialog *QItemSetupDialog::m_instance = Q_NULLPTR;

QItemSetupDialog::QItemSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QItemSetupDialog)
    , m_ItemRow(-1)
    , m_OriginalKeyListComboBox(new KeyListComboBox(this))
    , m_MappingKeyListComboBox(new KeyListComboBox(this))
{
    m_instance = this;
    ui->setupUi(this);

    initKeyListComboBoxes();

    ui->originalKeyLineEdit->setReadOnly(true);
    ui->mappingKeyLineEdit->setReadOnly(true);
    ui->originalKeyLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->mappingKeyLineEdit->setFocusPolicy(Qt::ClickFocus);

    ui->originalKeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->mappingKeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->burstpressSpinBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->burstreleaseSpinBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    m_OriginalKeyListComboBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    m_MappingKeyListComboBox->setFont(QFont(FONTNAME_ENGLISH, 9));

    ui->burstpressSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->burstreleaseSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
}

QItemSetupDialog::~QItemSetupDialog()
{
    delete ui;
}

void QItemSetupDialog::setUILanguagee(int languageindex)
{
    if (LANGUAGE_ENGLISH == languageindex) {
        ui->burstCheckBox->setText(BURSTCHECKBOX_ENGLISH);
        ui->lockCheckBox->setText(LOCKCHECKBOX_ENGLISH);
        ui->keyupActionCheckBox->setText(KEYUPACTIONCHECKBOX_ENGLISH);
        ui->passThroughCheckBox->setText(PASSTHROUGHCHECKBOX_ENGLISH);
        ui->burstpressLabel->setText(BURSTPRESSLABEL_ENGLISH);
        ui->burstreleaseLabel->setText(BURSTRELEASE_ENGLISH);
        ui->originalKeyLabel->setText(ORIKEYLABEL_ENGLISH);
        ui->mappingKeyLabel->setText(MAPKEYLABEL_ENGLISH);
        ui->orikeyListLabel->setText(ORIKEYLISTLABEL_ENGLISH);
        ui->mapkeyListLabel->setText(MAPKEYLISTLABEL_ENGLISH);
        ui->originalKeyUpdateButton->setText(UPDATEBUTTON_ENGLISH);
        ui->mappingKeyUpdateButton->setText(UPDATEBUTTON_ENGLISH);
    }
    else {
        ui->burstCheckBox->setText(BURSTCHECKBOX_CHINESE);
        ui->lockCheckBox->setText(LOCKCHECKBOX_CHINESE);
        ui->keyupActionCheckBox->setText(KEYUPACTIONCHECKBOX_CHINESE);
        ui->passThroughCheckBox->setText(PASSTHROUGHCHECKBOX_CHINESE);
        ui->burstpressLabel->setText(BURSTPRESSLABEL_CHINESE);
        ui->burstreleaseLabel->setText(BURSTRELEASE_CHINESE);
        ui->originalKeyLabel->setText(ORIKEYLABEL_CHINESE);
        ui->mappingKeyLabel->setText(MAPKEYLABEL_CHINESE);
        ui->orikeyListLabel->setText(ORIKEYLISTLABEL_CHINESE);
        ui->mapkeyListLabel->setText(MAPKEYLISTLABEL_CHINESE);
        ui->originalKeyUpdateButton->setText(UPDATEBUTTON_CHINESE);
        ui->mappingKeyUpdateButton->setText(UPDATEBUTTON_CHINESE);
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
    ui->burstpressLabel->setFont(customFont);
    ui->burstreleaseLabel->setFont(customFont);
    ui->originalKeyLabel->setFont(customFont);
    ui->mappingKeyLabel->setFont(customFont);
    ui->orikeyListLabel->setFont(customFont);
    ui->mapkeyListLabel->setFont(customFont);
    ui->originalKeyUpdateButton->setFont(customFont);
    ui->mappingKeyUpdateButton->setFont(customFont);
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

void QItemSetupDialog::setOriginalKeyText(const QString &new_keytext)
{
    return getInstance()->ui->originalKeyLineEdit->setText(new_keytext);
}

QString QItemSetupDialog::getMappingKeyText()
{
    return getInstance()->ui->mappingKeyLineEdit->text();
}

void QItemSetupDialog::setMappingKeyText(const QString &new_keytext)
{
    return getInstance()->ui->mappingKeyLineEdit->setText(new_keytext);
}

void QItemSetupDialog::closeEvent(QCloseEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::closeEvent]" << "Item Row initialize to -1";
#endif
    m_ItemRow = -1;

    emit QKeyMapper::getInstance()->setupDialogClosed_Signal();

    QDialog::closeEvent(event);
}

void QItemSetupDialog::showEvent(QShowEvent *event)
{
    if (m_ItemRow >= 0 && m_ItemRow < QKeyMapper::KeyMappingDataList.size()) {
        MAP_KEYDATA keymapdata = QKeyMapper::KeyMappingDataList.at(m_ItemRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QItemSetupDialog::showEvent]" << "Load Key Mapping Data[" << m_ItemRow << "] ->" << keymapdata;
#endif

        /* Load Original Key String */
        QString originalkey_str = keymapdata.Original_Key;
        if (originalkey_str.startsWith(PREFIX_SHORTCUT)) {
            originalkey_str.remove(PREFIX_SHORTCUT);
        }
        ui->originalKeyLineEdit->setText(originalkey_str);

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
        }
        else {
            ui->keyupActionCheckBox->setChecked(false);
        }

        /* Load PassThrough Status */
        if (true == keymapdata.PassThrough) {
            ui->passThroughCheckBox->setChecked(true);
        }
        else {
            ui->passThroughCheckBox->setChecked(false);
        }
    }

    QDialog::showEvent(event);
}

void QItemSetupDialog::initKeyListComboBoxes()
{
    KeyListComboBox *orikeyComboBox = QKeyMapper::getInstance()->m_orikeyComboBox;
    KeyListComboBox *mapkeyComboBox = QKeyMapper::getInstance()->m_mapkeyComboBox;

    for(int i = 0; i < orikeyComboBox->count(); i++) {
        QString text = orikeyComboBox->itemText(i);
        m_OriginalKeyListComboBox->addItem(text);
    }

    for(int i = 0; i < mapkeyComboBox->count(); i++) {
        QString text = mapkeyComboBox->itemText(i);
        m_MappingKeyListComboBox->addItem(text);
    }

    int left = ui->orikeyListLabel->x() + ui->orikeyListLabel->width() + 5;
    int top = ui->orikeyListLabel->y();
    m_OriginalKeyListComboBox->setObjectName(SETUPDIALOG_ORIKEY_COMBOBOX_NAME);
    m_OriginalKeyListComboBox->setGeometry(QRect(left, top, 142, 22));

    left = ui->mapkeyListLabel->x() + ui->mapkeyListLabel->width() + 5;
    top = ui->mapkeyListLabel->y();
    m_MappingKeyListComboBox->setObjectName(SETUPDIALOG_MAPKEY_COMBOBOX_NAME);
    m_MappingKeyListComboBox->setGeometry(QRect(left, top, 142, 22));
}

void QItemSetupDialog::on_burstpressSpinBox_editingFinished()
{

}


void QItemSetupDialog::on_burstreleaseSpinBox_editingFinished()
{

}


void QItemSetupDialog::on_burstCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList.size()) {
        return;
    }

    bool burst = ui->burstCheckBox->isChecked();
    if (burst != QKeyMapper::KeyMappingDataList.at(m_ItemRow).Burst) {
        QKeyMapper::KeyMappingDataList[m_ItemRow].Burst = burst;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "]: Row[" << m_ItemRow << "]["<< QKeyMapper::KeyMappingDataList[m_ItemRow].Original_Key << "] Burst -> " << burst;
#endif
    }
}


void QItemSetupDialog::on_lockCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList.size()) {
        return;
    }

    bool lock = ui->lockCheckBox->isChecked();
    if (lock != QKeyMapper::KeyMappingDataList.at(m_ItemRow).Lock) {
        QKeyMapper::KeyMappingDataList[m_ItemRow].Lock = lock;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "]: Row[" << m_ItemRow << "]["<< QKeyMapper::KeyMappingDataList[m_ItemRow].Original_Key << "] Lock -> " << lock;
#endif
    }
}


void QItemSetupDialog::on_keyupActionCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList.size()) {
        return;
    }

    bool keyup_action = ui->keyupActionCheckBox->isChecked();
    if (keyup_action != QKeyMapper::KeyMappingDataList.at(m_ItemRow).KeyUp_Action) {
        QKeyMapper::KeyMappingDataList[m_ItemRow].KeyUp_Action = keyup_action;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "]: Row[" << m_ItemRow << "]["<< QKeyMapper::KeyMappingDataList[m_ItemRow].Original_Key << "] KeyUpAction -> " << keyup_action;
#endif
    }
}


void QItemSetupDialog::on_passThroughCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList.size()) {
        return;
    }

    bool passthrough = ui->passThroughCheckBox->isChecked();
    if (passthrough != QKeyMapper::KeyMappingDataList.at(m_ItemRow).PassThrough) {
        QKeyMapper::KeyMappingDataList[m_ItemRow].PassThrough = passthrough;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "]: Row[" << m_ItemRow << "]["<< QKeyMapper::KeyMappingDataList[m_ItemRow].Original_Key << "] PassThrough -> " << passthrough;
#endif
    }
}

