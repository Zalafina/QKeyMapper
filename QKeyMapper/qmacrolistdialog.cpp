#include "qmacrolistdialog.h"
#include "ui_qmacrolistdialog.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QMacroListDialog *QMacroListDialog::m_instance = Q_NULLPTR;

QMacroListDialog::QMacroListDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QMacroListDialog)
{
    m_instance = this;
    ui->setupUi(this);

    initKeyListComboBoxes();

    QStyle* windowsStyle = QStyleFactory::create("windows");
    ui->mapList_SelectKeyboardButton->setStyle(windowsStyle);
    ui->mapList_SelectMouseButton->setStyle(windowsStyle);
    ui->mapList_SelectGamepadButton->setStyle(windowsStyle);
    ui->mapList_SelectFunctionButton->setStyle(windowsStyle);
    ui->mapList_SelectKeyboardButton->setIcon(QIcon(":/keyboard.svg"));
    ui->mapList_SelectMouseButton->setIcon(QIcon(":/mouse.svg"));
    ui->mapList_SelectGamepadButton->setIcon(QIcon(":/gamepad.svg"));
    ui->mapList_SelectFunctionButton->setIcon(QIcon(":/function.svg"));
    ui->mapList_SelectKeyboardButton->setChecked(true);
    ui->mapList_SelectMouseButton->setChecked(true);
    ui->mapList_SelectGamepadButton->setChecked(true);
    ui->mapList_SelectFunctionButton->setChecked(true);

    QFont customFont(FONTNAME_ENGLISH, 9);
    ui->macroListTabWidget->setFont(customFont);
    ui->macroNameLabel->setFont(customFont);
    ui->catetoryLabel->setFont(customFont);
    ui->macroContentLabel->setFont(customFont);
    ui->mapkeyLabel->setFont(customFont);
    ui->categoryFilterLabel->setFont(customFont);
    ui->MacroList_MappingKeyListComboBox->setFont(customFont);
    ui->categoryFilterComboBox->setFont(customFont);
    ui->macroNameLineEdit->setFont(customFont);
    ui->macroContentLineEdit->setFont(customFont);
    ui->categoryLineEdit->setFont(customFont);

    int scale = QKeyMapper::getInstance()->m_UI_Scale;
    if (UI_SCALE_4K_PERCENT_150 == scale) {
        customFont.setPointSize(14);
    }
    else {
        customFont.setPointSize(12);
    }
    ui->addMacroButton->setFont(customFont);

    initMacroListTabWidget();

    QObject::connect(ui->macroNameLineEdit, &QLineEdit::returnPressed, this, &QMacroListDialog::addMacroToList);
    QObject::connect(ui->macroContentLineEdit, &QLineEdit::returnPressed, this, &QMacroListDialog::addMacroToList);
    QObject::connect(ui->categoryLineEdit, &QLineEdit::returnPressed, this, &QMacroListDialog::addMacroToList);

    if (QItemSetupDialog::getInstance() != Q_NULLPTR) {
        QItemSetupDialog::getInstance()->syncConnectMappingKeySelectButtons();
    }
}

QMacroListDialog::~QMacroListDialog()
{
    delete ui;
}

void QMacroListDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Mapping Macro List"));

    ui->macroNameLabel->setText(tr("Name"));
    ui->catetoryLabel->setText(tr("Category"));
    ui->macroContentLabel->setText(tr("Macro"));
    ui->clearButton->setText(tr("Clear"));
    ui->addMacroButton->setText(tr("Add Macro"));
    ui->mapkeyLabel->setText(tr("MapKeys"));
    ui->categoryFilterLabel->setText(tr("Filter"));

    QTabWidget *tabWidget = ui->macroListTabWidget;
    tabWidget->setTabText(tabWidget->indexOf(ui->macrolist),            tr("Macro")          );
    tabWidget->setTabText(tabWidget->indexOf(ui->universalmacrolist),   tr("Universal Macro"));

    ui->macrolistTable->setHorizontalHeaderLabels(QStringList()             << tr("Name")
                                                                            << tr("Macro")
                                                                            << tr("Category"));
    ui->universalmacrolistTable->setHorizontalHeaderLabels(QStringList()    << tr("Name")
                                                                            << tr("Macro")
                                                                            << tr("Category"));
}

void QMacroListDialog::refreshMacroListTabWidget()
{

}

void QMacroListDialog::updateMappingKeyListComboBox()
{
    KeyListComboBox *mapkeyComboBox = QKeyMapper::getInstance()->m_mapkeyComboBox;

    ui->MacroList_MappingKeyListComboBox->clear();

    const QIcon &common_icon = QKeyMapper::s_Icon_Blank;
    ui->MacroList_MappingKeyListComboBox->addItem(QString());
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, SEPARATOR_WAITTIME);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_DOWN);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_UP);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_BOTH);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_EXCLUSION);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, REPEAT_STR);
    for(int i = 1; i < mapkeyComboBox->count(); i++) {
        QIcon icon = mapkeyComboBox->itemIcon(i);
        QString text = mapkeyComboBox->itemText(i);
        ui->MacroList_MappingKeyListComboBox->addItem(icon, text);
    }
}

QPushButton *QMacroListDialog::getMapListSelectKeyboardButton() const
{
    return ui->mapList_SelectKeyboardButton;
}

QPushButton *QMacroListDialog::getMapListSelectMouseButton() const
{
    return ui->mapList_SelectMouseButton;
}

QPushButton *QMacroListDialog::getMapListSelectGamepadButton() const
{
    return ui->mapList_SelectGamepadButton;
}

QPushButton *QMacroListDialog::getMapListSelectFunctionButton() const
{
    return ui->mapList_SelectFunctionButton;
}

QString QMacroListDialog::getEditingMacroText()
{
    return getInstance()->ui->macroContentLineEdit->text();
}

int QMacroListDialog::getEditingMacroCursorPosition()
{
    return getInstance()->ui->macroContentLineEdit->cursorPosition();
}

void QMacroListDialog::setEditingMacroText(const QString &new_macrotext)
{
    return getInstance()->ui->macroContentLineEdit->setText(new_macrotext);
}

QString QMacroListDialog::getCurrentMapKeyListText()
{
    return getInstance()->ui->MacroList_MappingKeyListComboBox->currentText();
}

void QMacroListDialog::showEvent(QShowEvent *event)
{
    refreshMacroListTabWidget();

    QDialog::showEvent(event);
}

void QMacroListDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

void QMacroListDialog::on_addMacroButton_clicked()
{
    addMacroToList();
}

void QMacroListDialog::addMacroToList()
{
    constexpr int CURRENT_TAB_NONE              = -1;
    constexpr int CURRENT_TAB_MACRO             = 0;
    constexpr int CURRENT_TAB_UNIVERSAL_MACRO   = 1;

    int current_tab = CURRENT_TAB_NONE;
    if (ui->macroListTabWidget->currentWidget() == ui->macrolist) {
        current_tab = CURRENT_TAB_MACRO;
    }
    else if (ui->macroListTabWidget->currentWidget() == ui->universalmacrolist) {
        current_tab = CURRENT_TAB_UNIVERSAL_MACRO;
    }

    if (current_tab == CURRENT_TAB_NONE) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    const char* currentTabStr = (current_tab == CURRENT_TAB_MACRO) ? "Macro List" : "Universal Macro List";
    qDebug() << "[QMacroListDialog::addMacroToList] Add macro to" << currentTabStr;
#endif

    static QRegularExpression simplified_regex(R"([\r\n]+)");

    QString macroname_str = ui->macroNameLineEdit->text();
    macroname_str = macroname_str.trimmed();
    macroname_str.replace(simplified_regex, " ");

    QString macro_str = ui->macroContentLineEdit->text();
    macro_str = macro_str.trimmed();
    macro_str.replace(simplified_regex, " ");

    QString category_str = ui->categoryLineEdit->text();
    category_str = category_str.trimmed();
    category_str.replace(simplified_regex, " ");

    ValidationResult result = QKeyMapper::validateMappingMacroString(macro_str);

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;
    if (!result.isValid) {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("Macro") + " -> " + result.errorMessage;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    OrderedMap<QString, MappingMacroData>& CurrentMacroList = (current_tab == CURRENT_TAB_MACRO) ? QKeyMapper::s_MappingMacroList : QKeyMapper::s_UniversalMappingMacroList;

    // Check if macro name already exists
    bool isUpdate = false;
    if (CurrentMacroList.contains(macroname_str)) {
        QString dialogTitle = (current_tab == CURRENT_TAB_MACRO) ? tr("Macro List") : tr("Universal Macro List");
        QString messageText = tr("Macro name already exists. Replace existing macro?");
        
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            dialogTitle,
            messageText,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        
        if (reply != QMessageBox::Yes) {
            return;
        }
        isUpdate = true;
    }

    // Insert or update macro in the list
    CurrentMacroList[macroname_str] = MappingMacroData{ macro_str, category_str };

    // Show success message
    popupMessageColor = SUCCESS_COLOR;
    if (current_tab == CURRENT_TAB_MACRO) {
        popupMessage = tr("Macro List") + " -> ";
    }
    else {
        popupMessage = tr("Universal Macro List") + " -> ";
    }
    if (isUpdate) {
        popupMessage = popupMessage + tr("Macro \"%1\" updated successfully").arg(macroname_str);
    }
    else {
        popupMessage = popupMessage + tr("Macro \"%1\" added successfully").arg(macroname_str);
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);

    // Refresh the macro list display
    refreshMacroListTabWidget();
}

void QMacroListDialog::initMacroListTabWidget()
{
    QTabWidget *tabWidget = ui->macroListTabWidget;
    QStyle* windowsStyle = QStyleFactory::create("windows");
    tabWidget->setStyle(windowsStyle);
    tabWidget->setFocusPolicy(Qt::StrongFocus);
    QTabBar *bar = tabWidget->tabBar();
    for (QObject *child : bar->children()) {
        if (QToolButton *btn = qobject_cast<QToolButton *>(child)) {
            btn->setFocusPolicy(Qt::NoFocus);
        }
    }
    tabWidget->setFont(QFont(FONTNAME_ENGLISH, 9));

    initMacroListTable(ui->macrolistTable);
    initMacroListTable(ui->universalmacrolistTable);
}

void QMacroListDialog::initMacroListTable(MacroListDataTableWidget *macroDataTable)
{
    macroDataTable->setFocusPolicy(Qt::NoFocus);
    macroDataTable->setColumnCount(MACROLISTDATA_TABLE_COLUMN_COUNT);

    macroDataTable->horizontalHeader()->setStretchLastSection(true);
    macroDataTable->horizontalHeader()->setHighlightSections(false);

    resizeMacroListTableColumnWidth(macroDataTable);

    macroDataTable->verticalHeader()->setVisible(false);
    macroDataTable->verticalHeader()->setDefaultSectionSize(25);
    macroDataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    macroDataTable->setSelectionMode(QAbstractItemView::ContiguousSelection);
    // Allow editing only for specific columns (will be controlled per item)
    macroDataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    /* Suuport Drag&Drop for macroDataTable Table */
    macroDataTable->setDragEnabled(true);
    macroDataTable->setDragDropMode(QAbstractItemView::InternalMove);

    macroDataTable->setHorizontalHeaderLabels(QStringList() << tr("Name")
                                                            << tr("Macro")
                                                            << tr("Category"));

    QFont customFont(FONTNAME_ENGLISH, 9);
    macroDataTable->setFont(customFont);
    macroDataTable->horizontalHeader()->setFont(customFont);
    macroDataTable->setStyle(QStyleFactory::create("Fusion"));
}

void QMacroListDialog::initKeyListComboBoxes()
{
    updateMappingKeyListComboBox();
}

void QMacroListDialog::resizeMacroListTabWidgetColumnWidth()
{
    resizeMacroListTableColumnWidth(ui->macrolistTable);
    resizeMacroListTableColumnWidth(ui->universalmacrolistTable);
}

void QMacroListDialog::resizeMacroListTableColumnWidth(MacroListDataTableWidget *macroDataTable)
{
#if 0
    mappingDataTable->resizeColumnToContents(ORIGINAL_KEY_COLUMN);

    int original_key_width_min = mappingDataTable->width()/5 - 15;
    int original_key_width_max = mappingDataTable->width() / 2;
    int original_key_width = mappingDataTable->columnWidth(ORIGINAL_KEY_COLUMN);

    mappingDataTable->resizeColumnToContents(BURST_MODE_COLUMN);
    int burst_mode_width = mappingDataTable->columnWidth(BURST_MODE_COLUMN);
    int lock_width = burst_mode_width;
    burst_mode_width += 8;

    int category_width = 0;
    if (mappingDataTable->isCategoryColumnVisible()) {
        lock_width += 8; // Add padding for lock column
        mappingDataTable->horizontalHeader()->setStretchLastSection(false);
        int category_width_max = mappingDataTable->width() / 5;
        mappingDataTable->resizeColumnToContents(CATEGORY_COLUMN);
        category_width = mappingDataTable->columnWidth(CATEGORY_COLUMN);
        if (category_width < burst_mode_width) {
            category_width = burst_mode_width;
        }
        if (category_width > category_width_max) {
            category_width = category_width_max;
        }
        mappingDataTable->horizontalHeader()->setStretchLastSection(true);
    }

    if (original_key_width < original_key_width_min) {
        original_key_width = original_key_width_min;
    }
    else if (original_key_width > original_key_width_max) {
        original_key_width = original_key_width_max;
    }

    int mapping_key_width_min = mappingDataTable->width()/5 - 15;
    int mapping_key_width = mappingDataTable->width() - original_key_width - burst_mode_width - lock_width - category_width - 16;
    if (mapping_key_width < mapping_key_width_min) {
        mapping_key_width = mapping_key_width_min;
    }

    mappingDataTable->setColumnWidth(ORIGINAL_KEY_COLUMN, original_key_width);
    mappingDataTable->setColumnWidth(MAPPING_KEY_COLUMN, mapping_key_width);
    mappingDataTable->setColumnWidth(BURST_MODE_COLUMN, burst_mode_width);
    mappingDataTable->setColumnWidth(LOCK_COLUMN, lock_width);
    if (mappingDataTable->isCategoryColumnVisible()) {
        mappingDataTable->setColumnWidth(CATEGORY_COLUMN, category_width);
    }
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[resizeKeyMappingDataTableColumnWidth]" << "mappingDataTable->rowCount" << mappingDataTable->rowCount();
    qDebug() << "[resizeKeyMappingDataTableColumnWidth]" << "original_key_width =" << original_key_width << ", mapping_key_width =" << mapping_key_width << ", burst_mode_width =" << burst_mode_width << ", lock_width =" << lock_width << ", category_width =" << category_width;
#endif

#endif
}

void MacroListTabWidget::keyPressEvent(QKeyEvent *event)
{

    QTabWidget::keyPressEvent(event);
}

void MacroListDataTableWidget::setCategoryFilter(const QString &category)
{

}

void MacroListDataTableWidget::clearCategoryFilter()
{

}

QStringList MacroListDataTableWidget::getAvailableCategories() const
{
    QStringList categories;

    return categories;
}

void MacroListDataTableWidget::startDrag(Qt::DropActions supportedActions)
{

    QTableWidget::startDrag(supportedActions);
}

void MacroListDataTableWidget::dropEvent(QDropEvent *event)
{

}

void MacroListDataTableWidget::dragEnterEvent(QDragEnterEvent *event)
{

}

void MacroListDataTableWidget::dragMoveEvent(QDragMoveEvent *event)
{

}

void MacroListDataTableWidget::updateRowVisibility()
{

}
