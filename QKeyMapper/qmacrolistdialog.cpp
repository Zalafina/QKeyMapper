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
    macroDataTable->clearContents();
    macroDataTable->setRowCount(0);

    if (false == mappingMacroDataList.isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshMacroListTabWidget]" << "mappingMacroDataList Start >>>";
#endif
        int rowindex = 0;
        macroDataTable->setRowCount(mappingMacroDataList.size());

        // Iterate through the OrderedMap using keys()
        QList<QString> macroNameList = mappingMacroDataList.keys();
        for (const QString& macroName : std::as_const(macroNameList))
        {
            const MappingMacroData& macroData = mappingMacroDataList.value(macroName);

            /* MACRO_NAME_COLUMN */
            QTableWidgetItem *name_TableItem = new QTableWidgetItem(macroName);
            name_TableItem->setToolTip(macroName);
            name_TableItem->setFlags(name_TableItem->flags() & ~Qt::ItemIsEditable); // Make read-only
            macroDataTable->setItem(rowindex, MACRO_NAME_COLUMN, name_TableItem);

            /* MACRO_CONTENT_COLUMN */
            QTableWidgetItem *content_TableItem = new QTableWidgetItem(macroData.MappingMacro);
            content_TableItem->setToolTip(macroData.MappingMacro);
            content_TableItem->setFlags(content_TableItem->flags() & ~Qt::ItemIsEditable); // Make read-only
            macroDataTable->setItem(rowindex, MACRO_CONTENT_COLUMN, content_TableItem);

            /* MACRO_CATEGORY_COLUMN */
            QTableWidgetItem *category_TableItem = new QTableWidgetItem(macroData.Category);
            category_TableItem->setToolTip(macroData.Category);
            // Category column is always editable (always visible)
            category_TableItem->setFlags(category_TableItem->flags() | Qt::ItemIsEditable);
            macroDataTable->setItem(rowindex, MACRO_CATEGORY_COLUMN, category_TableItem);

            rowindex += 1;

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[refreshMacroListTabWidget] " << macroName << " -> " << macroData.MappingMacro << ", Category -> " << macroData.Category;
#endif
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshMacroListTabWidget]" << "mappingMacroDataList End <<<";
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshMacroListTabWidget]" << "Empty mappingMacroDataList";
#endif
    }

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

void QMacroListDialog::on_clearButton_clicked()
{

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
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[MacroListTabWidget::keyPressEvent]" << "Key:" << (Qt::Key)event->key() << "Modifiers:" << event->modifiers();
#endif

    QMacroListDialog *macroListDialog = QMacroListDialog::getInstance();
    if (macroListDialog && QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus) {
        if (event->key() == Qt::Key_Up) {
            // Move selected items up
            // TODO: Implement selectedMacroItemsMoveUp()
            // macroListDialog->selectedMacroItemsMoveUp();
            // return;
        }
        else if (event->key() == Qt::Key_Down) {
            // Move selected items down
            // TODO: Implement selectedMacroItemsMoveDown()
            // macroListDialog->selectedMacroItemsMoveDown();
            // return;
        }
        else if (event->key() == Qt::Key_Delete) {
            // Delete selected items
            // TODO: Implement deleteMacroButton_clicked()
            // macroListDialog->on_deleteMacroButton_clicked();
            // return;
        }
    }

    QTabWidget::keyPressEvent(event);
}

void MacroListDataTableWidget::setCategoryFilter(const QString &category)
{
    m_CategoryFilter = category;
    updateRowVisibility();
}

void MacroListDataTableWidget::clearCategoryFilter()
{
    m_CategoryFilter.clear();
    updateRowVisibility();
}

QStringList MacroListDataTableWidget::getAvailableCategories() const
{
    QStringList categories;

    // Category column is always visible in macro list dialog
    bool hasNonEmptyCategories = false;
    bool hasEmptyCategories = false;

    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *categoryItem = item(row, MACRO_CATEGORY_COLUMN);
        QString category;

        if (categoryItem) {
            category = categoryItem->text().trimmed();
        }

        if (category.isEmpty()) {
            hasEmptyCategories = true;
        } else {
            hasNonEmptyCategories = true;
            if (!categories.contains(category)) {
#ifdef DEBUG_LOGOUT_ON
                // Avoid confusion with the built-in "All" option
                // Users can still create an "All" category, but we'll warn about it
                if (category == tr("All")) {
                    qDebug() << "[getAvailableCategories]" << "Warning: Found user-created category named 'All', which may cause confusion with the built-in 'All' option";
                }
#endif
                categories.append(category);
            }
        }
    }

    // Add "(Blanks)" option only if there are both non-empty and empty categories
    // If all categories are empty, showing "(Blanks)" would be the same as "All"
    if (hasEmptyCategories && hasNonEmptyCategories) {
        categories.append(tr("Blank"));
    }

    return categories;
}

void MacroListDataTableWidget::startDrag(Qt::DropActions supportedActions)
{
    QList<QTableWidgetSelectionRange> selectedRanges = this->selectedRanges();
    if (!selectedRanges.isEmpty()) {
        QTableWidgetSelectionRange range = selectedRanges.first();
        m_DraggedTopRow = range.topRow();
        m_DraggedBottomRow = range.bottomRow();
    }
    QTableWidget::startDrag(supportedActions);
}

void MacroListDataTableWidget::dropEvent(QDropEvent *event)
{
    if (event->dropAction() == Qt::MoveAction) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        int droppedRow = rowAt(event->position().toPoint().y());
#else
        int droppedRow = rowAt(event->pos().y());
#endif

        if (droppedRow < 0) {
            droppedRow = rowCount() - 1;
        }

        // TODO: Emit signal to handle macro list reordering
        // emit QMacroListDialog::getInstance()->macroListTableDragDropMove_Signal(m_DraggedTopRow, m_DraggedBottomRow, droppedRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MacroListDataTableWidget::dropEvent]" << "Drag from" << m_DraggedTopRow << "to" << m_DraggedBottomRow << "dropped at" << droppedRow;
#endif
    }
}

void MacroListDataTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (QMacroListDialog::getInstance()->isMacroDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the macro table is filtered!");
        QKeyMapper::getInstance()->showWarningPopup(message);
        event->ignore();
    } else {
        QTableWidget::dragEnterEvent(event);
    }
}

void MacroListDataTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (QMacroListDialog::getInstance()->isMacroDataTableFiltered()) {
        event->ignore();
    } else {
        QTableWidget::dragMoveEvent(event);
    }
}

void MacroListDataTableWidget::updateRowVisibility()
{
    if (m_CategoryFilter.isEmpty()) {
        // Show all rows when no filter is active
        for (int row = 0; row < rowCount(); ++row) {
            setRowHidden(row, false);
        }
        return;
    }

    // Filter rows based on category
    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *categoryItem = item(row, MACRO_CATEGORY_COLUMN);
        bool shouldShow = false;

        if (m_CategoryFilter == tr("Blank")) {
            // Show rows with empty/blank categories
            if (categoryItem) {
                QString itemCategory = categoryItem->text().trimmed();
                shouldShow = itemCategory.isEmpty();
            } else {
                // No item means it's also considered blank
                shouldShow = true;
            }
        } else {
            // Show rows matching the specific category
            if (categoryItem) {
                QString itemCategory = categoryItem->text().trimmed();
                shouldShow = (itemCategory == m_CategoryFilter);
            }
        }

        setRowHidden(row, !shouldShow);
    }
}
