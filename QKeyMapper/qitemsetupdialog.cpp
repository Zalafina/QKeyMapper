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

    QDialog::closeEvent(event);
}

void QItemSetupDialog::showEvent(QShowEvent *event)
{
    if (m_ItemRow >= 0 && m_ItemRow < QKeyMapper::KeyMappingDataList.size()) {
        MAP_KEYDATA mapping_data = QKeyMapper::KeyMappingDataList.at(m_ItemRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QItemSetupDialog::showEvent]" << "Load Item Mapping Data ->" << mapping_data;
#endif

        QString originalkey_str = mapping_data.Original_Key;
        if (originalkey_str.startsWith(PREFIX_SHORTCUT)) {
            originalkey_str.remove(PREFIX_SHORTCUT);
        }
        ui->originalKeyLineEdit->setText(originalkey_str);

        QString mappingkeys_str = mapping_data.Mapping_Keys.join(SEPARATOR_NEXTARROW);
        ui->mappingKeyLineEdit->setText(mappingkeys_str);
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
