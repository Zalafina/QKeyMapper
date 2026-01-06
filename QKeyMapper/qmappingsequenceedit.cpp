#include "qmappingsequenceedit.h"
#include "ui_qmappingsequenceedit.h"
#include "qkeymapper.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QMappingSequenceEdit *QMappingSequenceEdit::m_instance = Q_NULLPTR;

QMappingSequenceEdit::QMappingSequenceEdit(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QMappingSequenceEdit)
    , m_MappingSequenceList()
    , m_MappingSequenceEditType(MAPPINGSEQUENCEEDIT_TYPE_ITEMSETUP_MAPPINGKEYS)
{
    m_instance = this;
    ui->setupUi(this);

    initKeyListComboBoxes();
    ui->MappingSequenceEdit_MappingKeyLineEdit->setMaxLength(MAPPINGKEY_LINE_EDIT_MAX_LENGTH);

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->mapList_SelectKeyboardButton->setStyle(windowsStyle);
        ui->mapList_SelectMouseButton->setStyle(windowsStyle);
        ui->mapList_SelectGamepadButton->setStyle(windowsStyle);
        ui->mapList_SelectFunctionButton->setStyle(windowsStyle);
    }
    ui->mapList_SelectKeyboardButton->setIcon(QIcon(":/keyboard.svg"));
    ui->mapList_SelectMouseButton->setIcon(QIcon(":/mouse.svg"));
    ui->mapList_SelectGamepadButton->setIcon(QIcon(":/gamepad.svg"));
    ui->mapList_SelectFunctionButton->setIcon(QIcon(":/function.svg"));
    ui->mapList_SelectKeyboardButton->setChecked(true);
    ui->mapList_SelectMouseButton->setChecked(true);
    ui->mapList_SelectGamepadButton->setChecked(true);
    ui->mapList_SelectFunctionButton->setChecked(true);

    QFont customFont(FONTNAME_ENGLISH, 9);
    ui->mapkeyLabel->setFont(customFont);
    ui->MappingSequenceEdit_MappingKeyLineEdit->setFont(customFont);
    ui->MappingSequenceEdit_MappingKeyListComboBox->setFont(customFont);
    ui->validateCellCheckBox->setFont(customFont);
    ui->confirmButton->setFont(customFont);
    ui->cancelButton->setFont(customFont);
    ui->mappingSequenceEditTable->setFont(customFont);

    int scale = QKeyMapper::getInstance()->m_UI_Scale;
    if (UI_SCALE_4K_PERCENT_150 == scale) {
        customFont.setPointSize(14);
    }
    else {
        customFont.setPointSize(12);
    }
    ui->insertMappingKeyButton->setFont(customFont);

    initMappingSequenceEditTable(ui->mappingSequenceEditTable);

    QObject::connect(ui->MappingSequenceEdit_MappingKeyLineEdit, &QLineEdit::returnPressed, this, &QMappingSequenceEdit::insertMappingKeyToTable);

    if (QItemSetupDialog::getInstance() != Q_NULLPTR) {
        QItemSetupDialog::getInstance()->syncConnectMappingKeySelectButtons();
    }
}

QMappingSequenceEdit::~QMappingSequenceEdit()
{
    delete ui;
}

void QMappingSequenceEdit::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
}

void QMappingSequenceEdit::setTitle(const QString &title)
{
    setWindowTitle(title);
}

void QMappingSequenceEdit::setMappingSequence(const QString &mappingsequence)
{
    QString trimmed_mappingsequence = QKeyMapper::getTrimmedMappingKeyString(mappingsequence);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] MappingKeyText after trimmed -> " << trimmed_mappingsequence;
#endif

    QStringList mappingKeySeqList = splitMappingKeyString(trimmed_mappingsequence, SPLIT_WITH_NEXT);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QMappingSequenceEdit::setMappingSequence]" << "Split Mapping Sequence List ->" << mappingKeySeqList;
#endif

    if (trimmed_mappingsequence.isEmpty() || mappingKeySeqList.isEmpty()) {
        return;
    }

    m_MappingSequenceList = mappingKeySeqList;
}

void QMappingSequenceEdit::setMappingSequenceEditType(int edit_type)
{
    if (edit_type == MAPPINGSEQUENCEEDIT_TYPE_ITEMSETUP_MAPPINGKEYS
        || edit_type == MAPPINGSEQUENCEEDIT_TYPE_ITEMSETUP_MAPPINGKEYS_KEYUP) {
        m_MappingSequenceEditType = edit_type;
    }
}

void QMappingSequenceEdit::refreshMappingSequenceEditTableWidget(MappingSequenceEditTableWidget *mappingSequenceEditTable, const QStringList &mappingSequenceList)
{

}

void QMappingSequenceEdit::updateMappingKeyListComboBox()
{
    KeyListComboBox *mapkeyComboBox = QKeyMapper::getInstance()->m_mapkeyComboBox;

    ui->MappingSequenceEdit_MappingKeyListComboBox->clear();

    const QIcon &common_icon = QKeyMapper::s_Icon_Blank;
    ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(QString());
    ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(common_icon, SEPARATOR_WAITTIME);
    ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(common_icon, SEPARATOR_NEXTARROW);
    ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_DOWN);
    ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_UP);
    ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_BOTH);
    ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_EXCLUSION);
    ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(common_icon, REPEAT_STR);
    for(int i = 1; i < mapkeyComboBox->count(); i++) {
        QIcon icon = mapkeyComboBox->itemIcon(i);
        QString text = mapkeyComboBox->itemText(i);
        ui->MappingSequenceEdit_MappingKeyListComboBox->addItem(icon, text);
    }
}

QString QMappingSequenceEdit::joinCurentMappingSequenceTable()
{
    MappingSequenceEditTableWidget *mappingSequenceTable = ui->mappingSequenceEditTable;

    QString merged_mappingsequence;
    /* If MappingSequenceEditTable is not empty, join MAPPINGSEQUENCEEDIT_MAPPINGKEY_COLUMN item strings into one with "SEPARATOR_NEXTARROW" */

    return merged_mappingsequence;
}

QPushButton *QMappingSequenceEdit::getMapListSelectKeyboardButton() const
{
    return ui->mapList_SelectKeyboardButton;
}

QPushButton *QMappingSequenceEdit::getMapListSelectMouseButton() const
{
    return ui->mapList_SelectMouseButton;
}

QPushButton *QMappingSequenceEdit::getMapListSelectGamepadButton() const
{
    return ui->mapList_SelectGamepadButton;
}

int QMappingSequenceEdit::getMappingSequenceEditType()
{
    return m_MappingSequenceEditType;
}

QPushButton *QMappingSequenceEdit::getMapListSelectFunctionButton() const
{
    return ui->mapList_SelectFunctionButton;
}

void QMappingSequenceEdit::showEvent(QShowEvent *event)
{
    refreshMappingSequenceEditTableWidget(ui->mappingSequenceEditTable, m_MappingSequenceList);

    QDialog::showEvent(event);
}

void QMappingSequenceEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

void QMappingSequenceEdit::insertMappingKeyToTable()
{
    QString mappingkeystr = ui->MappingSequenceEdit_MappingKeyLineEdit->text();
    mappingkeystr = QKeyMapper::getTrimmedMappingKeyString(mappingkeystr);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] MappingKeyText after trimmed -> " << mappingkeystr;
#endif
}

void QMappingSequenceEdit::mappingSequenceTableItemDoubleClicked(QTableWidgetItem *item)
{
    if (item == Q_NULLPTR) {
        return;
    }

    int rowindex = item->row();
    int columnindex = item->column();
    Q_UNUSED(columnindex);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[mappingSequenceTableItemDoubleClicked]" << "Row" << rowindex << "Column" << columnindex << "DoubleClicked";
#endif

    if (columnindex != MAPPINGSEQUENCEEDIT_MAPPINGKEY_COLUMN) {
        return;
    }

    MappingSequenceEditTableWidget *mappingSequenceTable = ui->mappingSequenceEditTable;

    Qt::MouseButtons buttons = QApplication::mouseButtons();
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

    int editmode = QKeyMapper::getEditModeIndex();
    bool load_data = true;
    if (editmode == EDITMODE_LEFT_DOUBLECLICK) {
        if (buttons & Qt::LeftButton && !(buttons & Qt::RightButton)) {
            if (modifiers & Qt::AltModifier) {
                load_data = true;
            }
            else {
                load_data = false;
            }
        }
    }
    else {
        if (buttons & Qt::RightButton && !(buttons & Qt::LeftButton)) {
            load_data = false;
        }
        else if (modifiers & Qt::AltModifier) {
            load_data = false;
        }
    }

    if (load_data) {
        // Temporarily disable edit triggers
        mappingSequenceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // load mappingkey data to LineEdit controls
        QTableWidgetItem *mappingkeyItem = mappingSequenceTable->item(rowindex, MAPPINGSEQUENCEEDIT_MAPPINGKEY_COLUMN);

        if (mappingkeyItem) {
            ui->MappingSequenceEdit_MappingKeyLineEdit->setText(mappingkeyItem->text());
        }

        // Restore edit triggers after event processing
        QTimer::singleShot(0, this, [=]() {
            mappingSequenceTable->setEditTriggers(QAbstractItemView::DoubleClicked);
        });

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[mappingSequenceTableItemDoubleClicked]" << "Loaded mapping key to LineEdit controls";
#endif
    }
    else {
        mappingSequenceTable->editItem(item);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[mappingSequenceTableItemDoubleClicked]" << "Entering edit mode";
#endif
    }
}

void QMappingSequenceEdit::initMappingSequenceEditTable(MappingSequenceEditTableWidget *mappingSequenceEditTable)
{
    mappingSequenceEditTable->setFocusPolicy(Qt::NoFocus);
    mappingSequenceEditTable->setColumnCount(MAPPINGSEQUENCEEDIT_TABLE_COLUMN_COUNT);

    mappingSequenceEditTable->horizontalHeader()->setStretchLastSection(true);
    mappingSequenceEditTable->horizontalHeader()->setHighlightSections(false);

    // macroDataTable->verticalHeader()->setVisible(false);
    mappingSequenceEditTable->verticalHeader()->setDefaultSectionSize(25);
    mappingSequenceEditTable->verticalHeader()->setStyleSheet("QHeaderView::section { color: #1A9EDB; padding-left: 2px; padding-right: 1px;}");
    mappingSequenceEditTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mappingSequenceEditTable->setSelectionMode(QAbstractItemView::ContiguousSelection);
    // Enable double-click editing for category column
    mappingSequenceEditTable->setEditTriggers(QAbstractItemView::DoubleClicked);

    /* Support Drag&Drop for macroDataTable Table */
    mappingSequenceEditTable->setDragEnabled(true);
    mappingSequenceEditTable->setDragDropMode(QAbstractItemView::InternalMove);

    mappingSequenceEditTable->setHorizontalHeaderLabels(QStringList() << tr("Split Mapping Sequence"));

    QFont customFont(FONTNAME_ENGLISH, 9);
    mappingSequenceEditTable->setFont(customFont);
    mappingSequenceEditTable->horizontalHeader()->setFont(customFont);
    if (QStyle *fusionStyle = QKeyMapperStyle::fusionStyle()) {
        mappingSequenceEditTable->setStyle(fusionStyle);
    }

    // Connect signals for this table
    updateMappingSequenceEditTableConnection(mappingSequenceEditTable);
}

void QMappingSequenceEdit::initKeyListComboBoxes()
{
    updateMappingKeyListComboBox();
}

void QMappingSequenceEdit::updateMappingSequenceEditTableConnection(MappingSequenceEditTableWidget *mappingSequenceEditTable)
{
#if 0
    if (mappingSequenceEditTable != Q_NULLPTR) {
        QObject::connect(macroDataTable, &QTableWidget::cellChanged,
                         this, &QMacroListDialog::macroTableCellChanged, Qt::UniqueConnection);
        QObject::connect(macroDataTable, &QTableWidget::itemSelectionChanged,
                         this, &QMacroListDialog::macroTableItemSelectionChanged, Qt::UniqueConnection);
        QObject::connect(macroDataTable, &QTableWidget::itemDoubleClicked,
                         this, &QMacroListDialog::macroTableItemDoubleClicked, Qt::UniqueConnection);
#ifdef DEBUG_LOGOUT_ON
        QObject::connect(macroDataTable, &QTableWidget::currentCellChanged,
                         this, &QMacroListDialog::macroTableCurrentCellChanged, Qt::UniqueConnection);
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[updateMappingSequenceEditTableConnection]" << "Invalid mappingSequenceEditTable pointer!";
#endif
    }
#endif
}

void MappingSequenceEditTableWidget::keyPressEvent(QKeyEvent *event)
{
    QTableWidget::keyPressEvent(event);
}
