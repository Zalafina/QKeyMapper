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

void QMacroListDialog::refreshMacroListTabWidget(MacroListDataTableWidget *macroDataTable, const OrderedMap<QString, MappingMacroData> &mappingMacroDataList)
{

    resizeMacroListTableColumnWidth(macroDataTable);

    updateMacroCategoryFilterComboBox();
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

bool QMacroListDialog::isMacroDataTableFiltered()
{
    if (ui->categoryFilterComboBox->currentIndex() != CATEGORY_FILTER_ALL_INDEX) {
        return true;
    }
    else {
        return false;
    }
}

void QMacroListDialog::onMacroCategoryFilterChanged(int index)
{

}

void QMacroListDialog::updateMacroCategoryFilterComboBox()
{

}

void QMacroListDialog::showEvent(QShowEvent *event)
{
    refreshMacroListTabWidget(ui->macrolistTable, QKeyMapper::s_MappingMacroList);
    refreshMacroListTabWidget(ui->universalmacrolistTable, QKeyMapper::s_UniversalMappingMacroList);

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
    MacroListDataTableWidget *macroDataTable = Q_NULLPTR;
    if (ui->macroListTabWidget->currentWidget() == ui->macrolist) {
        current_tab = CURRENT_TAB_MACRO;
        macroDataTable = ui->macrolistTable;
    }
    else if (ui->macroListTabWidget->currentWidget() == ui->universalmacrolist) {
        current_tab = CURRENT_TAB_UNIVERSAL_MACRO;
        macroDataTable = ui->universalmacrolistTable;
    }

    if (current_tab == CURRENT_TAB_NONE) {
        return;
    }

    OrderedMap<QString, MappingMacroData>& CurrentMacroList = (current_tab == CURRENT_TAB_MACRO) ? QKeyMapper::s_MappingMacroList : QKeyMapper::s_UniversalMappingMacroList;

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

    if (macro_str.isEmpty()) {
        return;
    }

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;

    if (macroname_str.isEmpty()) {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("Macro name cannot be empty.");
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    ValidationResult result = QKeyMapper::validateMappingMacroString(macro_str);

    if (!result.isValid) {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("Macro") + " -> " + result.errorMessage;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

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
    refreshMacroListTabWidget(macroDataTable, CurrentMacroList);
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
    tabWidget->setCurrentIndex(tabWidget->indexOf(ui->macrolist));

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
    macroDataTable->resizeColumnToContents(MACRO_NAME_COLUMN);

    int macro_name_width_min = macroDataTable->width()/5 - 15;
    int macro_name_width_max = macroDataTable->width() / 2;
    int macro_name_width = macroDataTable->columnWidth(MACRO_NAME_COLUMN);

    macroDataTable->horizontalHeader()->setStretchLastSection(false);
    int macro_category_width_max = macroDataTable->width() / 5;
    macroDataTable->resizeColumnToContents(MACRO_CATEGORY_COLUMN);
    int macro_category_width = macroDataTable->columnWidth(MACRO_CATEGORY_COLUMN);
    // if (macro_category_width < burst_mode_width) {
    //     macro_category_width = burst_mode_width;
    // }
    if (macro_category_width > macro_category_width_max) {
        macro_category_width = macro_category_width_max;
    }
    macroDataTable->horizontalHeader()->setStretchLastSection(true);

    if (macro_name_width < macro_name_width_min) {
        macro_name_width = macro_name_width_min;
    }
    else if (macro_name_width > macro_name_width_max) {
        macro_name_width = macro_name_width_max;
    }

    int macro_content_width_min = macroDataTable->width()/5 - 15;
    int macro_content_width = macroDataTable->width() - macro_name_width - macro_category_width - 16;
    if (macro_content_width < macro_content_width_min) {
        macro_content_width = macro_content_width_min;
    }

    macroDataTable->setColumnWidth(MACRO_NAME_COLUMN, macro_name_width);
    macroDataTable->setColumnWidth(MACRO_CONTENT_COLUMN, macro_content_width);
    macroDataTable->setColumnWidth(MACRO_CATEGORY_COLUMN, macro_category_width);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[resizeMacroListTableColumnWidth]" << "macroDataTable->rowCount" << macroDataTable->rowCount();
    qDebug() << "[resizeMacroListTableColumnWidth]" << "macro_name_width =" << macro_name_width << ", macro_content_width =" << macro_content_width << ", macro_category_width =" << macro_category_width;
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
