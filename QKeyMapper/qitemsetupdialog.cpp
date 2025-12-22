#include "qkeymapper.h"
#include "qitemsetupdialog.h"
#include "ui_qitemsetupdialog.h"

using namespace QKeyMapperConstants;

QItemSetupDialog *QItemSetupDialog::m_instance = Q_NULLPTR;
QStringList QItemSetupDialog::s_valiedOriginalKeyList;
QStringList QItemSetupDialog::s_valiedMappingKeyList;
int QItemSetupDialog::s_editingMappingKeyLineEdit = ITEMSETUP_EDITING_MAPPINGKEY;

QItemSetupDialog::QItemSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QItemSetupDialog)
    , m_TabIndex(-1)
    , m_ItemRow(-1)
    , m_KeyRecordDialog(Q_NULLPTR)
    , m_CrosshairSetupDialog(Q_NULLPTR)
    , m_OriginalKeyListComboBox(new KeyListComboBox(this))
    , m_MappingKeyListComboBox(new KeyListComboBox(this))
    , m_MappingKeyLineEdit(new KeyStringLineEdit(this))
    , m_MappingKey_KeyUpLineEdit(new KeyStringLineEdit(this))
{
    m_instance = this;
    ui->setupUi(this);

    ui->keyRecordLineEdit->installEventFilter(this);

    m_KeyRecordDialog = new QKeyRecord(this);
    m_KeyRecordDialog->setWindowFlags(Qt::Popup);

    m_CrosshairSetupDialog = new QCrosshairSetupDialog(this);

    initKeyListComboBoxes();
    initKeyStringLineEdit();

    QStyle* windowsStyle = QStyleFactory::create("windows");
    ui->oriList_SelectKeyboardButton->setStyle(windowsStyle);
    ui->oriList_SelectMouseButton->setStyle(windowsStyle);
    ui->oriList_SelectGamepadButton->setStyle(windowsStyle);
    ui->oriList_SelectFunctionButton->setStyle(windowsStyle);
    ui->mapList_SelectKeyboardButton->setStyle(windowsStyle);
    ui->mapList_SelectMouseButton->setStyle(windowsStyle);
    ui->mapList_SelectGamepadButton->setStyle(windowsStyle);
    ui->mapList_SelectFunctionButton->setStyle(windowsStyle);
    ui->oriList_SelectKeyboardButton->setIcon(QIcon(":/keyboard.svg"));
    ui->oriList_SelectMouseButton->setIcon(QIcon(":/mouse.svg"));
    ui->oriList_SelectGamepadButton->setIcon(QIcon(":/gamepad.svg"));
    ui->oriList_SelectFunctionButton->setIcon(QIcon(":/function.svg"));
    ui->mapList_SelectKeyboardButton->setIcon(QIcon(":/keyboard.svg"));
    ui->mapList_SelectMouseButton->setIcon(QIcon(":/mouse.svg"));
    ui->mapList_SelectGamepadButton->setIcon(QIcon(":/gamepad.svg"));
    ui->mapList_SelectFunctionButton->setIcon(QIcon(":/function.svg"));
    ui->oriList_SelectKeyboardButton->setChecked(true);
    ui->oriList_SelectMouseButton->setChecked(true);
    ui->oriList_SelectGamepadButton->setChecked(true);
    ui->oriList_SelectFunctionButton->setChecked(true);
    ui->mapList_SelectKeyboardButton->setChecked(true);
    ui->mapList_SelectMouseButton->setChecked(true);
    ui->mapList_SelectGamepadButton->setChecked(true);
    ui->mapList_SelectFunctionButton->setChecked(true);

    ui->originalKeyLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_MappingKeyLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_MappingKey_KeyUpLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->itemNoteLineEdit->setFocusPolicy(Qt::ClickFocus);

    ui->originalKeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    m_MappingKeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    m_MappingKey_KeyUpLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->itemNoteLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->burstpressSpinBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->burstreleaseSpinBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->repeatTimesSpinBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    m_OriginalKeyListComboBox->setFont(QFont(FONTNAME_ENGLISH, 9));
    m_MappingKeyListComboBox->setFont(QFont(FONTNAME_ENGLISH, 9));

    ui->burstpressSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->burstreleaseSpinBox->setRange(BURST_TIME_MIN, BURST_TIME_MAX);
    ui->repeatTimesSpinBox->setRange(REPEAT_TIMES_MIN, REPEAT_TIMES_MAX);

    ui->fixedVKeyCodeSpinBox->setRange(FIXED_VIRTUAL_KEY_CODE_MIN, FIXED_VIRTUAL_KEY_CODE_MAX);
    ui->fixedVKeyCodeSpinBox->setPrefix("0x");
    ui->fixedVKeyCodeSpinBox->setDisplayIntegerBase(16);
    ui->fixedVKeyCodeSpinBox->setValue(FIXED_VIRTUAL_KEY_CODE_NONE);

    QStringList sendtiming_list = QStringList() \
                                  << tr("Normal")
                                  << tr("KeyDown")
                                  << tr("KeyUp")
                                  << tr("KeyDown+KeyUp")
                                  << tr("Normal+KeyUp")
        ;
    ui->sendTimingComboBox->addItems(sendtiming_list);
    ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);

    // ui->originalKeyLineEdit->setReadOnly(true);
    // ui->mappingKeyLineEdit->setReadOnly(true);
    // ui->originalKeyUpdateButton->setVisible(false);
    // ui->mappingKeyUpdateButton->setVisible(false);

    ui->keyRecordLineEdit->setReadOnly(true);
    ui->keyRecordLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->keyRecordEditModeButton->setText(tr("Edit"));

    QObject::connect(ui->originalKeyLineEdit, &QLineEdit::returnPressed, this, &QItemSetupDialog::updateMappingInfo_OriginalKeyFirst);
    QObject::connect(m_MappingKeyLineEdit, &QLineEdit::returnPressed, this, &QItemSetupDialog::updateMappingInfo_MappingKeyFirst);
    QObject::connect(m_MappingKey_KeyUpLineEdit, &QLineEdit::returnPressed, this, &QItemSetupDialog::updateMappingInfo_MappingKey_KeyUpFirst);
    // QObject::connect(ui->itemNoteLineEdit, &QLineEdit::returnPressed, this, &QItemSetupDialog::on_itemNoteUpdateButton_clicked);

    // Synchronize button states between main window and sub window:
    // Use bidirectional connect to ensure the four category buttons (Keyboard, Mouse, Gamepad, Function) in both windows always have the same checked state.
    // Any change in one window will be immediately reflected in the other, achieving full linkage.
    syncConnectOriginalKeySelectButtons();
}

QItemSetupDialog::~QItemSetupDialog()
{
    delete ui;
}

void QItemSetupDialog::setUILanguage(int languageindex)
{
    setWindowTitle(tr(ITEMSETUPDIALOG_WINDOWTITLE_STR));
    ui->disabledCheckBox->setText(tr("Disabled"));
    ui->burstCheckBox->setText(tr(BURSTCHECKBOX_STR));
    ui->lockCheckBox->setText(tr(LOCKCHECKBOX_STR));
    ui->mappingKeyUnlockCheckBox->setText(tr(MAPPINGKEYUNLOCKCHECKBOX_STR));
    ui->disableOriginalKeyUnlockCheckBox->setText(tr("DisableOriKeyUnlock"));
    ui->disableFnKeySwitchCheckBox->setText(tr("DisableFnKeySwitch"));
    ui->sendMappingKeyMethodLabel->setText(tr("SendMethod"));
    ui->checkCombKeyOrderCheckBox->setText(tr(CHECKCOMBKEYORDERCHECKBOX_STR));
    ui->unbreakableCheckBox->setText(tr(UNBREAKABLECHECKBOX_STR));
    ui->passThroughCheckBox->setText(tr(PASSTHROUGHCHECKBOX_STR));
    ui->keySeqHoldDownCheckBox->setText(tr(KEYSEQHOLDDOWNCHECKBOX_STR));
    ui->repeatByKeyCheckBox->setText(tr(REPEATBYKEYCHECKBOX_STR));
    ui->repeatByTimesCheckBox->setText(tr(REPEATBYTIMESCHECKBOX_STR));
    ui->burstpressLabel->setText(tr(BURSTPRESSLABEL_STR));
    ui->burstreleaseLabel->setText(tr(BURSTRELEASE_STR));
    ui->originalKeyLabel->setText(tr(ORIGINALKEYLABEL_STR));
    ui->mappingKeyLabel->setText(tr(MAPPINGKEYLABEL_STR));
    ui->mappingKey_KeyUpLabel->setText(tr(KEYUPMAPPINGLABEL_STR));
    ui->itemNoteLabel->setText(tr(ITEMNOTELABEL_STR));
    ui->orikeyListLabel->setText(tr(ORIKEYLISTLABEL_STR));
    ui->mapkeyListLabel->setText(tr(MAPKEYLISTLABEL_STR));
    ui->keyRecordLabel->setText(tr("KeyRecord"));
    ui->oriList_SelectKeyboardButton->setToolTip(tr("Keyboard Keys"));
    ui->oriList_SelectMouseButton->setToolTip(tr("Mouse Keys"));
    ui->oriList_SelectGamepadButton->setToolTip(tr("Gamepad Keys"));
    ui->oriList_SelectFunctionButton->setToolTip(tr("Function Keys"));
    ui->mapList_SelectKeyboardButton->setToolTip(tr("Keyboard Keys"));
    ui->mapList_SelectMouseButton->setToolTip(tr("Mouse Keys"));
    ui->mapList_SelectGamepadButton->setToolTip(tr("Gamepad Keys"));
    ui->mapList_SelectFunctionButton->setToolTip(tr("Function Keys"));
    // ui->originalKeyUpdateButton->setText(tr(UPDATEBUTTON_STR));
    // ui->mappingKeyUpdateButton->setText(tr(UPDATEBUTTON_STR));
    // ui->mappingKey_KeyUpUpdateButton->setText(tr(UPDATEBUTTON_STR));
    // ui->itemNoteUpdateButton->setText(tr(UPDATEBUTTON_STR));
    ui->updateMappingInfoButton->setText(tr("Mapping\nUpdate"));
    ui->recordKeysButton->setText(tr(RECORDKEYSBUTTON_STR));
    ui->crosshairSetupButton->setText(tr(CROSSHAIRSETUPBUTTON_STR));
    ui->sendTimingLabel->setText(tr(SENDTIMINGLABEL_STR));
    ui->fixedVKeyCodeLabel->setText(tr("FixedVKeyCode"));
    ui->pasteTextModeLabel->setText(tr("PasteTextMode"));

    if (m_ItemSetupKeyRecordEditMode == KEYRECORD_EDITMODE_MANUALEDIT) {
        ui->keyRecordEditModeButton->setText(tr("Capture"));
    }
    else {
        ui->keyRecordEditModeButton->setText(tr("Edit"));
    }
    if (!ui->keyRecordLineEdit->placeholderText().isEmpty()) {
        ui->keyRecordLineEdit->setPlaceholderText(tr("Press any key to record..."));
    }

    ui->burstpressSpinBox->setSuffix(tr(" ms"));
    ui->burstreleaseSpinBox->setSuffix(tr(" ms"));

    ui->sendTimingComboBox->setItemText(SENDTIMING_NORMAL,              tr("Normal"));
    ui->sendTimingComboBox->setItemText(SENDTIMING_KEYDOWN,             tr("KeyDown"));
    ui->sendTimingComboBox->setItemText(SENDTIMING_KEYUP,               tr("KeyUp"));
    ui->sendTimingComboBox->setItemText(SENDTIMING_KEYDOWN_AND_KEYUP,   tr("KeyDown+KeyUp"));
    ui->sendTimingComboBox->setItemText(SENDTIMING_NORMAL_AND_KEYUP,    tr("Normal+KeyUp"));

    if (m_KeyRecordDialog != Q_NULLPTR) {
        m_KeyRecordDialog->setUILanguage(languageindex);
    }
    if (m_CrosshairSetupDialog != Q_NULLPTR) {
        m_CrosshairSetupDialog->setUILanguage(languageindex);
    }
}

void QItemSetupDialog::resetFontSize()
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

    ui->disabledCheckBox->setFont(customFont);
    ui->burstCheckBox->setFont(customFont);
    ui->lockCheckBox->setFont(customFont);
    ui->mappingKeyUnlockCheckBox->setFont(customFont);
    ui->disableOriginalKeyUnlockCheckBox->setFont(customFont);
    ui->disableFnKeySwitchCheckBox->setFont(customFont);
    ui->sendMappingKeyMethodComboBox->setFont(customFont);
    ui->checkCombKeyOrderCheckBox->setFont(customFont);
    ui->unbreakableCheckBox->setFont(customFont);
    ui->passThroughCheckBox->setFont(customFont);
    ui->keySeqHoldDownCheckBox->setFont(customFont);
    ui->repeatByKeyCheckBox->setFont(customFont);
    ui->repeatByTimesCheckBox->setFont(customFont);
    ui->burstpressLabel->setFont(customFont);
    ui->burstreleaseLabel->setFont(customFont);
    ui->originalKeyLabel->setFont(customFont);
    ui->mappingKeyLabel->setFont(customFont);
    ui->mappingKey_KeyUpLabel->setFont(customFont);
    ui->itemNoteLabel->setFont(customFont);
    ui->orikeyListLabel->setFont(customFont);
    ui->mapkeyListLabel->setFont(customFont);
    // ui->originalKeyUpdateButton->setFont(customFont);
    // ui->mappingKeyUpdateButton->setFont(customFont);
    // ui->mappingKey_KeyUpUpdateButton->setFont(customFont);
    ui->updateMappingInfoButton->setFont(customFont);
    ui->recordKeysButton->setFont(customFont);
    ui->crosshairSetupButton->setFont(customFont);
    // ui->itemNoteUpdateButton->setFont(customFont);
    ui->sendTimingLabel->setFont(customFont);
    ui->sendTimingComboBox->setFont(customFont);
    ui->pasteTextModeLabel->setFont(customFont);
    ui->pasteTextModeComboBox->setFont(customFont);

    if (m_KeyRecordDialog != Q_NULLPTR) {
        m_KeyRecordDialog->resetFontSize();
    }
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

void QItemSetupDialog::updateOriginalKeyListComboBox()
{
    KeyListComboBox *orikeyComboBox = QKeyMapper::getInstance()->m_orikeyComboBox;

    m_OriginalKeyListComboBox->clear();

    const QIcon &common_icon = QKeyMapper::s_Icon_Blank;
    m_OriginalKeyListComboBox->addItem(QString());
    m_OriginalKeyListComboBox->addItem(common_icon, SEPARATOR_LONGPRESS);
    m_OriginalKeyListComboBox->addItem(common_icon, SEPARATOR_DOUBLEPRESS);
    for(int i = 1; i < orikeyComboBox->count(); i++) {
        QIcon icon = orikeyComboBox->itemIcon(i);
        QString text = orikeyComboBox->itemText(i);
        m_OriginalKeyListComboBox->addItem(icon, text);
    }
}

void QItemSetupDialog::updateMappingKeyListComboBox()
{
    KeyListComboBox *mapkeyComboBox = QKeyMapper::getInstance()->m_mapkeyComboBox;

    m_MappingKeyListComboBox->clear();

    const QIcon &common_icon = QKeyMapper::s_Icon_Blank;
    m_MappingKeyListComboBox->addItem(QString());
    m_MappingKeyListComboBox->addItem(common_icon, SEPARATOR_WAITTIME);
    m_MappingKeyListComboBox->addItem(common_icon, SEPARATOR_NEXTARROW);
    m_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_DOWN);
    m_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_UP);
    m_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_BOTH);
    m_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_EXCLUSION);
    m_MappingKeyListComboBox->addItem(common_icon, REPEAT_STR);
    for(int i = 1; i < mapkeyComboBox->count(); i++) {
        QIcon icon = mapkeyComboBox->itemIcon(i);
        QString text = mapkeyComboBox->itemText(i);
        m_MappingKeyListComboBox->addItem(icon, text);
    }
}

bool QItemSetupDialog::isCrosshairSetupDialogVisible()
{
    if (m_CrosshairSetupDialog != Q_NULLPTR
        && m_CrosshairSetupDialog->isVisible()) {
        return true;
    }
    else {
        return false;
    }
}

void QItemSetupDialog::updateKeyRecordLineEditWithRealKeyListChanged(const QString &keycodeString, int keyupdown)
{
    // Ignore mouse left clicks when cursor is over originalKeyEditModeButton to prevent unwanted input
    if (!m_isItemSetupKeyRecordLineEdit_CapturingKey
        && (keycodeString.contains("Mouse-L") || keycodeString.contains("Mouse-R"))
        && keyupdown == KEY_DOWN) {

        // Get current mouse cursor position
        QPoint globalMousePos = QCursor::pos();

        // Get originalKeyEditModeButton's global position and size
        QPoint lineEditGlobalPos = ui->keyRecordLineEdit->mapToGlobal(QPoint(0, 0));
        QRect lineEditGlobalRect(lineEditGlobalPos, ui->keyRecordLineEdit->size());

        // Check if mouse cursor is within lineedit area
        if (!lineEditGlobalRect.contains(globalMousePos)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::updateKeyLineEditWithRealKeyListChanged] Ignoring Mouse-L & Mouse-R click outside keyRecordLineEdit area";
            qDebug() << "[QKeyMapper::updateKeyLineEditWithRealKeyListChanged] Mouse pos:" << globalMousePos << "Button rect:" << lineEditGlobalRect;
#endif
            return;
        }
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QKeyMapper::updateKeyLineEditWithRealKeyListChanged]" << keycodeString << (keyupdown == KEY_DOWN?"KeyDown":"KeyUp");
    qDebug() << "[QKeyMapper::updateKeyLineEditWithRealKeyListChanged]" << "pressedRealKeysListRemoveMultiInput ->" << QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput;
#endif

    // Start capturing when first key is pressed
    if (!m_isItemSetupKeyRecordLineEdit_CapturingKey && keyupdown == KEY_DOWN) {
        ui->keyRecordLineEdit->clear();
        m_isItemSetupKeyRecordLineEdit_CapturingKey = true;
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QKeyMapper::updateKeyLineEditWithRealKeyListChanged] Start capturing keys";
#endif
    }

    if (keyupdown == KEY_DOWN) {
        // Handle key press - update the OriginalKey LineEdit display
        if (m_isItemSetupKeyRecordLineEdit_CapturingKey) {
            QStringList pressedRealKeys = QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput;
            pressedRealKeys.removeAll(GAMEPAD_HOME_STR);
            QString updatedOriginalKeyString = pressedRealKeys.join(SEPARATOR_PLUS);
            ui->keyRecordLineEdit->setText(updatedOriginalKeyString);

#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::updateKeyLineEditWithRealKeyListChanged] Updated OriginalKeyString :" << updatedOriginalKeyString;
#endif
        }
    }
    else { /* keyupdown == KEY_UP */
        if (m_isItemSetupKeyRecordLineEdit_CapturingKey && QKeyMapper_Worker::pressedRealKeysListRemoveMultiInput.isEmpty()) {
            m_isItemSetupKeyRecordLineEdit_CapturingKey = false;
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QKeyMapper::updateKeyLineEditWithRealKeyListChanged] End capturing keys, final OriginalKeyString:" << ui->keyRecordLineEdit->text();
#endif
        }
    }
}

void QItemSetupDialog::syncConnectOriginalKeySelectButtons()
{
    /* OriginalKeys List Category Buttons */
    QPushButton* oriSelectKeyboardButton    = QKeyMapper::getInstance()->getOriListSelectKeyboardButton();
    QPushButton* oriSelectMouseButton       = QKeyMapper::getInstance()->getOriListSelectMouseButton();
    QPushButton* oriSelectGamepadButton     = QKeyMapper::getInstance()->getOriListSelectGamepadButton();
    QPushButton* oriSelectFunctionButton    = QKeyMapper::getInstance()->getOriListSelectFunctionButton();
    QObject::connect(oriSelectKeyboardButton,           &QPushButton::toggled, ui->oriList_SelectKeyboardButton,    &QPushButton::setChecked);
    QObject::connect(ui->oriList_SelectKeyboardButton,  &QPushButton::toggled, oriSelectKeyboardButton,             &QPushButton::setChecked);
    QObject::connect(oriSelectMouseButton,              &QPushButton::toggled, ui->oriList_SelectMouseButton,       &QPushButton::setChecked);
    QObject::connect(ui->oriList_SelectMouseButton,     &QPushButton::toggled, oriSelectMouseButton,                &QPushButton::setChecked);
    QObject::connect(oriSelectGamepadButton,            &QPushButton::toggled, ui->oriList_SelectGamepadButton,     &QPushButton::setChecked);
    QObject::connect(ui->oriList_SelectGamepadButton,   &QPushButton::toggled, oriSelectGamepadButton,              &QPushButton::setChecked);
    QObject::connect(oriSelectFunctionButton,           &QPushButton::toggled, ui->oriList_SelectFunctionButton,    &QPushButton::setChecked);
    QObject::connect(ui->oriList_SelectFunctionButton,  &QPushButton::toggled, oriSelectFunctionButton,             &QPushButton::setChecked);
}

void QItemSetupDialog::syncConnectMappingKeySelectButtons()
{
    /* MappingKeys List Category Buttons - QKeyMapper Main Window */
    QPushButton* mapSelectKeyboardButton    = QKeyMapper::getInstance()->getMapListSelectKeyboardButton();
    QPushButton* mapSelectMouseButton       = QKeyMapper::getInstance()->getMapListSelectMouseButton();
    QPushButton* mapSelectGamepadButton     = QKeyMapper::getInstance()->getMapListSelectGamepadButton();
    QPushButton* mapSelectFunctionButton    = QKeyMapper::getInstance()->getMapListSelectFunctionButton();

    /* MappingKeys List Category Buttons - QMacroListDialog Window */
    QPushButton* macroMapSelectKeyboardButton   = QMacroListDialog::getInstance()->getMapListSelectKeyboardButton();
    QPushButton* macroMapSelectMouseButton      = QMacroListDialog::getInstance()->getMapListSelectMouseButton();
    QPushButton* macroMapSelectGamepadButton    = QMacroListDialog::getInstance()->getMapListSelectGamepadButton();
    QPushButton* macroMapSelectFunctionButton   = QMacroListDialog::getInstance()->getMapListSelectFunctionButton();

    /* Sync QKeyMapper Main Window <-> QItemSetupDialog Window */
    QObject::connect(mapSelectKeyboardButton,           &QPushButton::toggled, ui->mapList_SelectKeyboardButton,    &QPushButton::setChecked);
    QObject::connect(ui->mapList_SelectKeyboardButton,  &QPushButton::toggled, mapSelectKeyboardButton,             &QPushButton::setChecked);
    QObject::connect(mapSelectMouseButton,              &QPushButton::toggled, ui->mapList_SelectMouseButton,       &QPushButton::setChecked);
    QObject::connect(ui->mapList_SelectMouseButton,     &QPushButton::toggled, mapSelectMouseButton,                &QPushButton::setChecked);
    QObject::connect(mapSelectGamepadButton,            &QPushButton::toggled, ui->mapList_SelectGamepadButton,     &QPushButton::setChecked);
    QObject::connect(ui->mapList_SelectGamepadButton,   &QPushButton::toggled, mapSelectGamepadButton,              &QPushButton::setChecked);
    QObject::connect(mapSelectFunctionButton,           &QPushButton::toggled, ui->mapList_SelectFunctionButton,    &QPushButton::setChecked);
    QObject::connect(ui->mapList_SelectFunctionButton,  &QPushButton::toggled, mapSelectFunctionButton,             &QPushButton::setChecked);

    /* Sync QKeyMapper Main Window <-> QMacroListDialog Window */
    QObject::connect(mapSelectKeyboardButton,               &QPushButton::toggled, macroMapSelectKeyboardButton,    &QPushButton::setChecked);
    QObject::connect(macroMapSelectKeyboardButton,          &QPushButton::toggled, mapSelectKeyboardButton,         &QPushButton::setChecked);
    QObject::connect(mapSelectMouseButton,                  &QPushButton::toggled, macroMapSelectMouseButton,       &QPushButton::setChecked);
    QObject::connect(macroMapSelectMouseButton,             &QPushButton::toggled, mapSelectMouseButton,            &QPushButton::setChecked);
    QObject::connect(mapSelectGamepadButton,                &QPushButton::toggled, macroMapSelectGamepadButton,     &QPushButton::setChecked);
    QObject::connect(macroMapSelectGamepadButton,           &QPushButton::toggled, mapSelectGamepadButton,          &QPushButton::setChecked);
    QObject::connect(mapSelectFunctionButton,               &QPushButton::toggled, macroMapSelectFunctionButton,    &QPushButton::setChecked);
    QObject::connect(macroMapSelectFunctionButton,          &QPushButton::toggled, mapSelectFunctionButton,         &QPushButton::setChecked);

    /* Sync QItemSetupDialog Window <-> QMacroListDialog Window */
    QObject::connect(ui->mapList_SelectKeyboardButton,      &QPushButton::toggled, macroMapSelectKeyboardButton,    &QPushButton::setChecked);
    QObject::connect(macroMapSelectKeyboardButton,          &QPushButton::toggled, ui->mapList_SelectKeyboardButton,&QPushButton::setChecked);
    QObject::connect(ui->mapList_SelectMouseButton,         &QPushButton::toggled, macroMapSelectMouseButton,       &QPushButton::setChecked);
    QObject::connect(macroMapSelectMouseButton,             &QPushButton::toggled, ui->mapList_SelectMouseButton,   &QPushButton::setChecked);
    QObject::connect(ui->mapList_SelectGamepadButton,       &QPushButton::toggled, macroMapSelectGamepadButton,     &QPushButton::setChecked);
    QObject::connect(macroMapSelectGamepadButton,           &QPushButton::toggled, ui->mapList_SelectGamepadButton, &QPushButton::setChecked);
    QObject::connect(ui->mapList_SelectFunctionButton,      &QPushButton::toggled, macroMapSelectFunctionButton,    &QPushButton::setChecked);
    QObject::connect(macroMapSelectFunctionButton,          &QPushButton::toggled, ui->mapList_SelectFunctionButton,&QPushButton::setChecked);
}

QPushButton *QItemSetupDialog::getOriListSelectKeyboardButton() const
{
    return ui->oriList_SelectKeyboardButton;
}

QPushButton *QItemSetupDialog::getOriListSelectMouseButton() const
{
    return ui->oriList_SelectMouseButton;
}

QPushButton *QItemSetupDialog::getOriListSelectGamepadButton() const
{
    return ui->oriList_SelectGamepadButton;
}

QPushButton *QItemSetupDialog::getOriListSelectFunctionButton() const
{
    return ui->oriList_SelectFunctionButton;
}

QPushButton *QItemSetupDialog::getMapListSelectKeyboardButton() const
{
    return ui->mapList_SelectKeyboardButton;
}

QPushButton *QItemSetupDialog::getMapListSelectMouseButton() const
{
    return ui->mapList_SelectMouseButton;
}

QPushButton *QItemSetupDialog::getMapListSelectGamepadButton() const
{
    return ui->mapList_SelectGamepadButton;
}

QPushButton *QItemSetupDialog::getMapListSelectFunctionButton() const
{
    return ui->mapList_SelectFunctionButton;
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
    if (ITEMSETUP_EDITING_KEYUPMAPPINGKEY == s_editingMappingKeyLineEdit) {
        return getInstance()->m_MappingKey_KeyUpLineEdit->text();
    }
    else {
        return getInstance()->m_MappingKeyLineEdit->text();
    }
}

int QItemSetupDialog::getMappingKeyCursorPosition()
{
    if (ITEMSETUP_EDITING_KEYUPMAPPINGKEY == s_editingMappingKeyLineEdit) {
        return getInstance()->m_MappingKey_KeyUpLineEdit->cursorPosition();
    }
    else {
        return getInstance()->m_MappingKeyLineEdit->cursorPosition();
    }
}

void QItemSetupDialog::setMappingKeyText(const QString &new_keytext)
{
    if (ITEMSETUP_EDITING_KEYUPMAPPINGKEY == s_editingMappingKeyLineEdit) {
        return getInstance()->m_MappingKey_KeyUpLineEdit->setText(new_keytext);
    }
    else {
        return getInstance()->m_MappingKeyLineEdit->setText(new_keytext);
    }
}

QString QItemSetupDialog::getCurrentOriKeyListText()
{
    return getInstance()->m_OriginalKeyListComboBox->currentText();
}

QString QItemSetupDialog::getCurrentMapKeyListText()
{
    return getInstance()->m_MappingKeyListComboBox->currentText();
}

void QItemSetupDialog::setEditingMappingKeyLineEdit(int editing_lineedit)
{
    if (ITEMSETUP_EDITING_KEYUPMAPPINGKEY == editing_lineedit) {
        s_editingMappingKeyLineEdit = ITEMSETUP_EDITING_KEYUPMAPPINGKEY;

        getInstance()->m_MappingKey_KeyUpLineEdit->setStyleSheet("QLineEdit { border: 2px solid #546de5; }");
        getInstance()->m_MappingKeyLineEdit->setStyleSheet("");
    }
    else {
        s_editingMappingKeyLineEdit = ITEMSETUP_EDITING_MAPPINGKEY;

        getInstance()->m_MappingKeyLineEdit->setStyleSheet("QLineEdit { border: 2px solid #546de5; }");
        getInstance()->m_MappingKey_KeyUpLineEdit->setStyleSheet("");
    }
}

QLineEdit *QItemSetupDialog::getKeyRecordLineEdit()
{
    return ui->keyRecordLineEdit;
}

#if 0
QPair<QString, QStringList> QItemSetupDialog::extractSendTextWithBracketBalancing(const QString &mappingKey, const QRegularExpression &sendtext_regex)
{
    QStringList sendTextParts;
    QString tempMappingKey = mappingKey;
    int currentPos = 0;
    int replacementIndex = 0;

    while (currentPos < mappingKey.length()) {
        QRegularExpressionMatch match = sendtext_regex.match(mappingKey, currentPos);
        if (!match.hasMatch()) {
            break;
        }

        // Check if brackets are balanced in the captured content
        QString functionName = match.captured(1);   // "SendText" or "PasteText"
        QString captured = match.captured(2);       // Text content
        int bracketCount = 0;
        bool isBalanced = true;

        for (const QChar &ch : std::as_const(captured)) {
            if (ch == '(') bracketCount++;
            else if (ch == ')') bracketCount--;
            if (bracketCount < 0) {
                isBalanced = false;
                break;
            }
        }
        isBalanced = isBalanced && (bracketCount == 0);

        if (isBalanced) {
            // Brackets are balanced, use this match
            QString placeholder = QString("__SENDTEXT_PLACEHOLDER_%1__").arg(replacementIndex++);
            sendTextParts.append(match.captured(0)); // Store the entire SendText(...) or PasteText(...) part
            tempMappingKey.replace(match.captured(0), placeholder);
            currentPos = match.capturedEnd();
        } else {
            // Brackets not balanced, try to find the correct closing bracket
            int startPos = match.capturedStart();
            // Skip function name length: "SendText" = 8, "PasteText" = 9
            int skipLength = functionName.length();
            int openPos = mappingKey.indexOf('(', startPos + skipLength);
            if (openPos != -1) {
                int closePos = openPos + 1;
                int depth = 1;

                while (closePos < mappingKey.length() && depth > 0) {
                    if (mappingKey[closePos] == '(') depth++;
                    else if (mappingKey[closePos] == ')') depth--;
                    closePos++;
                }

                if (depth == 0) {
                    // Found balanced brackets
                    QString fullMatch = mappingKey.mid(startPos, closePos - startPos);
                    QString placeholder = QString("__SENDTEXT_PLACEHOLDER_%1__").arg(replacementIndex++);
                    sendTextParts.append(fullMatch);
                    tempMappingKey.replace(fullMatch, placeholder);
                    currentPos = closePos;
                } else {
                    // Still unbalanced, move past this match
                    currentPos = match.capturedEnd();
                }
            } else {
                currentPos = match.capturedEnd();
            }
        }
    }

    return qMakePair(tempMappingKey, sendTextParts);
}
#endif

QPair<QString, QStringList> QItemSetupDialog::extractSpecialPatternsWithBracketBalancing(const QString &mappingKey, const QRegularExpression &sendtext_regex, const QRegularExpression &run_regex, const QRegularExpression &switchtab_regex, const QRegularExpression &unlock_regex, const QRegularExpression &setvolume_regex, const QRegularExpression &repeat_regex, const QRegularExpression &macro_regex)
{
    QStringList preservedParts;
    QString tempMappingKey = mappingKey;

    // Create a list of all matches (SendText, Run, SwitchTab, Unlock, SetVolume, Repeat, and Macro) with their positions
    struct MatchInfo {
        int start;
        int end;
        QString content;
        QString type; // "sendtext", "run", "switchtab", "switchtab_save", "unlock", "setvolume", "repeat", "macro", or "universalmacro"
    };

    QList<MatchInfo> allMatches;

    // Find all SendText matches
    int currentPos = 0;
    while (currentPos < mappingKey.length()) {
        if (sendtext_regex.pattern().isEmpty()) {
            break;
        }

        QRegularExpressionMatch match = sendtext_regex.match(mappingKey, currentPos);
        if (!match.hasMatch()) {
            break;
        }

        // Check if brackets are balanced in the captured content for SendText/PasteText
        QString functionName = match.captured(1);   // "SendText" or "PasteText"
        QString captured = match.captured(2);       // Text content
        int bracketCount = 0;
        bool isBalanced = true;

        for (const QChar &ch : std::as_const(captured)) {
            if (ch == '(') bracketCount++;
            else if (ch == ')') bracketCount--;
            if (bracketCount < 0) {
                isBalanced = false;
                break;
            }
        }
        isBalanced = isBalanced && (bracketCount == 0);

        if (isBalanced) {
            // Brackets are balanced, use this match
            MatchInfo info;
            info.start = match.capturedStart();
            info.end = match.capturedEnd();
            info.content = match.captured(0);
            info.type = "sendtext";  // Keep type as "sendtext" for both SendText and PasteText
            allMatches.append(info);
            currentPos = match.capturedEnd();
        } else {
            // Brackets not balanced, try to find the correct closing bracket
            int startPos = match.capturedStart();
            // Skip function name length: "SendText" = 8, "PasteText" = 9
            int skipLength = functionName.length();
            int openPos = mappingKey.indexOf('(', startPos + skipLength);
            if (openPos != -1) {
                int closePos = openPos + 1;
                int depth = 1;

                while (closePos < mappingKey.length() && depth > 0) {
                    if (mappingKey[closePos] == '(') depth++;
                    else if (mappingKey[closePos] == ')') depth--;
                    closePos++;
                }

                if (depth == 0) {
                    // Found balanced brackets
                    MatchInfo info;
                    info.start = startPos;
                    info.end = closePos;
                    info.content = mappingKey.mid(startPos, closePos - startPos);
                    info.type = "sendtext";
                    allMatches.append(info);
                    currentPos = closePos;
                } else {
                    // Still unbalanced, move past this match
                    currentPos = match.capturedEnd();
                }
            } else {
                currentPos = match.capturedEnd();
            }
        }
    }

    // Find all Run matches (need bracket balancing like SendText)
    currentPos = 0;
    while (currentPos < mappingKey.length()) {
        if (run_regex.pattern().isEmpty()) {
            break;
        }

        QRegularExpressionMatch match = run_regex.match(mappingKey, currentPos);
        if (!match.hasMatch()) {
            break;
        }

        // Check if brackets are balanced in the captured content for Run
        QString captured = match.captured(1);
        int bracketCount = 0;
        bool isBalanced = true;

        for (const QChar &ch : std::as_const(captured)) {
            if (ch == '(') bracketCount++;
            else if (ch == ')') bracketCount--;
            if (bracketCount < 0) {
                isBalanced = false;
                break;
            }
        }
        isBalanced = isBalanced && (bracketCount == 0);

        if (isBalanced) {
            // Brackets are balanced, use this match
            MatchInfo info;
            info.start = match.capturedStart();
            info.end = match.capturedEnd();
            info.content = match.captured(0);
            info.type = "run";
            allMatches.append(info);
            currentPos = match.capturedEnd();
        } else {
            // Brackets not balanced, try to find the correct closing bracket
            int startPos = match.capturedStart();
            int openPos = mappingKey.indexOf('(', startPos + 3); // Skip "Run"
            if (openPos != -1) {
                int closePos = openPos + 1;
                int depth = 1;

                while (closePos < mappingKey.length() && depth > 0) {
                    if (mappingKey[closePos] == '(') depth++;
                    else if (mappingKey[closePos] == ')') depth--;
                    closePos++;
                }

                if (depth == 0) {
                    // Found balanced brackets
                    MatchInfo info;
                    info.start = startPos;
                    info.end = closePos;
                    info.content = mappingKey.mid(startPos, closePos - startPos);
                    info.type = "run";
                    allMatches.append(info);
                    currentPos = closePos;
                } else {
                    // Still unbalanced, move past this match
                    currentPos = match.capturedEnd();
                }
            } else {
                currentPos = match.capturedEnd();
            }
        }
    }

    // Find all SwitchTab matches (need bracket balancing like Run)
    currentPos = 0;
    while (currentPos < mappingKey.length()) {
        if (switchtab_regex.pattern().isEmpty()) {
            break;
        }

        QRegularExpressionMatch match = switchtab_regex.match(mappingKey, currentPos);
        if (!match.hasMatch()) {
            break;
        }

        // Check if brackets are balanced in the captured content for SwitchTab
        // Note: For new pattern SwitchTab(💾)?\((.+?)\), captured(1) is optional 💾, captured(2) is content
        QString captured = match.captured(2);
        int bracketCount = 0;
        bool isBalanced = true;

        for (const QChar &ch : std::as_const(captured)) {
            if (ch == '(') bracketCount++;
            else if (ch == ')') bracketCount--;
            if (bracketCount < 0) {
                isBalanced = false;
                break;
            }
        }
        isBalanced = isBalanced && (bracketCount == 0);

        if (isBalanced) {
            // Brackets are balanced, use this match
            MatchInfo info;
            info.start = match.capturedStart();
            info.end = match.capturedEnd();
            info.content = match.captured(0);
            // Distinguish between SwitchTab(...) and SwitchTab💾(...) to preserve original format
            if (!match.captured(1).isEmpty()) {
                info.type = "switchtab_save"; // SwitchTab💾(...) format
            } else {
                info.type = "switchtab"; // SwitchTab(...) format
            }
            allMatches.append(info);
            currentPos = match.capturedEnd();
        } else {
            // Brackets not balanced, try to find the correct closing bracket
            int startPos = match.capturedStart();
            // Find the first opening bracket in the matched text (after "SwitchTab" with or without 💾)
            QString matchedText = match.captured(0);
            int openPos = matchedText.indexOf('(');
            if (openPos != -1) {
                openPos += startPos; // Convert to absolute position
                int closePos = openPos + 1;
                int depth = 1;

                while (closePos < mappingKey.length() && depth > 0) {
                    if (mappingKey[closePos] == '(') depth++;
                    else if (mappingKey[closePos] == ')') depth--;
                    closePos++;
                }

                if (depth == 0) {
                    // Found balanced brackets
                    MatchInfo info;
                    info.start = startPos;
                    info.end = closePos;
                    info.content = mappingKey.mid(startPos, closePos - startPos);
                    // Distinguish between SwitchTab(...) and SwitchTab💾(...) to preserve original format
                    if (!match.captured(1).isEmpty()) {
                        info.type = "switchtab_save"; // SwitchTab💾(...) format
                    } else {
                        info.type = "switchtab"; // SwitchTab(...) format
                    }
                    allMatches.append(info);
                    currentPos = closePos;
                } else {
                    // Still unbalanced, move past this match
                    currentPos = match.capturedEnd();
                }
            } else {
                currentPos = match.capturedEnd();
            }
        }
    }

    // Find all Unlock matches (simple pattern without bracket balancing issues)
    currentPos = 0;
    while (currentPos < mappingKey.length()) {
        if (unlock_regex.pattern().isEmpty()) {
            break;
        }

        QRegularExpressionMatch match = unlock_regex.match(mappingKey, currentPos);
        if (!match.hasMatch()) {
            break;
        }

        // Unlock pattern is simple: Unlock(...) with no nested brackets expected
        MatchInfo info;
        info.start = match.capturedStart();
        info.end = match.capturedEnd();
        info.content = match.captured(0);
        info.type = "unlock";
        allMatches.append(info);
        currentPos = match.capturedEnd();
    }

    // Find all SetVolume matches (simple pattern without bracket balancing issues)
    currentPos = 0;
    while (currentPos < mappingKey.length()) {
        if (setvolume_regex.pattern().isEmpty()) {
            break;
        }

        QRegularExpressionMatch match = setvolume_regex.match(mappingKey, currentPos);
        if (!match.hasMatch()) {
            break;
        }

        // SetVolume pattern is simple: SetVolume(...) with no nested brackets expected
        MatchInfo info;
        info.start = match.capturedStart();
        info.end = match.capturedEnd();
        info.content = match.captured(0);
        info.type = "setvolume";
        allMatches.append(info);
        currentPos = match.capturedEnd();
    }

    // Find all Repeat{...}x... matches with brace balancing
    // Note: Unlike SendText/Run, we don't modify the content, just preserve the whole pattern during splitting
    currentPos = 0;
    while (currentPos < mappingKey.length()) {
        if (repeat_regex.pattern().isEmpty()) {
            break;
        }

        // Find "Repeat{" pattern start
        int repeatStart = mappingKey.indexOf("Repeat{", currentPos);
        if (repeatStart == -1) {
            break;
        }

        // Find the matching closing brace with balance checking
        int bracePos = repeatStart + 7; // Position after "Repeat{"
        int depth = 1;
        while (bracePos < mappingKey.length() && depth > 0) {
            if (mappingKey[bracePos] == '{') depth++;
            else if (mappingKey[bracePos] == '}') depth--;
            bracePos++;
        }

        if (depth == 0) {
            // Found balanced braces, now look for the "x<count>" part
            // Note: Don't use ^ anchor because we're matching from an offset position
            QRegularExpression countPattern(R"(x(\d+))");
            QRegularExpressionMatch countMatch = countPattern.match(mappingKey, bracePos);

            if (countMatch.hasMatch()) {
                // Complete Repeat{...}x<count> pattern found
                MatchInfo info;
                info.start = repeatStart;
                info.end = countMatch.capturedEnd();
                info.content = mappingKey.mid(repeatStart, info.end - repeatStart);
                info.type = "repeat";
                allMatches.append(info);
                currentPos = info.end;
            } else {
                // No valid "x<count>" found, move past this Repeat{
                currentPos = bracePos;
            }
        } else {
            // Unbalanced braces, move past this Repeat{
            currentPos = repeatStart + 7;
        }
    }

    // Find all Macro(...) and UniversalMacro(...) matches
    // Pattern: Macro(name) or Macro(name)x<count> or UniversalMacro(name) or UniversalMacro(name)x<count>
    currentPos = 0;
    while (currentPos < mappingKey.length()) {
        if (macro_regex.pattern().isEmpty()) {
            break;
        }

        QRegularExpressionMatch match = macro_regex.match(mappingKey, currentPos);
        if (!match.hasMatch()) {
            break;
        }

        // Macro/UniversalMacro pattern is simple with no nested brackets expected
        MatchInfo info;
        info.start = match.capturedStart();
        info.end = match.capturedEnd();
        info.content = match.captured(0);
        // Determine if it's Macro or UniversalMacro based on the captured group(1)
        if (!match.captured(1).isEmpty()) {
            info.type = "universalmacro"; // UniversalMacro(...) format
        } else {
            info.type = "macro"; // Macro(...) format
        }
        allMatches.append(info);
        currentPos = match.capturedEnd();
    }

    // Sort matches by start position
    std::sort(allMatches.begin(), allMatches.end(), [](const MatchInfo &a, const MatchInfo &b) {
        return a.start < b.start;
    });

    // Remove overlapping matches (keep the outermost/first match)
    // This handles cases like: Repeat{Run(...)»SendText(...)»A}x2
    // where Run(...) and SendText(...) are inside Repeat{...}
    QList<MatchInfo> filteredMatches;
    for (int i = 0; i < allMatches.size(); ++i) {
        bool isOverlapped = false;
        for (int j = 0; j < filteredMatches.size(); ++j) {
            // Check if current match is completely inside a previously added match
            if (allMatches[i].start >= filteredMatches[j].start && allMatches[i].end <= filteredMatches[j].end) {
                isOverlapped = true;
                break;
            }
            // Check if current match overlaps with a previously added match
            if (allMatches[i].start < filteredMatches[j].end && allMatches[i].end > filteredMatches[j].start) {
                isOverlapped = true;
                break;
            }
        }
        if (!isOverlapped) {
            filteredMatches.append(allMatches[i]);
        }
    }
    allMatches = filteredMatches;

    // Store content in order and create a map for placeholder-to-content mapping
    static QRegularExpression simplified_regex(R"([\r\n]+)");
    static QRegularExpression whitespace_reg(R"(\s+)");
    preservedParts.clear();
    for (int i = 0; i < allMatches.size(); ++i) {
        QString content = allMatches[i].content;

        // Apply custom simplified() to Run(...) and SwitchTab(...) content only, keep SendText(...) unchanged
        if (allMatches[i].type == "run") {
            // Extract the content inside Run(...) and apply custom simplified()
            // Format: Run(content) -> find content between first ( and last )
            int firstParen = content.indexOf('(');
            int lastParen = content.lastIndexOf(')');
            if (firstParen != -1 && lastParen != -1 && firstParen < lastParen) {
                QString innerContent = content.mid(firstParen + 1, lastParen - firstParen - 1);
                innerContent.replace(simplified_regex, " ");
                QString simplifiedInnerContent = innerContent.trimmed();
                content = QString("%1(%2)").arg(RUN_STR, simplifiedInnerContent);
            }
        }
        else if (allMatches[i].type == "switchtab") {
            // Extract the content inside SwitchTab(...) and apply custom simplified()
            // Format: SwitchTab(content) -> find content between first ( and last )
            int firstParen = content.indexOf('(');
            int lastParen = content.lastIndexOf(')');
            if (firstParen != -1 && lastParen != -1 && firstParen < lastParen) {
                QString innerContent = content.mid(firstParen + 1, lastParen - firstParen - 1);
                innerContent.replace(simplified_regex, " ");
                // Don't trim to preserve user's spacing - only replace line breaks
                content = QString("%1(%2)").arg(SWITCHTAB_STR, innerContent);
            }
        }
        else if (allMatches[i].type == "switchtab_save") {
            // Extract the content inside SwitchTab💾(...) and apply custom simplified()
            // Format: SwitchTab💾(content) -> find content between first ( and last )
            int firstParen = content.indexOf('(');
            int lastParen = content.lastIndexOf(')');
            if (firstParen != -1 && lastParen != -1 && firstParen < lastParen) {
                QString innerContent = content.mid(firstParen + 1, lastParen - firstParen - 1);
                innerContent.replace(simplified_regex, " ");
                // Don't trim to preserve user's spacing - only replace line breaks
                content = QString("%1(%2)").arg(SWITCHTAB_SAVE_STR, innerContent);
            }
        }
        else if (allMatches[i].type == "unlock") {
            // Extract the content inside Unlock(...) and apply custom simplified()
            // Format: Unlock(content) -> find content between first ( and last )
            int firstParen = content.indexOf('(');
            int lastParen = content.lastIndexOf(')');
            if (firstParen != -1 && lastParen != -1 && firstParen < lastParen) {
                QString innerContent = content.mid(firstParen + 1, lastParen - firstParen - 1);
                innerContent = innerContent.simplified();
                innerContent.remove(whitespace_reg);
                content = QString("%1(%2)").arg(UNLOCK_STR, innerContent);
            }
        }
        else if (allMatches[i].type == "setvolume") {
            // SetVolume(...) content remains unchanged as it has strict format validation
            // Format: SetVolume(numeric_value) - no whitespace manipulation needed
        }
        else if (allMatches[i].type == "repeat") {
            // Repeat{...}x... needs recursive processing to remove whitespace from inner mapping keys
            // Extract the pattern: Repeat{inner_content}x<count>
            static QRegularExpression repeatPattern(REGEX_PATTERN_REPEAT);
            QRegularExpressionMatch repeatMatch = repeatPattern.match(content);

            if (repeatMatch.hasMatch()) {
                QString innerContent = repeatMatch.captured(1);  // Extract {...} content
                QString repeatCount = repeatMatch.captured(2);   // Extract x<count>

#if 0
                // Normalize whitespace between keywords and their brackets before recursive processing
                // This ensures regex patterns can match correctly: "SendText  (" -> "SendText("
                static QRegularExpression keywordSpacesRegex(R"((SendText|Run|SwitchTab💾|SwitchTab|Unlock|SetVolume|Repeat)\s+([({]))");
                innerContent.replace(keywordSpacesRegex, R"(\1\2)");
#endif

                // Recursively process inner content to handle nested Repeat and remove whitespace
                QPair<QString, QStringList> innerResult = extractSpecialPatternsWithBracketBalancing(
                    innerContent, sendtext_regex, run_regex, switchtab_regex, unlock_regex, setvolume_regex, repeat_regex, macro_regex
                );
                QString processedInner = innerResult.first;
                QStringList innerPreservedParts = innerResult.second;

                // Remove whitespace from the processed inner content
                static QRegularExpression whitespace_reg(R"(\s+)");
                processedInner.remove(whitespace_reg);

                // Restore preserved parts in inner content
                for (int j = 0; j < innerPreservedParts.size(); ++j) {
                    QString innerPlaceholder = QString("__PRESERVED_PLACEHOLDER_%1__").arg(j);
                    processedInner.replace(innerPlaceholder, innerPreservedParts[j]);
                }

                // Reconstruct the Repeat pattern with processed inner content
                content = QString("Repeat{%1}x%2").arg(processedInner, repeatCount);
            }
            // If pattern doesn't match, keep original content unchanged
        }
        else if (allMatches[i].type == "macro") {
            // Macro(...) content - remove whitespace from macro name
            // Format: Macro(macroName) or Macro(macroName)x<count>
            static QRegularExpression macroPattern(REGEX_PATTERN_MACRO);
            QRegularExpressionMatch macroMatch = macroPattern.match(content);
            if (macroMatch.hasMatch()) {
                QString macroName = macroMatch.captured(2).simplified();
                macroName.remove(whitespace_reg);
                QString repeatCount = macroMatch.captured(3);
                if (repeatCount.isEmpty()) {
                    content = QString("%1(%2)").arg(MACRO_STR, macroName);
                } else {
                    content = QString("%1(%2)x%3").arg(MACRO_STR, macroName, repeatCount);
                }
            }
        }
        else if (allMatches[i].type == "universalmacro") {
            // UniversalMacro(...) content - remove whitespace from macro name
            // Format: UniversalMacro(macroName) or UniversalMacro(macroName)x<count>
            static QRegularExpression macroPattern(REGEX_PATTERN_MACRO);
            QRegularExpressionMatch macroMatch = macroPattern.match(content);
            if (macroMatch.hasMatch()) {
                QString macroName = macroMatch.captured(2).simplified();
                macroName.remove(whitespace_reg);
                QString repeatCount = macroMatch.captured(3);
                if (repeatCount.isEmpty()) {
                    content = QString("%1(%2)").arg(UNIVERSAL_MACRO_STR, macroName);
                } else {
                    content = QString("%1(%2)x%3").arg(UNIVERSAL_MACRO_STR, macroName, repeatCount);
                }
            }
        }
        // SendText(...) content remains completely unchanged

        preservedParts.append(content);
    }

    // Replace all matches with placeholders (from end to start to maintain positions)
    for (int i = allMatches.size() - 1; i >= 0; --i) {
        const MatchInfo &info = allMatches[i];
        QString placeholder = QString("__PRESERVED_PLACEHOLDER_%1__").arg(i);
        tempMappingKey.replace(info.start, info.end - info.start, placeholder);
    }

    return qMakePair(tempMappingKey, preservedParts);
}

#ifndef CLOSE_SETUPDIALOG_ONDATACHANGED
bool QItemSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            if (isCrosshairSetupDialogVisible()
                || QKeyMapper::isSelectColorDialogVisible()) {
            }
            else {
                close();
            }
        }
    }
    return QDialog::event(event);
}
#endif

void QItemSetupDialog::closeEvent(QCloseEvent *event)
{
    m_ItemSetupKeyRecordEditMode = QKeyMapperConstants::KEYRECORD_EDITMODE_CAPTURE;
    m_isItemSetupKeyRecordLineEdit_CapturingKey = false;
    ui->keyRecordLineEdit->clear();
    ui->keyRecordLineEdit->setReadOnly(true);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::closeEvent]" << "Item Row initialize to -1, Tab Index initialize to -1";
#endif
    m_ItemRow = -1;
    m_TabIndex = -1;

    QWidget *focusedWidget = focusWidget();
    if (focusedWidget && focusedWidget != this) {
        focusedWidget->clearFocus();
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QItemSetupDialog::closeEvent]" << "Clear current Focus on closed.";
#endif
    }

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
        m_MappingKeyLineEdit->setText(mappingkeys_str);

        /* Load KeyUp MappingKeys String */
        if (keymapdata.MappingKeys_KeyUp.isEmpty()) {
            keymapdata.MappingKeys_KeyUp = keymapdata.Mapping_Keys;
            m_MappingKey_KeyUpLineEdit->setText(mappingkeys_str);
        }
        else {
            QString keyup_mappingkeys_str = keymapdata.MappingKeys_KeyUp.join(SEPARATOR_NEXTARROW);
            m_MappingKey_KeyUpLineEdit->setText(keyup_mappingkeys_str);
        }

        /* Load Note String */
        ui->itemNoteLineEdit->setText(keymapdata.Note);

        /* Load Disabled */
        if (true == keymapdata.Disabled) {
            ui->disabledCheckBox->setChecked(true);
        }
        else {
            ui->disabledCheckBox->setChecked(false);
        }

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
            ui->mappingKeyUnlockCheckBox->setEnabled(true);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(true);
        }
        else {
            ui->lockCheckBox->setChecked(false);
            ui->mappingKeyUnlockCheckBox->setEnabled(false);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(false);
        }

        /* Load MappingKeyUnlock */
        if (true == keymapdata.MappingKeyUnlock) {
            ui->mappingKeyUnlockCheckBox->setChecked(true);
        }
        else {
            ui->mappingKeyUnlockCheckBox->setChecked(false);
        }

        /* Load DisableOriginalKeyUnlock */
        if (true == keymapdata.DisableOriginalKeyUnlock) {
            ui->disableOriginalKeyUnlockCheckBox->setChecked(true);
        }
        else {
            ui->disableOriginalKeyUnlockCheckBox->setChecked(false);
        }

        /* Load DisableFnKeySwitchCheckBox */
        if (true == keymapdata.DisableFnKeySwitch) {
            ui->disableFnKeySwitchCheckBox->setChecked(true);
        }
        else {
            ui->disableFnKeySwitchCheckBox->setChecked(false);
        }

        /* Load SendMappingKeyMethod State */
        if (SENDMAPPINGKEY_METHOD_SENDMESSAGE == keymapdata.SendMappingKeyMethod) {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_SENDMESSAGE);
        }
        else if (SENDMAPPINGKEY_METHOD_FAKERINPUT == keymapdata.SendMappingKeyMethod) {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_FAKERINPUT);
        }
        else {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_SENDINPUT);
        }

        /* Load FixedVKeyCode */
        if (FIXED_VIRTUAL_KEY_CODE_MIN <= keymapdata.FixedVKeyCode && keymapdata.FixedVKeyCode <= FIXED_VIRTUAL_KEY_CODE_MAX) {
            ui->fixedVKeyCodeSpinBox->setValue(keymapdata.FixedVKeyCode);
        }
        else {
            ui->fixedVKeyCodeSpinBox->setValue(FIXED_VIRTUAL_KEY_CODE_NONE);
        }

        /* Load PasteTextMode State */
        if (PASTETEXT_MODE_CTRLV == keymapdata.PasteTextMode) {
            ui->pasteTextModeComboBox->setCurrentIndex(PASTETEXT_MODE_CTRLV);
        }
        else {
            ui->pasteTextModeComboBox->setCurrentIndex(PASTETEXT_MODE_SHIFTINSERT);
        }

        /* Load SendTiming State */
        if (SENDTIMING_KEYDOWN == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYDOWN);
        }
        else if (SENDTIMING_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYUP);
        }
        else if (SENDTIMING_KEYDOWN_AND_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYDOWN_AND_KEYUP);
        }
        else if (SENDTIMING_NORMAL_AND_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL_AND_KEYUP);
        }
        else {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);
        }

        bool isSendTimingValid = QKeyMapper::validateSendTimingByKeyMapData(keymapdata);
        if (!isSendTimingValid) {
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].SendTiming = SENDTIMING_NORMAL;
            keymapdata.SendTiming = SENDTIMING_NORMAL;
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);
            ui->sendTimingComboBox->setEnabled(false);
        }
        else {
            ui->sendTimingComboBox->setEnabled(true);
        }

        /* Only SENDTIMING_NORMAL enable KeySeqHoldDown */
        if (SENDTIMING_NORMAL == keymapdata.SendTiming) {
            if (keymapdata.Mapping_Keys.size() > 1) {
                ui->keySeqHoldDownCheckBox->setEnabled(true);
            }
            else {
                ui->keySeqHoldDownCheckBox->setEnabled(false);
                keymapdata.KeySeqHoldDown = false;
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
            }
        }
        else {
            ui->keySeqHoldDownCheckBox->setEnabled(false);
            keymapdata.KeySeqHoldDown = false;
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
        }

        /* Update CheckCombKeyOrder Enable Status & Load CheckCombKeyOrder Status */
        if (keymapdata.Original_Key.contains(SEPARATOR_PLUS)) {
            ui->checkCombKeyOrderCheckBox->setEnabled(true);
        }
        else {
            ui->checkCombKeyOrderCheckBox->setEnabled(false);
        }
        if (true == keymapdata.CheckCombKeyOrder) {
            ui->checkCombKeyOrderCheckBox->setChecked(true);
        }
        else {
            ui->checkCombKeyOrderCheckBox->setChecked(false);
        }

        /* Load Unbreakable */
        if (true == keymapdata.Unbreakable) {
            ui->unbreakableCheckBox->setChecked(true);
        }
        else {
            ui->unbreakableCheckBox->setChecked(false);
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

        m_ItemSetupKeyRecordEditMode = QKeyMapperConstants::KEYRECORD_EDITMODE_CAPTURE;
        m_isItemSetupKeyRecordLineEdit_CapturingKey = false;
        ui->keyRecordLineEdit->clear();
        ui->keyRecordLineEdit->setReadOnly(true);

//         QTimer::singleShot(100, this, [=]() {
//             QWidget *focused = focusWidget();
//             if (focused && focused != this) {
//                 focused->clearFocus();
// #ifdef DEBUG_LOGOUT_ON
//                 qDebug() << "[QItemSetupDialog::showEvent]" << "Clear initial Focus.";
// #endif
//             }
//         });

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
            ui->mappingKeyUnlockCheckBox->setEnabled(false);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(false);
        }

        if (keymapdata.Mapping_Keys.size() > 1) {
            ui->repeatByKeyCheckBox->setEnabled(true);
            ui->repeatByTimesCheckBox->setEnabled(true);
            ui->repeatTimesSpinBox->setEnabled(true);
        }
        else {
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

void QItemSetupDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }
    else if (event->button() == Qt::RightButton) {
        // Mouse position in dialog coordinates
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        QPoint localPos = event->position().toPoint();
#else
        QPoint localPos = event->pos();
#endif

        // Map button's rect to dialog coordinates
        QRect buttonRectInParent = QRect(
            ui->keyRecordEditModeButton->mapTo(this, QPoint(0, 0)),
            ui->keyRecordEditModeButton->size()
            );

        if (buttonRectInParent.contains(localPos)) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[QItemSetupDialog::mousePressEvent] Mouse-R click inside keyRecordEditModeButton area";
#endif
            QString currentKeyRecordText = ui->keyRecordLineEdit->text();
            if (!currentKeyRecordText.isEmpty()) {
                if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
                    if (s_editingMappingKeyLineEdit == ITEMSETUP_EDITING_KEYUPMAPPINGKEY) {
                        QString currentMappingKey_KeyUpText = m_MappingKey_KeyUpLineEdit->text();
                        QString newKeyText;
                        if (currentMappingKey_KeyUpText.isEmpty()) {
                            newKeyText = currentKeyRecordText;
                        }
                        else {
                            newKeyText = currentMappingKey_KeyUpText + QString(SEPARATOR_PLUS) + currentKeyRecordText;
                        }
                        m_MappingKey_KeyUpLineEdit->setText(newKeyText);

#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[QItemSetupDialog::mousePressEvent]" << "Set new MappingKey_KeyUp Text ->" << newKeyText;
#endif
                    }
                    else {
                        QString currentMappingKeyText = m_MappingKeyLineEdit->text();
                        QString newKeyText;
                        if (currentMappingKeyText.isEmpty()) {
                            newKeyText = currentKeyRecordText;
                        }
                        else {
                            newKeyText = currentMappingKeyText + QString(SEPARATOR_PLUS) + currentKeyRecordText;
                        }
                        m_MappingKeyLineEdit->setText(newKeyText);

#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[QItemSetupDialog::mousePressEvent]" << "Set new MappingKey Text ->" << newKeyText;
#endif
                    }
                }
                else {
                    QString currentOriginalKeyText = ui->originalKeyLineEdit->text();
                    QString newKeyText;
                    if (currentOriginalKeyText.isEmpty()) {
                        newKeyText = currentKeyRecordText;
                    }
                    else {
                        newKeyText = currentOriginalKeyText + QString(SEPARATOR_PLUS) + currentKeyRecordText;
                    }
                    ui->originalKeyLineEdit->setText(newKeyText);

#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[QItemSetupDialog::mousePressEvent]" << "Set new OriginalKeyText ->" << newKeyText;
#endif
                }
            }

            return;
        }
    }

    QDialog::mousePressEvent(event);
}

bool QItemSetupDialog::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->keyRecordLineEdit) {
        if (event->type() == QEvent::FocusIn) {
            if (m_ItemSetupKeyRecordEditMode == KEYRECORD_EDITMODE_CAPTURE) {
                ui->keyRecordLineEdit->setPlaceholderText(tr("Press any key to record..."));
            }
            else {
                ui->keyRecordLineEdit->setPlaceholderText(QString());
            }
        }
        else if (event->type() == QEvent::FocusOut) {
            ui->keyRecordLineEdit->setPlaceholderText(QString());
        }
        else if (event->type() == QEvent::ReadOnlyChange
                 && ui->keyRecordLineEdit->hasFocus()) {
            if (m_ItemSetupKeyRecordEditMode == KEYRECORD_EDITMODE_CAPTURE) {
                ui->keyRecordLineEdit->setPlaceholderText(tr("Press any key to record..."));
            }
            else {
                ui->keyRecordLineEdit->setPlaceholderText(QString());
            }
        }
        else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
                return true;
            }
        }
    }

    return QDialog::eventFilter(object, event);
}

void QItemSetupDialog::initKeyListComboBoxes()
{
    updateOriginalKeyListComboBox();
    updateMappingKeyListComboBox();

    KeyListComboBox *orikeyComboBox = QKeyMapper::getInstance()->m_orikeyComboBox;
    KeyListComboBox *mapkeyComboBox = QKeyMapper::getInstance()->m_mapkeyComboBox;

    for(int i = 1; i < orikeyComboBox->count(); i++) {
        QString text = orikeyComboBox->itemText(i);
        s_valiedOriginalKeyList.append(text);
    }

    for(int i = 1; i < mapkeyComboBox->count(); i++) {
        QString text = mapkeyComboBox->itemText(i);
        s_valiedMappingKeyList.append(text);
    }
    s_valiedMappingKeyList.append(QKeyMapper_Worker::SpecialOriginalKeysList);

    s_valiedMappingKeyList.removeOne(SENDTEXT_STR);
    s_valiedMappingKeyList.removeOne(PASTETEXT_STR);
    s_valiedMappingKeyList.removeOne(RUN_STR);
    s_valiedMappingKeyList.removeOne(SWITCHTAB_STR);
    s_valiedMappingKeyList.removeOne(SWITCHTAB_SAVE_STR);
    s_valiedMappingKeyList.removeOne(MACRO_STR);
    s_valiedMappingKeyList.removeOne(UNIVERSAL_MACRO_STR);
    s_valiedMappingKeyList.removeOne(REPEAT_STR);
    s_valiedMappingKeyList.removeOne(UNLOCK_STR);
    s_valiedMappingKeyList.removeOne(SETVOLUME_STR);
    s_valiedMappingKeyList.removeOne(SETVOLUME_NOTIFY_STR);
    s_valiedMappingKeyList.removeOne(SETMICVOLUME_STR);
    s_valiedMappingKeyList.removeOne(SETMICVOLUME_NOTIFY_STR);

    s_valiedMappingKeyList.removeOne(MOUSE_L_WINDOWPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_R_WINDOWPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_M_WINDOWPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_X1_WINDOWPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_X2_WINDOWPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_MOVE_WINDOWPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_L_SCREENPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_R_SCREENPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_M_SCREENPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_X1_SCREENPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_X2_SCREENPOINT_STR);
    s_valiedMappingKeyList.removeOne(MOUSE_MOVE_SCREENPOINT_STR);

    int left = ui->orikeyListLabel->x() + ui->orikeyListLabel->width() + 5;
    int top = ui->orikeyListLabel->y();
    m_OriginalKeyListComboBox->setObjectName(SETUPDIALOG_ORIKEY_COMBOBOX_NAME);
    m_OriginalKeyListComboBox->setGeometry(QRect(left, top, 160, 22));
    m_OriginalKeyListComboBox->setFocusPolicy(Qt::ClickFocus);

    left = ui->mapkeyListLabel->x() + ui->mapkeyListLabel->width() + 5;
    top = ui->mapkeyListLabel->y();
    m_MappingKeyListComboBox->setObjectName(SETUPDIALOG_MAPKEY_COMBOBOX_NAME);
    m_MappingKeyListComboBox->setGeometry(QRect(left, top, 160, 22));
    m_MappingKeyListComboBox->setFocusPolicy(Qt::ClickFocus);
}

void QItemSetupDialog::initKeyStringLineEdit()
{
    int width = ui->originalKeyLineEdit->width();
    int height = ui->originalKeyLineEdit->height();

    int left = ui->mappingKeyLabel->x() + ui->mappingKeyLabel->width() + 4;
    int top = ui->mappingKeyLabel->y();
    m_MappingKeyLineEdit->setObjectName(SETUPDIALOG_MAPKEY_LINEEDIT_NAME);
    m_MappingKeyLineEdit->setGeometry(QRect(left, top, width, height));

    left = ui->mappingKey_KeyUpLabel->x() + ui->mappingKey_KeyUpLabel->width() + 4;
    top = ui->mappingKey_KeyUpLabel->y();
    m_MappingKey_KeyUpLineEdit->setObjectName(SETUPDIALOG_MAPKEY_KEYUP_LINEEDIT_NAME);
    m_MappingKey_KeyUpLineEdit->setGeometry(QRect(left, top, width, height));

    m_MappingKeyLineEdit->setMaxLength(MAPPINGKEY_LINE_EDIT_MAX_LENGTH);
    m_MappingKey_KeyUpLineEdit->setMaxLength(MAPPINGKEY_LINE_EDIT_MAX_LENGTH);

    setEditingMappingKeyLineEdit(ITEMSETUP_EDITING_MAPPINGKEY);
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

        /* Load Disabled */
        if (true == keymapdata.Disabled) {
            ui->disabledCheckBox->setChecked(true);
        }
        else {
            ui->disabledCheckBox->setChecked(false);
        }

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
            ui->mappingKeyUnlockCheckBox->setEnabled(true);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(true);
        }
        else {
            ui->lockCheckBox->setChecked(false);
            ui->mappingKeyUnlockCheckBox->setEnabled(false);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(false);
        }

        /* Load MappingKeyUnlock */
        if (true == keymapdata.MappingKeyUnlock) {
            ui->mappingKeyUnlockCheckBox->setChecked(true);
        }
        else {
            ui->mappingKeyUnlockCheckBox->setChecked(false);
        }

        /* Load DisableOriginalKeyUnlock */
        if (true == keymapdata.DisableOriginalKeyUnlock) {
            ui->disableOriginalKeyUnlockCheckBox->setChecked(true);
        }
        else {
            ui->disableOriginalKeyUnlockCheckBox->setChecked(false);
        }

        /* Load DisableFnKeySwitchCheckBox */
        if (true == keymapdata.DisableFnKeySwitch) {
            ui->disableFnKeySwitchCheckBox->setChecked(true);
        }
        else {
            ui->disableFnKeySwitchCheckBox->setChecked(false);
        }

        /* Load SendMappingKeyMethod State */
        if (SENDMAPPINGKEY_METHOD_SENDMESSAGE == keymapdata.SendMappingKeyMethod) {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_SENDMESSAGE);
        }
        else if (SENDMAPPINGKEY_METHOD_FAKERINPUT == keymapdata.SendMappingKeyMethod) {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_FAKERINPUT);
        }
        else {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_SENDINPUT);
        }

        /* Load FixedVKeyCode */
        if (FIXED_VIRTUAL_KEY_CODE_MIN <= keymapdata.FixedVKeyCode && keymapdata.FixedVKeyCode <= FIXED_VIRTUAL_KEY_CODE_MAX) {
            ui->fixedVKeyCodeSpinBox->setValue(keymapdata.FixedVKeyCode);
        }
        else {
            ui->fixedVKeyCodeSpinBox->setValue(FIXED_VIRTUAL_KEY_CODE_NONE);
        }

        /* Load PasteTextMode State */
        if (PASTETEXT_MODE_CTRLV == keymapdata.PasteTextMode) {
            ui->pasteTextModeComboBox->setCurrentIndex(PASTETEXT_MODE_CTRLV);
        }
        else {
            ui->pasteTextModeComboBox->setCurrentIndex(PASTETEXT_MODE_SHIFTINSERT);
        }

        /* Load SendTiming State */
        if (SENDTIMING_KEYDOWN == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYDOWN);
        }
        else if (SENDTIMING_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYUP);
        }
        else if (SENDTIMING_KEYDOWN_AND_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYDOWN_AND_KEYUP);
        }
        else if (SENDTIMING_NORMAL_AND_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL_AND_KEYUP);
        }
        else {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);
        }

        bool isSendTimingValid = QKeyMapper::validateSendTimingByKeyMapData(keymapdata);
        if (!isSendTimingValid) {
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].SendTiming = SENDTIMING_NORMAL;
            keymapdata.SendTiming = SENDTIMING_NORMAL;
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);
            ui->sendTimingComboBox->setEnabled(false);
        }
        else {
            ui->sendTimingComboBox->setEnabled(true);
        }

        /* Only SENDTIMING_NORMAL enable KeySeqHoldDown */
        if (SENDTIMING_NORMAL == keymapdata.SendTiming) {
            if (keymapdata.Mapping_Keys.size() > 1) {
                ui->keySeqHoldDownCheckBox->setEnabled(true);
            }
            else {
                ui->keySeqHoldDownCheckBox->setEnabled(false);
                keymapdata.KeySeqHoldDown = false;
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
            }
        }
        else {
            ui->keySeqHoldDownCheckBox->setEnabled(false);
            keymapdata.KeySeqHoldDown = false;
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
        }

        /* Update CheckCombKeyOrder Enable Status & Load CheckCombKeyOrder Status */
        if (keymapdata.Original_Key.contains(SEPARATOR_PLUS)) {
            ui->checkCombKeyOrderCheckBox->setEnabled(true);
        }
        else {
            ui->checkCombKeyOrderCheckBox->setEnabled(false);
        }
        if (true == keymapdata.CheckCombKeyOrder) {
            ui->checkCombKeyOrderCheckBox->setChecked(true);
        }
        else {
            ui->checkCombKeyOrderCheckBox->setChecked(false);
        }

        /* Load Unbreakable */
        if (true == keymapdata.Unbreakable) {
            ui->unbreakableCheckBox->setChecked(true);
        }
        else {
            ui->unbreakableCheckBox->setChecked(false);
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

//         QWidget *focused = focusWidget();
//         if (focused && focused != this) {
//             focused->clearFocus();
// #ifdef DEBUG_LOGOUT_ON
//             qDebug() << "[QItemSetupDialog::refreshOriginalKeyRelatedUI]" << "Clear initial Focus.";
// #endif
//         }

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
            ui->mappingKeyUnlockCheckBox->setEnabled(false);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(false);
        }

        if (keymapdata.Mapping_Keys.size() > 1) {
            ui->repeatByKeyCheckBox->setEnabled(true);
            ui->repeatByTimesCheckBox->setEnabled(true);
            ui->repeatTimesSpinBox->setEnabled(true);
        }
        else {
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
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].LockState = LOCK_STATE_LOCKOFF;
                keymapdata.Lock = false;
                value_changed = true;
            }
            ui->lockCheckBox->setEnabled(false);
            ui->mappingKeyUnlockCheckBox->setEnabled(false);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(false);
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

#if 0
        /* Load Mapping Keys String */
        QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
        m_MappingKeyLineEdit->setText(mappingkeys_str);

        /* Load KeyUp MappingKeys String */
        if (keymapdata.MappingKeys_KeyUp.isEmpty()) {
            keymapdata.MappingKeys_KeyUp = keymapdata.Mapping_Keys;
            m_MappingKey_KeyUpLineEdit->setText(mappingkeys_str);
        }
        else {
            QString keyup_mappingkeys_str = keymapdata.MappingKeys_KeyUp.join(SEPARATOR_NEXTARROW);
            m_MappingKey_KeyUpLineEdit->setText(keyup_mappingkeys_str);
        }
#endif

        /* Load Disabled */
        if (true == keymapdata.Disabled) {
            ui->disabledCheckBox->setChecked(true);
        }
        else {
            ui->disabledCheckBox->setChecked(false);
        }

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
            ui->mappingKeyUnlockCheckBox->setEnabled(true);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(true);
        }
        else {
            ui->lockCheckBox->setChecked(false);
            ui->mappingKeyUnlockCheckBox->setEnabled(false);
            ui->disableOriginalKeyUnlockCheckBox->setEnabled(false);
        }

        /* Load MappingKeyUnlock */
        if (true == keymapdata.MappingKeyUnlock) {
            ui->mappingKeyUnlockCheckBox->setChecked(true);
        }
        else {
            ui->mappingKeyUnlockCheckBox->setChecked(false);
        }

        /* Load DisableOriginalKeyUnlock */
        if (true == keymapdata.DisableOriginalKeyUnlock) {
            ui->disableOriginalKeyUnlockCheckBox->setChecked(true);
        }
        else {
            ui->disableOriginalKeyUnlockCheckBox->setChecked(false);
        }

        /* Load DisableFnKeySwitchCheckBox */
        if (true == keymapdata.DisableFnKeySwitch) {
            ui->disableFnKeySwitchCheckBox->setChecked(true);
        }
        else {
            ui->disableFnKeySwitchCheckBox->setChecked(false);
        }

        /* Load SendMappingKeyMethod State */
        if (SENDMAPPINGKEY_METHOD_SENDMESSAGE == keymapdata.SendMappingKeyMethod) {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_SENDMESSAGE);
        }
        else if (SENDMAPPINGKEY_METHOD_FAKERINPUT == keymapdata.SendMappingKeyMethod) {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_FAKERINPUT);
        }
        else {
            ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_SENDINPUT);
        }

        /* Load FixedVKeyCode */
        if (FIXED_VIRTUAL_KEY_CODE_MIN <= keymapdata.FixedVKeyCode && keymapdata.FixedVKeyCode <= FIXED_VIRTUAL_KEY_CODE_MAX) {
            ui->fixedVKeyCodeSpinBox->setValue(keymapdata.FixedVKeyCode);
        }
        else {
            ui->fixedVKeyCodeSpinBox->setValue(FIXED_VIRTUAL_KEY_CODE_NONE);
        }

        /* Load PasteTextMode State */
        if (PASTETEXT_MODE_CTRLV == keymapdata.PasteTextMode) {
            ui->pasteTextModeComboBox->setCurrentIndex(PASTETEXT_MODE_CTRLV);
        }
        else {
            ui->pasteTextModeComboBox->setCurrentIndex(PASTETEXT_MODE_SHIFTINSERT);
        }

        /* Load SendTiming State */
        if (SENDTIMING_KEYDOWN == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYDOWN);
        }
        else if (SENDTIMING_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYUP);
        }
        else if (SENDTIMING_KEYDOWN_AND_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYDOWN_AND_KEYUP);
        }
        else if (SENDTIMING_NORMAL_AND_KEYUP == keymapdata.SendTiming) {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL_AND_KEYUP);
        }
        else {
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);
        }

        bool isSendTimingValid = QKeyMapper::validateSendTimingByKeyMapData(keymapdata);
        if (!isSendTimingValid) {
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].SendTiming = SENDTIMING_NORMAL;
            keymapdata.SendTiming = SENDTIMING_NORMAL;
            ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);
            ui->sendTimingComboBox->setEnabled(false);
        }
        else {
            ui->sendTimingComboBox->setEnabled(true);
        }

        /* Only SENDTIMING_NORMAL enable KeySeqHoldDown */
        if (SENDTIMING_NORMAL == keymapdata.SendTiming) {
            if (keymapdata.Mapping_Keys.size() > 1) {
                ui->keySeqHoldDownCheckBox->setEnabled(true);
            }
            else {
                ui->keySeqHoldDownCheckBox->setEnabled(false);
                keymapdata.KeySeqHoldDown = false;
                (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
            }
        }
        else {
            ui->keySeqHoldDownCheckBox->setEnabled(false);
            keymapdata.KeySeqHoldDown = false;
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
        }

        /* Update CheckCombKeyOrder Enable Status & Load CheckCombKeyOrder Status */
        if (keymapdata.Original_Key.contains(SEPARATOR_PLUS)) {
            ui->checkCombKeyOrderCheckBox->setEnabled(true);
        }
        else {
            ui->checkCombKeyOrderCheckBox->setEnabled(false);
        }
        if (true == keymapdata.CheckCombKeyOrder) {
            ui->checkCombKeyOrderCheckBox->setChecked(true);
        }
        else {
            ui->checkCombKeyOrderCheckBox->setChecked(false);
        }

        /* Load Unbreakable */
        if (true == keymapdata.Unbreakable) {
            ui->unbreakableCheckBox->setChecked(true);
        }
        else {
            ui->unbreakableCheckBox->setChecked(false);
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

//         QWidget *focused = focusWidget();
//         if (focused && focused != this) {
//             focused->clearFocus();
// #ifdef DEBUG_LOGOUT_ON
//             qDebug() << "[QItemSetupDialog::refreshMappingKeyRelatedUI]" << "Clear initial Focus.";
// #endif
//         }
    }

    return value_changed;
}

void QItemSetupDialog::refreshAllRelatedUI()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    MAP_KEYDATA keymapdata = QKeyMapper::KeyMappingDataList->at(m_ItemRow);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QItemSetupDialog::refreshOriginalKeyRelatedUI]" << "Load Key Mapping Data[" << m_ItemRow << "] ->" << keymapdata;
#endif

    /* Load Original Key String */
    QString originalkey_str = keymapdata.Original_Key;
    ui->originalKeyLineEdit->setText(originalkey_str);

    /* Load Mapping Keys String */
    QString mappingkeys_str = keymapdata.Mapping_Keys.join(SEPARATOR_NEXTARROW);
    m_MappingKeyLineEdit->setText(mappingkeys_str);

    /* Load KeyUp MappingKeys String */
    if (keymapdata.MappingKeys_KeyUp.isEmpty()) {
        keymapdata.MappingKeys_KeyUp = keymapdata.Mapping_Keys;
        m_MappingKey_KeyUpLineEdit->setText(mappingkeys_str);
    }
    else {
        QString keyup_mappingkeys_str = keymapdata.MappingKeys_KeyUp.join(SEPARATOR_NEXTARROW);
        m_MappingKey_KeyUpLineEdit->setText(keyup_mappingkeys_str);
    }

    bool burstEnabled = QKeyMapper::getKeyMappingDataTableItemBurstStatus(m_ItemRow);
    bool lockEnabled = QKeyMapper::getKeyMappingDataTableItemLockStatus(m_ItemRow);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::refreshAllRelatedUI] Burst item in row" << m_ItemRow << " enabled =" << burstEnabled;
    qDebug() << "[QItemSetupDialog::refreshAllRelatedUI] Lock item in row" << m_ItemRow << " enabled =" << lockEnabled;
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
            QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, BURST_MODE_COLUMN);
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
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].LockState = LOCK_STATE_LOCKOFF;
            keymapdata.Lock = false;
            QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, LOCK_COLUMN);
        }
        ui->lockCheckBox->setEnabled(false);
        ui->mappingKeyUnlockCheckBox->setEnabled(false);
        ui->disableOriginalKeyUnlockCheckBox->setEnabled(false);
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
        }
        if (keymapdata.KeySeqHoldDown) {
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
            keymapdata.KeySeqHoldDown = false;
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
        }
        ui->repeatByKeyCheckBox->setEnabled(false);
        ui->repeatByTimesCheckBox->setEnabled(false);
        ui->repeatTimesSpinBox->setEnabled(false);
    }

    keymapdata = QKeyMapper::KeyMappingDataList->at(m_ItemRow);

    /* Load Disabled */
    if (true == keymapdata.Disabled) {
        ui->disabledCheckBox->setChecked(true);
    }
    else {
        ui->disabledCheckBox->setChecked(false);
    }

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
        ui->mappingKeyUnlockCheckBox->setEnabled(true);
        ui->disableOriginalKeyUnlockCheckBox->setEnabled(true);
    }
    else {
        ui->lockCheckBox->setChecked(false);
        ui->mappingKeyUnlockCheckBox->setEnabled(false);
        ui->disableOriginalKeyUnlockCheckBox->setEnabled(false);
    }

    /* Load MappingKeyUnlock */
    if (true == keymapdata.MappingKeyUnlock) {
        ui->mappingKeyUnlockCheckBox->setChecked(true);
    }
    else {
        ui->mappingKeyUnlockCheckBox->setChecked(false);
    }

    /* Load DisableOriginalKeyUnlock */
    if (true == keymapdata.DisableOriginalKeyUnlock) {
        ui->disableOriginalKeyUnlockCheckBox->setChecked(true);
    }
    else {
        ui->disableOriginalKeyUnlockCheckBox->setChecked(false);
    }

    /* Load DisableFnKeySwitchCheckBox */
    if (true == keymapdata.DisableFnKeySwitch) {
        ui->disableFnKeySwitchCheckBox->setChecked(true);
    }
    else {
        ui->disableFnKeySwitchCheckBox->setChecked(false);
    }

    /* Load SendMappingKeyMethod State */
    if (SENDMAPPINGKEY_METHOD_SENDMESSAGE == keymapdata.SendMappingKeyMethod) {
        ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_SENDMESSAGE);
    }
    else if (SENDMAPPINGKEY_METHOD_FAKERINPUT == keymapdata.SendMappingKeyMethod) {
        ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_FAKERINPUT);
    }
    else {
        ui->sendMappingKeyMethodComboBox->setCurrentIndex(SENDMAPPINGKEY_METHOD_SENDINPUT);
    }

    /* Load FixedVKeyCode */
    if (FIXED_VIRTUAL_KEY_CODE_MIN <= keymapdata.FixedVKeyCode && keymapdata.FixedVKeyCode <= FIXED_VIRTUAL_KEY_CODE_MAX) {
        ui->fixedVKeyCodeSpinBox->setValue(keymapdata.FixedVKeyCode);
    }
    else {
        ui->fixedVKeyCodeSpinBox->setValue(FIXED_VIRTUAL_KEY_CODE_NONE);
    }

    /* Load PasteTextMode State */
    if (PASTETEXT_MODE_CTRLV == keymapdata.PasteTextMode) {
        ui->pasteTextModeComboBox->setCurrentIndex(PASTETEXT_MODE_CTRLV);
    }
    else {
        ui->pasteTextModeComboBox->setCurrentIndex(PASTETEXT_MODE_SHIFTINSERT);
    }

    /* Load SendTiming State */
    if (SENDTIMING_KEYDOWN == keymapdata.SendTiming) {
        ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYDOWN);
    }
    else if (SENDTIMING_KEYUP == keymapdata.SendTiming) {
        ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYUP);
    }
    else if (SENDTIMING_KEYDOWN_AND_KEYUP == keymapdata.SendTiming) {
        ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_KEYDOWN_AND_KEYUP);
    }
    else if (SENDTIMING_NORMAL_AND_KEYUP == keymapdata.SendTiming) {
        ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL_AND_KEYUP);
    }
    else {
        ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);
    }

    bool isSendTimingValid = QKeyMapper::validateSendTimingByKeyMapData(keymapdata);
    if (!isSendTimingValid) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].SendTiming = SENDTIMING_NORMAL;
        keymapdata.SendTiming = SENDTIMING_NORMAL;
        ui->sendTimingComboBox->setCurrentIndex(SENDTIMING_NORMAL);
        ui->sendTimingComboBox->setEnabled(false);
    }
    else {
        ui->sendTimingComboBox->setEnabled(true);
    }

    /* Only SENDTIMING_NORMAL enable KeySeqHoldDown */
    if (SENDTIMING_NORMAL == keymapdata.SendTiming) {
        if (keymapdata.Mapping_Keys.size() > 1) {
            ui->keySeqHoldDownCheckBox->setEnabled(true);
        }
        else {
            ui->keySeqHoldDownCheckBox->setEnabled(false);
            keymapdata.KeySeqHoldDown = false;
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
        }
    }
    else {
        ui->keySeqHoldDownCheckBox->setEnabled(false);
        keymapdata.KeySeqHoldDown = false;
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].KeySeqHoldDown = false;
    }

    /* Update CheckCombKeyOrder Enable Status & Load CheckCombKeyOrder Status */
    if (keymapdata.Original_Key.contains(SEPARATOR_PLUS)) {
        ui->checkCombKeyOrderCheckBox->setEnabled(true);
    }
    else {
        ui->checkCombKeyOrderCheckBox->setEnabled(false);
    }
    if (true == keymapdata.CheckCombKeyOrder) {
        ui->checkCombKeyOrderCheckBox->setChecked(true);
    }
    else {
        ui->checkCombKeyOrderCheckBox->setChecked(false);
    }

    /* Load Unbreakable */
    if (true == keymapdata.Unbreakable) {
        ui->unbreakableCheckBox->setChecked(true);
    }
    else {
        ui->unbreakableCheckBox->setChecked(false);
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
}

void QItemSetupDialog::updateMappingInfoByOrder(int update_order)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    // Define update priorities based on focus or specified order
    QList<int> updatePriorities;
    if (update_order != MAPPING_UPDATE_ORDER_DEFAULT) {
        if (update_order == MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST) {
            updatePriorities << MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST << MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST;
        }
        else if (update_order == MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST) {
            updatePriorities << MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST << MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST;
        }
        else {
            updatePriorities << MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST;
        }
    }
    else {
        // Determine focus-based priority if available
        QWidget* focusWidget = QApplication::focusWidget();
        if (focusWidget) {
            if (focusWidget == ui->originalKeyLineEdit) {
                updatePriorities << MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST;
            }
            else if (focusWidget == m_MappingKeyLineEdit) {
                updatePriorities << MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST << MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST;
            }
            else if (focusWidget == m_MappingKey_KeyUpLineEdit) {
                updatePriorities << MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST << MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST;
            }
            else {
                // Default priority order if no relevant widget has focus
                updatePriorities << MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST;
            }
        }
        else {
            // No focus widget, use defaults order
            updatePriorities << MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST << MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST;
        }
    }

    // Store the original UI content for validation
    QString originalKeytoUpdate = ui->originalKeyLineEdit->text();
    QString mappingKeytoUpdate = m_MappingKeyLineEdit->text();
    QString mappingKey_KeyUptoUpdate = m_MappingKey_KeyUpLineEdit->text();

    // Execute validation in priority order, stopping on first validation error
    bool allUpdatesSuccessful = true;
    for (int priority : std::as_const(updatePriorities)) {
        bool success = false;

        switch (priority) {
            case MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST:
                // Validate original key without updating UI
                success = updateOriginalKey(originalKeytoUpdate, mappingKeytoUpdate, m_ItemRow);
                break;
            case MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST:
                // Validate mapping key without updating UI
                success = updateMappingKey(mappingKeytoUpdate, originalKeytoUpdate, m_ItemRow);
                break;
            case MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST:
                // Validate mapping key key-up without updating UI
                success = updateMappingKeyKeyUp(mappingKey_KeyUptoUpdate, originalKeytoUpdate, m_ItemRow);
                break;
            default:
                continue; // Skip unknown priorities
        }

        // Stop on first validation error - error message already shown by update functions
        if (!success) {
            allUpdatesSuccessful = false;
            break;
        }
    }

    // Apply UI updates only if all validations passed
    if (allUpdatesSuccessful && !updatePriorities.isEmpty()) {
        // Update LineEdit controls with validated and potentially modified content
        updateAllMappingInfoFinally(originalKeytoUpdate, mappingKeytoUpdate, mappingKey_KeyUptoUpdate);

        // Show success message
        QString popupMessage;
        QString popupMessageColor;
        int popupMessageDisplayTime = 3000;

        popupMessageColor = SUCCESS_COLOR;
        popupMessage = tr("Key mapping updated successfully");
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
    }
}

bool QItemSetupDialog::updateOriginalKey(QString &originalKey, const QString &mappingKey, int rowindex)
{
    static QRegularExpression simplified_regex(R"([\r\n]+)");
    static QRegularExpression whitespace_reg(R"(\s+)");
    originalKey.replace(simplified_regex, " ");
    originalKey.remove(whitespace_reg);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] OriginalKeyText remove whitespace -> " << originalKey;
#endif

    ValidationResult result = QKeyMapper::validateOriginalKeyString(originalKey, rowindex, mappingKey);

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;
    if (result.isValid) {
        return true;
    }
    else {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr(ORIGINALKEYLABEL_STR) + " -> " + result.errorMessage;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return false;
    }
}

bool QItemSetupDialog::updateMappingKey(QString &mappingKey, const QString &originalKey, int rowindex)
{
    static QRegularExpression simplified_regex(R"([\r\n]+)");
    static QRegularExpression whitespace_reg(R"(\s+)");
    static QRegularExpression sendtext_regex(REGEX_PATTERN_SENDTEXT_FIND, QRegularExpression::MultilineOption);
    static QRegularExpression run_regex(REGEX_PATTERN_RUN_FIND);
    static QRegularExpression switchtab_regex(REGEX_PATTERN_SWITCHTAB_FIND);
    static QRegularExpression unlock_regex(REGEX_PATTERN_UNLOCK_FIND);
    static QRegularExpression repeat_regex(REGEX_PATTERN_REPEAT_FIND);
    static QRegularExpression macro_regex(REGEX_PATTERN_MACRO_FIND);

    // Extract SendText(...), Run(...), SwitchTab(...), Unlock(...), SetVolume(...), Repeat{...}x..., and Macro(...) content to preserve them
    QPair<QString, QStringList> extractResult = extractSpecialPatternsWithBracketBalancing(mappingKey, sendtext_regex, run_regex, switchtab_regex, unlock_regex, QRegularExpression(), repeat_regex, macro_regex);
    QString tempMappingKey = extractResult.first;
    QStringList preservedParts = extractResult.second;

    tempMappingKey.replace(simplified_regex, " ");
    // Remove whitespace from the temporary string (excluding Run and SendText content)
    tempMappingKey.remove(whitespace_reg);

    // Restore all preserved parts
    for (int i = 0; i < preservedParts.size(); ++i) {
        QString placeholder = QString("__PRESERVED_PLACEHOLDER_%1__").arg(i);
        tempMappingKey.replace(placeholder, preservedParts[i]);
    }

    mappingKey = tempMappingKey;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] MappingKeyText after preserving Run(...) and SendText(...) and removing whitespace -> " << mappingKey;
#endif

    QStringList mappingKeySeqList = splitMappingKeyString(mappingKey, SPLIT_WITH_NEXT);
    ValidationResult result = QKeyMapper::validateMappingKeyString(mappingKey, mappingKeySeqList, rowindex, originalKey);

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;
    if (result.isValid) {
        return true;
    }
    else {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr(MAPPINGKEYLABEL_STR) + " -> " + result.errorMessage;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return false;
    }
}

bool QItemSetupDialog::updateMappingKeyKeyUp(QString &mappingKey, const QString &originalKey, int rowindex)
{
    static QRegularExpression simplified_regex(R"([\r\n]+)");
    static QRegularExpression whitespace_reg(R"(\s+)");
    static QRegularExpression sendtext_regex(REGEX_PATTERN_SENDTEXT_FIND, QRegularExpression::MultilineOption);
    static QRegularExpression run_regex(REGEX_PATTERN_RUN_FIND);
    static QRegularExpression switchtab_regex(REGEX_PATTERN_SWITCHTAB_FIND);
    static QRegularExpression unlock_regex(REGEX_PATTERN_UNLOCK);
    static QRegularExpression repeat_regex(REGEX_PATTERN_REPEAT_FIND);
    static QRegularExpression macro_regex(REGEX_PATTERN_MACRO_FIND);

    // Extract SendText(...), Run(...), SwitchTab(...), Unlock(...), SetVolume(...), Repeat{...}x..., and Macro(...) content to preserve them
    QPair<QString, QStringList> extractResult = extractSpecialPatternsWithBracketBalancing(mappingKey, sendtext_regex, run_regex, switchtab_regex, unlock_regex, QRegularExpression(), repeat_regex, macro_regex);
    QString tempMappingKey = extractResult.first;
    QStringList preservedParts = extractResult.second;

    tempMappingKey.replace(simplified_regex, " ");
    // Remove whitespace from the temporary string (excluding Run and SendText content)
    tempMappingKey.remove(whitespace_reg);

    // Restore all preserved parts
    for (int i = 0; i < preservedParts.size(); ++i) {
        QString placeholder = QString("__PRESERVED_PLACEHOLDER_%1__").arg(i);
        tempMappingKey.replace(placeholder, preservedParts[i]);
    }

    mappingKey = tempMappingKey;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] KeyUp MappingKeyText after preserving Run(...) and SendText(...) and removing whitespace -> " << mappingKey;
#endif

    ValidationResult result;
    if (mappingKey.isEmpty()) {
        result.isValid = true;
    }
    else {
        QStringList mappingKeySeqList = splitMappingKeyString(mappingKey, SPLIT_WITH_NEXT);
        result = QKeyMapper::validateMappingKeyString(mappingKey, mappingKeySeqList, rowindex, originalKey);
    }

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;
    if (result.isValid) {
        return true;
    }
    else {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr(KEYUPMAPPINGLABEL_STR) + " -> " + result.errorMessage;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return false;
    }
}

void QItemSetupDialog::updateAllMappingInfoFinally(const QString &originalKey, const QString &mappingKey, const QString &mappingKey_KeyUp)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    /* OriginalKey Update */
    if ((*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key != originalKey) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key = originalKey;
    }

    /* MappingKey Update */
    QKeyMapper::updateKeyMappingDataListMappingKeys(m_ItemRow, mappingKey);

    /* KeyUp MappingKey Update */
    if (mappingKey_KeyUp.isEmpty()) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].MappingKeys_KeyUp = (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Mapping_Keys;
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Pure_MappingKeys_KeyUp = (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Pure_MappingKeys;
    }
    else {
        QKeyMapper::updateKeyMappingDataListKeyUpMappingKeys(m_ItemRow, mappingKey_KeyUp);
    }

    QKeyMapper::getInstance()->refreshKeyMappingDataTableByTabIndex(m_TabIndex);

    refreshAllRelatedUI();
}

void QItemSetupDialog::keyMappingTableItemCheckStateChanged(int row, int col, bool checked)
{
    Q_UNUSED(checked);
    if (row < 0 || row >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    if (row != m_ItemRow) {
        return;
    }

    if (col == BURST_MODE_COLUMN) {
        bool burst = ui->burstCheckBox->isChecked();
        if (burst != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Burst) {
            ui->burstCheckBox->setChecked(QKeyMapper::KeyMappingDataList->at(m_ItemRow).Burst);

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Sync Burst check state -> " << QKeyMapper::KeyMappingDataList->at(m_ItemRow).Burst;
#endif
        }
    }
    else if (col == LOCK_COLUMN) {
        bool lock = ui->lockCheckBox->isChecked();
        if (lock != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Lock) {
            ui->lockCheckBox->setChecked(QKeyMapper::KeyMappingDataList->at(m_ItemRow).Lock);

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Sync Lock checkstate -> " << QKeyMapper::KeyMappingDataList->at(m_ItemRow).Lock;
#endif
        }
    }
    else if (col == DISABLED_COLUMN) {
        bool disabled = ui->disabledCheckBox->isChecked();
        if (disabled != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Disabled) {
            ui->disabledCheckBox->setChecked(QKeyMapper::KeyMappingDataList->at(m_ItemRow).Disabled);

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Sync Disabled checkstate -> " << QKeyMapper::KeyMappingDataList->at(m_ItemRow).Disabled;
#endif
        }
    }
}

void QItemSetupDialog::on_burstpressSpinBox_valueChanged(int value)
{
    Q_UNUSED(value);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->burstpressSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).BurstPressTime) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].BurstPressTime = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_burstpressSpinBox_valueChanged]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Burst Press Time -> " << current_value;
#endif
    }
}


void QItemSetupDialog::on_burstreleaseSpinBox_valueChanged(int value)
{
    Q_UNUSED(value);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->burstreleaseSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).BurstReleaseTime) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].BurstReleaseTime = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_burstreleaseSpinBox_valueChanged]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Burst Release Time -> " << current_value;
#endif
    }
}

void QItemSetupDialog::on_burstCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    bool burst = ui->burstCheckBox->isChecked();
    if (burst != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Burst) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Burst = burst;
        QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, BURST_MODE_COLUMN);

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
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    bool lock = ui->lockCheckBox->isChecked();
    if (lock != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Lock) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Lock = lock;
        QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, LOCK_COLUMN);

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Lock -> " << lock;
#endif
    }

    refreshMappingKeyRelatedUI();
}

void QItemSetupDialog::on_disabledCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool disabled = ui->disabledCheckBox->isChecked();
    if (disabled != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Disabled) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Disabled = disabled;
        QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, DISABLED_COLUMN);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Disabled -> " << disabled;
#endif
    }
}

void QItemSetupDialog::on_sendTimingComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int sendtiming_index = ui->sendTimingComboBox->currentIndex();
    if (sendtiming_index != QKeyMapper::KeyMappingDataList->at(m_ItemRow).SendTiming) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].SendTiming = sendtiming_index;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] SendTiming -> " << sendtiming_index;
#endif
    }

    refreshMappingKeyRelatedUI();
}

void QItemSetupDialog::on_pasteTextModeComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int pastetextmode_index = ui->pasteTextModeComboBox->currentIndex();
    if (pastetextmode_index != QKeyMapper::KeyMappingDataList->at(m_ItemRow).PasteTextMode) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].PasteTextMode = pastetextmode_index;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] PasteTextMode -> " << pastetextmode_index;
#endif
    }
}

void QItemSetupDialog::on_sendMappingKeyMethodComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int sendmappingkeymethod_index = ui->sendMappingKeyMethodComboBox->currentIndex();
    if (sendmappingkeymethod_index != QKeyMapper::KeyMappingDataList->at(m_ItemRow).SendMappingKeyMethod) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].SendMappingKeyMethod = sendmappingkeymethod_index;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] SendMappingKeyMethod -> " << sendmappingkeymethod_index;
#endif
    }
}

void QItemSetupDialog::on_checkCombKeyOrderCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool checkcombkeyorder = ui->checkCombKeyOrderCheckBox->isChecked();
    if (checkcombkeyorder != QKeyMapper::KeyMappingDataList->at(m_ItemRow).CheckCombKeyOrder) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].CheckCombKeyOrder = checkcombkeyorder;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] CheckCombKeyOrder -> " << checkcombkeyorder;
#endif
    }
}

void QItemSetupDialog::on_passThroughCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    bool passthrough = ui->passThroughCheckBox->isChecked();
    if (passthrough != QKeyMapper::KeyMappingDataList->at(m_ItemRow).PassThrough) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].PassThrough = passthrough;
        QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, ORIGINAL_KEY_COLUMN);

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

#if 0
void QItemSetupDialog::on_originalKeyUpdateButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

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
        popupMessageColor = SUCCESS_COLOR;
        popupMessage = tr("OriginalKey update success");

        if ((*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key != originalKey) {
            (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key = originalKey;
        }

        QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, ORIGINAL_KEY_COLUMN);

        refreshOriginalKeyRelatedUI();
    }
    else {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = result.errorMessage;
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}

void QItemSetupDialog::on_mappingKeyUpdateButton_clicked()
{
    updateMappingInfoByOrder(MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST);
}

void QItemSetupDialog::on_mappingKey_KeyUpUpdateButton_clicked()
{
    updateMappingInfoByOrder(MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST);
}
#endif

void QItemSetupDialog::on_recordKeysButton_clicked()
{
    if (Q_NULLPTR == m_KeyRecordDialog) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[on_recordKeysButton_clicked] Show Record Keys Dialog Window";
#endif

    if (!m_KeyRecordDialog->isVisible()) {
        m_KeyRecordDialog->show();
    }
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

void QItemSetupDialog::on_repeatTimesSpinBox_valueChanged(int value)
{
    Q_UNUSED(value);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->repeatTimesSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).RepeatTimes) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].RepeatTimes = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_repeatTimesSpinBox_valueChanged]" << " Row[" << m_ItemRow << "]("<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << ") Repeat Times -> " << current_value;
#endif
    }
}

#if 0
void QItemSetupDialog::on_itemNoteUpdateButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    QString note_str = ui->itemNoteLineEdit->text();

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] Item note -> " << note_str;
#endif

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;

    popupMessageColor = SUCCESS_COLOR;
    popupMessage = tr("Mapping item note update success");

    if ((*QKeyMapper::KeyMappingDataList)[m_ItemRow].Note != note_str) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Note = note_str;
    }

    QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, ORIGINAL_KEY_COLUMN);

    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}
#endif

void QItemSetupDialog::on_mappingKeyUnlockCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool mappingkeyunlock = ui->mappingKeyUnlockCheckBox->isChecked();
    if (mappingkeyunlock != QKeyMapper::KeyMappingDataList->at(m_ItemRow).MappingKeyUnlock) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].MappingKeyUnlock = mappingkeyunlock;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] MappingKeyUnlock -> " << mappingkeyunlock;
#endif
    }
}

void QItemSetupDialog::on_disableOriginalKeyUnlockCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool disableoriginalkeyunlock = ui->disableOriginalKeyUnlockCheckBox->isChecked();
    if (disableoriginalkeyunlock != QKeyMapper::KeyMappingDataList->at(m_ItemRow).DisableOriginalKeyUnlock) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].DisableOriginalKeyUnlock = disableoriginalkeyunlock;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] DisableOriginalKeyUnlock -> " << disableoriginalkeyunlock;
#endif
    }
}

void QItemSetupDialog::on_disableFnKeySwitchCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool disablefnkeyswitch = ui->disableFnKeySwitchCheckBox->isChecked();
    if (disablefnkeyswitch != QKeyMapper::KeyMappingDataList->at(m_ItemRow).DisableFnKeySwitch) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].DisableFnKeySwitch = disablefnkeyswitch;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] DisableFnKeySwitch -> " << disablefnkeyswitch;
#endif
    }
}

#if 0
void QItemSetupDialog::on_postMappingKeyCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool postmappingkey = ui->postMappingKeyCheckBox->isChecked();
    if (postmappingkey != QKeyMapper::KeyMappingDataList->at(m_ItemRow).PostMappingKey) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].PostMappingKey = postmappingkey;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] PostMappingKey -> " << postmappingkey;
#endif
    }
}
#endif

void QItemSetupDialog::on_unbreakableCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool unbreakable = ui->unbreakableCheckBox->isChecked();
    if (unbreakable != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Unbreakable) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Unbreakable = unbreakable;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Unbreakable -> " << unbreakable;
#endif
    }
}

void QItemSetupDialog::on_crosshairSetupButton_clicked()
{
    if (Q_NULLPTR == m_CrosshairSetupDialog) {
        return;
    }
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[on_crosshairConfigButton_clicked] Show Crosshait Config Dialog Window";
#endif

    if (!m_CrosshairSetupDialog->isVisible()) {
        m_CrosshairSetupDialog->setItemRow(m_ItemRow);
        m_CrosshairSetupDialog->show();
    }
}

void KeyStringLineEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);

    if (objectName() == SETUPDIALOG_MAPKEY_KEYUP_LINEEDIT_NAME) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[KeyStringLineEdit::focusInEvent]" << "MappingKey_KeyUp LineEdit focus in, set editing boarder to it.";
#endif
        QItemSetupDialog::setEditingMappingKeyLineEdit(ITEMSETUP_EDITING_KEYUPMAPPINGKEY);
    }
    else if (objectName() == SETUPDIALOG_MAPKEY_LINEEDIT_NAME) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[KeyStringLineEdit::focusInEvent]" << "MappingKey LineEdit focus in, set editing boarder to it.";
#endif
        QItemSetupDialog::setEditingMappingKeyLineEdit(ITEMSETUP_EDITING_MAPPINGKEY);
    }
}

void QItemSetupDialog::on_fixedVKeyCodeSpinBox_valueChanged(int value)
{
    Q_UNUSED(value);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int fixedvkeycode = ui->fixedVKeyCodeSpinBox->value();
    if (fixedvkeycode < FIXED_VIRTUAL_KEY_CODE_MIN || fixedvkeycode > FIXED_VIRTUAL_KEY_CODE_MAX) {
        fixedvkeycode = FIXED_VIRTUAL_KEY_CODE_NONE;
        ui->fixedVKeyCodeSpinBox->setValue(fixedvkeycode);
        return;
    }
    if (fixedvkeycode != QKeyMapper::KeyMappingDataList->at(m_ItemRow).FixedVKeyCode) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].FixedVKeyCode = fixedvkeycode;
#ifdef DEBUG_LOGOUT_ON
        QString fixedvkeycodeStr = QString("0x%1").arg(QString::number(fixedvkeycode, 16).toUpper(), 2, '0');
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] FixedVKeyCode -> " << fixedvkeycodeStr;
#endif
    }
}

void QItemSetupDialog::on_keyRecordEditModeButton_clicked()
{
    m_isItemSetupKeyRecordLineEdit_CapturingKey = false;
    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
        ui->keyRecordLineEdit->clear();
        return;
    }

    if (m_ItemSetupKeyRecordEditMode == KEYRECORD_EDITMODE_MANUALEDIT) {
        m_ItemSetupKeyRecordEditMode = KEYRECORD_EDITMODE_CAPTURE;
        ui->keyRecordLineEdit->setPlaceholderText(tr("Press any key to record..."));
        ui->keyRecordLineEdit->clear();
        ui->keyRecordLineEdit->setReadOnly(true);
        ui->keyRecordLineEdit->setFocus();
        ui->keyRecordEditModeButton->setText(tr("Edit"));
    }
    else {
        m_ItemSetupKeyRecordEditMode = KEYRECORD_EDITMODE_MANUALEDIT;
        ui->keyRecordLineEdit->setPlaceholderText(QString());
        ui->keyRecordLineEdit->setReadOnly(false);
        ui->keyRecordLineEdit->setFocus();
        ui->keyRecordLineEdit->setCursorPosition(ui->keyRecordLineEdit->text().length());
        ui->keyRecordEditModeButton->setText(tr("Capture"));
    }
}

void QItemSetupDialog::on_keyRecordLineEdit_textChanged(const QString &text)
{
    Q_UNUSED(text);
    bool set_place_holder = false;
    if (QKeyMapper::getInstance()->m_KeyMapStatus == QKeyMapper::KEYMAP_IDLE) {
        if (m_ItemSetupKeyRecordEditMode == KEYRECORD_EDITMODE_CAPTURE) {
            if (ui->keyRecordLineEdit->hasFocus()) {
                set_place_holder = true;
            }
        }
    }

    if (set_place_holder) {
        ui->keyRecordLineEdit->setPlaceholderText(tr("Press any key to record..."));
    }
    else {
        ui->keyRecordLineEdit->setPlaceholderText(QString());
    }

    ui->keyRecordLineEdit->setToolTip(ui->keyRecordLineEdit->text());
}

void QItemSetupDialog::on_itemNoteLineEdit_textChanged(const QString &text)
{
    Q_UNUSED(text);
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    QString note_str = ui->itemNoteLineEdit->text();

    if ((*QKeyMapper::KeyMappingDataList)[m_ItemRow].Note != note_str) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Item note -> " << note_str;
#endif
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Note = note_str;
        QKeyMapper::getInstance()->updateTableWidgetItem(m_TabIndex, m_ItemRow, ORIGINAL_KEY_COLUMN);
    }
}

void QItemSetupDialog::updateMappingInfo_OriginalKeyFirst()
{
    updateMappingInfoByOrder(MAPPING_UPDATE_ORDER_ORIGINAL_KEY_FIRST);
}

void QItemSetupDialog::updateMappingInfo_MappingKeyFirst()
{
    updateMappingInfoByOrder(MAPPING_UPDATE_ORDER_MAPPING_KEY_FIRST);
}

void QItemSetupDialog::updateMappingInfo_MappingKey_KeyUpFirst()
{
    updateMappingInfoByOrder(MAPPING_UPDATE_ORDER_MAPPING_KEY_KEYUP_FIRST);
}

void QItemSetupDialog::on_updateMappingInfoButton_clicked()
{
    updateMappingInfoByOrder();
}
