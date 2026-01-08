#include "qmappingsequenceedit.h"
#include "ui_qmappingsequenceedit.h"
#include "qkeymapper.h"
#include "qkeymapper_constants.h"

#include <QScrollBar>

using namespace QKeyMapperConstants;

QMappingSequenceEdit *QMappingSequenceEdit::m_instance = Q_NULLPTR;
QStringList QMappingSequenceEdit::s_CopiedMappingSequenceList;

QMappingSequenceEdit::QMappingSequenceEdit(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QMappingSequenceEdit)
    , m_MappingSequenceList()
    , m_MappingSequenceEditType(MAPPINGSEQUENCEEDIT_TYPE_ITEMSETUP_MAPPINGKEYS)
    , m_HistorySnapshots()
    , m_HistoryIndex(-1)
    , m_IsRestoringHistory(false)
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
    ui->mappingKeyLabel->setFont(customFont);
    ui->mapkeyListLabel->setFont(customFont);
    ui->MappingSequenceEdit_MappingKeyLineEdit->setFont(customFont);
    ui->MappingSequenceEdit_MappingKeyListComboBox->setFont(customFont);
    ui->confirmButton->setFont(customFont);
    ui->cancelButton->setFont(customFont);
    ui->deleteButton->setFont(customFont);
    ui->undoButton->setFont(customFont);
    ui->redoButton->setFont(customFont);
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
    QObject::connect(ui->MappingSequenceEdit_MappingKeyListComboBox, &KeyListComboBox::currentTextChanged, this, &QMappingSequenceEdit::MapkeyComboBox_currentTextChangedSlot);

    // Connect drag and drop move signal
    QObject::connect(this, &QMappingSequenceEdit::mappingSequenceTableDragDropMove_Signal,
                     this, &QMappingSequenceEdit::mappingSequenceTableDragDropMove);

    if (QItemSetupDialog::getInstance() != Q_NULLPTR) {
        QItemSetupDialog::getInstance()->syncConnectMappingKeySelectButtons();
    }

    updateUndoRedoButtonsEnabled();
}

QMappingSequenceEdit::~QMappingSequenceEdit()
{
    delete ui;
}

void QMappingSequenceEdit::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);

    ui->mappingSequenceEditTable->setHorizontalHeaderLabels(QStringList() << tr("Split Mapping Sequence"));
    ui->confirmButton->setText(tr(CONFIRMBUTTON_STR));
    ui->cancelButton->setText(tr(CANCELBUTTON_STR));
    ui->deleteButton->setText(tr("Delete"));
    ui->undoButton->setText(tr("Undo"));
    ui->redoButton->setText(tr("Redo"));

    ui->mappingKeyLabel->setText(tr("MappingKey"));
    ui->mapkeyListLabel->setText(tr("MappingKeyList"));
    ui->insertMappingKeyButton->setText(tr("Insert"));

    ui->mapList_SelectKeyboardButton->setToolTip(tr("Keyboard Keys"));
    ui->mapList_SelectMouseButton->setToolTip(tr("Mouse Keys"));
    ui->mapList_SelectGamepadButton->setToolTip(tr("Gamepad Keys"));
    ui->mapList_SelectFunctionButton->setToolTip(tr("Function Keys"));
}

void QMappingSequenceEdit::setTitle(const QString &title)
{
    setWindowTitle(title);
}

void QMappingSequenceEdit::setMappingSequence(const QString &mappingsequence)
{
    // New content is being loaded. Reset undo/redo history (Excel-like).
    clearHistory();
    updateUndoRedoButtonsEnabled();

    QString trimmed_mappingsequence = QKeyMapper::getTrimmedMappingKeyString(mappingsequence);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] MappingKeyText after trimmed -> " << trimmed_mappingsequence;
#endif

    QStringList mappingKeySeqList = splitMappingKeyString(trimmed_mappingsequence, SPLIT_WITH_NEXT);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QMappingSequenceEdit::setMappingSequence]" << "Split Mapping Sequence List ->" << mappingKeySeqList;
#endif

    // Always update internal list. Empty input means clearing previous state.
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
    if (!mappingSequenceEditTable) {
        return;
    }

    QSignalBlocker blocker(mappingSequenceEditTable);
    mappingSequenceEditTable->clearContents();
    mappingSequenceEditTable->setRowCount(0);

    if (mappingSequenceList.isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshMappingSequenceEditTableWidget]" << "Empty mappingSequenceList";
#endif
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    QString debugmessage = QString("[refreshMappingSequenceEditTableWidget] mappingSequenceList(%1) Start >>>").arg(mappingSequenceList.size());
    qDebug().nospace().noquote() << debugmessage;
#endif

    int rowindex = 0;
    mappingSequenceEditTable->setRowCount(mappingSequenceList.size());

    for (const QString &mappingkeystr : mappingSequenceList) {
        /* MACRO_CONTENT_COLUMN */
        QTableWidgetItem *mappingkey_TableItem = new QTableWidgetItem(mappingkeystr);
        mappingSequenceEditTable->setItem(rowindex, MAPPINGSEQUENCEEDIT_MAPPINGKEY_COLUMN, mappingkey_TableItem);

        rowindex += 1;

#ifdef DEBUG_LOGOUT_ON
        QString debugmessage = QString("[refreshMappingSequenceEditTableWidget] row(%1) -> \"%2\"").arg(rowindex).arg(mappingkeystr);
        qDebug().nospace().noquote() << debugmessage;
#endif
    }

#ifdef DEBUG_LOGOUT_ON
    debugmessage = QString("[refreshMappingSequenceEditTableWidget] mappingSequenceList(%1) End <<<").arg(mappingSequenceList.size());
    qDebug().nospace().noquote() << debugmessage;
#endif
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
    QStringList currentMappingSequenceList = m_MappingSequenceList;
    currentMappingSequenceList.removeAll("");

    QString joined_mappingsequence = currentMappingSequenceList.join(SEPARATOR_NEXTARROW);

    joined_mappingsequence = QKeyMapper::getTrimmedMappingKeyString(joined_mappingsequence);

    return joined_mappingsequence;
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

QString QMappingSequenceEdit::getEditingMappingKeyText()
{
    return getInstance()->ui->MappingSequenceEdit_MappingKeyLineEdit->text();
}

int QMappingSequenceEdit::getEditingMappingKeyCursorPosition()
{
    return getInstance()->ui->MappingSequenceEdit_MappingKeyLineEdit->cursorPosition();
}

void QMappingSequenceEdit::setEditingMappingKeyText(const QString &new_mappingkeytext)
{
    return getInstance()->ui->MappingSequenceEdit_MappingKeyLineEdit->setText(new_mappingkeytext);
}

QString QMappingSequenceEdit::getCurrentMapKeyListText()
{
    return getInstance()->ui->MappingSequenceEdit_MappingKeyListComboBox->currentText();
}

void QMappingSequenceEdit::showEvent(QShowEvent *event)
{
    refreshMappingSequenceEditTableWidget(ui->mappingSequenceEditTable, m_MappingSequenceList);
    ui->mappingSequenceEditTable->setCurrentCell(-1, -1);

    updateUndoRedoButtonsEnabled();

    QDialog::showEvent(event);
}

void QMappingSequenceEdit::closeEvent(QCloseEvent *event)
{
    // Dialog instance is reused. Clear history to avoid cross-session undo/redo.
    clearHistory();
    updateUndoRedoButtonsEnabled();
    QDialog::closeEvent(event);
}

void QMappingSequenceEdit::clearHistory()
{
    m_HistorySnapshots.clear();
    m_HistoryIndex = -1;
}

void QMappingSequenceEdit::updateUndoRedoButtonsEnabled()
{
    if (!ui) {
        return;
    }

    const bool canUndo = (!m_HistorySnapshots.isEmpty() && m_HistoryIndex > 0);
    const bool canRedo = (!m_HistorySnapshots.isEmpty() && m_HistoryIndex >= 0 && m_HistoryIndex < (m_HistorySnapshots.size() - 1));

    if (ui->undoButton) {
        ui->undoButton->setEnabled(canUndo);
    }
    if (ui->redoButton) {
        ui->redoButton->setEnabled(canRedo);
    }
}

void QMappingSequenceEdit::ensureBaseSnapshotBeforeListChange()
{
    if (m_IsRestoringHistory) {
        return;
    }

    // Only create a base snapshot when the first list change happens.
    // This keeps Ctrl+Z ineffective right after opening the dialog.
    if (!m_HistorySnapshots.isEmpty()) {
        return;
    }

    const MappingSequenceHistorySnapshot base = captureHistorySnapshot();
    m_HistorySnapshots.append(base);
    m_HistoryIndex = 0;

    updateUndoRedoButtonsEnabled();
}

QMappingSequenceEdit::MappingSequenceHistorySnapshot QMappingSequenceEdit::captureHistorySnapshot() const
{
    MappingSequenceHistorySnapshot snapshot;
    snapshot.mappingSequenceList = m_MappingSequenceList;

    const MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return snapshot;
    }

    const QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (!ranges.isEmpty()) {
        const QTableWidgetSelectionRange range = ranges.first();
        snapshot.selectionTopRow = range.topRow();
        snapshot.selectionBottomRow = range.bottomRow();
    }

    snapshot.currentRow = table->currentRow();
    snapshot.currentColumn = table->currentColumn();
    if (QScrollBar *sb = table->verticalScrollBar()) {
        snapshot.verticalScrollValue = sb->value();
    }
    return snapshot;
}

void QMappingSequenceEdit::restoreHistorySnapshot(const MappingSequenceHistorySnapshot &snapshot)
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        m_MappingSequenceList = snapshot.mappingSequenceList;
        return;
    }

    m_IsRestoringHistory = true;
    QSignalBlocker blocker(table);

    m_MappingSequenceList = snapshot.mappingSequenceList;
    refreshMappingSequenceEditTableWidget(table, m_MappingSequenceList);

    table->clearSelection();

    const int lastRow = table->rowCount() - 1;
    if (lastRow >= 0 && snapshot.selectionTopRow >= 0 && snapshot.selectionBottomRow >= 0) {
        const int top = qBound(0, snapshot.selectionTopRow, lastRow);
        const int bottom = qBound(top, snapshot.selectionBottomRow, lastRow);
        QTableWidgetSelectionRange range(top, 0, bottom, 0);
        table->setRangeSelected(range, true);
    }

    if (lastRow >= 0 && snapshot.currentRow >= 0) {
        const int r = qBound(0, snapshot.currentRow, lastRow);
        const int c = qBound(0, snapshot.currentColumn, table->columnCount() - 1);
        table->setCurrentCell(r, c, QItemSelectionModel::NoUpdate);
    }
    else {
        table->setCurrentCell(-1, -1);
    }

    // Restore scroll after selection/current to avoid scrollToItem overriding it.
    if (QScrollBar *sb = table->verticalScrollBar()) {
        sb->setValue(snapshot.verticalScrollValue);
    }

    m_IsRestoringHistory = false;

    updateUndoRedoButtonsEnabled();
}

void QMappingSequenceEdit::commitHistorySnapshotIfNeeded()
{
    if (m_IsRestoringHistory) {
        return;
    }

    const MappingSequenceHistorySnapshot snapshot = captureHistorySnapshot();

    // Skip empty steps: only record when the list content actually changes.
    if (m_HistoryIndex >= 0 && m_HistoryIndex < m_HistorySnapshots.size()) {
        if (m_HistorySnapshots.at(m_HistoryIndex).mappingSequenceList == snapshot.mappingSequenceList) {
            return;
        }
    }

    // If we have undone some steps, discard redo branch when a new change is committed.
    if (m_HistoryIndex >= 0 && m_HistoryIndex < m_HistorySnapshots.size() - 1) {
        m_HistorySnapshots.resize(m_HistoryIndex + 1);
    }

    m_HistorySnapshots.append(snapshot);
    m_HistoryIndex = m_HistorySnapshots.size() - 1;

    const int maxSnapshots = qMax(2, MAPPINGSEQUENCEEDIT_HISTORY_MAX + 1);
    while (m_HistorySnapshots.size() > maxSnapshots) {
        m_HistorySnapshots.remove(0);
        m_HistoryIndex = qMax(0, m_HistoryIndex - 1);
    }

    updateUndoRedoButtonsEnabled();
}

void QMappingSequenceEdit::undo()
{
    if (m_IsRestoringHistory) {
        return;
    }
    if (m_HistoryIndex <= 0 || m_HistorySnapshots.isEmpty()) {
        return;
    }
    m_HistoryIndex--;
    restoreHistorySnapshot(m_HistorySnapshots.at(m_HistoryIndex));

    updateUndoRedoButtonsEnabled();
}

void QMappingSequenceEdit::redo()
{
    if (m_IsRestoringHistory) {
        return;
    }
    if (m_HistoryIndex < 0 || m_HistorySnapshots.isEmpty()) {
        return;
    }
    if (m_HistoryIndex >= m_HistorySnapshots.size() - 1) {
        return;
    }
    m_HistoryIndex++;
    restoreHistorySnapshot(m_HistorySnapshots.at(m_HistoryIndex));

    updateUndoRedoButtonsEnabled();
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

void QMappingSequenceEdit::MapkeyComboBox_currentTextChangedSlot(const QString &text)
{
    if (!text.isEmpty()) {
        ui->MappingSequenceEdit_MappingKeyListComboBox->setToolTip(text);
    }
}

void QMappingSequenceEdit::insertMappingKeyToTable()
{
    QString mappingkeystr = ui->MappingSequenceEdit_MappingKeyLineEdit->text();
    mappingkeystr = QKeyMapper::getTrimmedMappingKeyString(mappingkeystr);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[" << __func__ << "] MappingKeyText after trimmed -> " << mappingkeystr;
#endif

    QString popupMessage;
    QString trimmed = mappingkeystr;
    if (!validateOrAllowEmptyMappingKey(&trimmed, &popupMessage)) {
        emitValidationFailurePopup(popupMessage);
        return;
    }

    const int insertRow = getInsertRowFromSelectionOrAppend();
    if (insertRow < 0) {
        return;
    }

    ensureBaseSnapshotBeforeListChange();

    if (insertRow >= m_MappingSequenceList.size()) {
        m_MappingSequenceList.append(trimmed);
    } else {
        m_MappingSequenceList.insert(insertRow, trimmed);
    }

    refreshMappingSequenceEditTableWidget(ui->mappingSequenceEditTable, m_MappingSequenceList);
    reselectionRangeAndScroll(insertRow, insertRow);

    commitHistorySnapshotIfNeeded();
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

void MappingSequenceEditTableWidget::keyPressEvent(QKeyEvent *event)
{
    QMappingSequenceEdit *dlg = QMappingSequenceEdit::getInstance();
    if (!dlg || QKeyMapper::KEYMAP_IDLE != QKeyMapper::getInstance()->m_KeyMapStatus) {
        QTableWidget::keyPressEvent(event);
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[MappingSequenceEditTableWidget::keyPressEvent]" << "Key:" << (Qt::Key)event->key() << ", Modifiers:" << event->modifiers() << ", State:" << this->state();
#endif

    if (this->state() == QAbstractItemView::EditingState) {
        QTableWidget::keyPressEvent(event);
        return;
    }

    // Excel-like undo/redo (table-level only). When editing a cell, Ctrl+Z/Ctrl+Y is handled by the editor.
    const Qt::KeyboardModifiers mods = event->modifiers();
    if ((mods & Qt::ControlModifier) && !(mods & (Qt::AltModifier | Qt::MetaModifier))) {
        if (event->key() == Qt::Key_Z && !(mods & Qt::ShiftModifier)) {
            dlg->undo();
            return;
        }
        if (event->key() == Qt::Key_Y) {
            dlg->redo();
            return;
        }
    }

    switch (event->key()) {
    case Qt::Key_F2: {
        // Enter edit mode for current item when there is an active highlight selection.
        // This matches common Windows table behavior.
        const QList<QTableWidgetSelectionRange> ranges = selectedRanges();
        QTableWidgetItem *cur = currentItem();
        if (!ranges.isEmpty() && cur) {
            const int r = cur->row();
            const int c = cur->column();
            if (r >= 0 && r < rowCount() && c >= 0 && c < columnCount()) {
                dlg->reselectionRangeAndScroll(r, r);
                editItem(cur);
                return;
            }
        }
        break;
    }
    case Qt::Key_Up:
        if (event->modifiers() & Qt::ControlModifier) {
            if ((event->modifiers() & Qt::ShiftModifier)) {
                dlg->selectedMappingKeyItemsMoveToTop();
            }
            else {
                dlg->selectedMappingKeyItemsMoveUp();
            }
        }
        else {
            QTableWidget::keyPressEvent(event);
        }
        return;
    case Qt::Key_Down:
        if (event->modifiers() & Qt::ControlModifier) {
            if ((event->modifiers() & Qt::ShiftModifier)) {
                dlg->selectedMappingKeyItemsMoveToBottom();
            }
            else {
                dlg->selectedMappingKeyItemsMoveDown();
            }
        }
        else {
            QTableWidget::keyPressEvent(event);
        }
        return;
    case Qt::Key_Delete:
        dlg->deleteMappingKeySelectedItems();
        return;
    case Qt::Key_Home:
        if (event->modifiers() & Qt::ControlModifier) {
            dlg->selectedMappingKeyItemsMoveToTop();
        }
        else {
            dlg->highlightSelectFirst();
        }
        return;
    case Qt::Key_End:
        if (event->modifiers() & Qt::ControlModifier) {
            dlg->selectedMappingKeyItemsMoveToBottom();
        }
        else {
            dlg->highlightSelectLast();
        }
        return;
    case Qt::Key_Backspace:
        dlg->clearHighlightSelection();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        dlg->highlightSelectLoadData();
        return;
    default:
        break;
    }

    if (event->key() == Qt::Key_C && (event->modifiers() & Qt::ControlModifier)) {
        dlg->copySelectedMappingKeyToCopiedList();
        return;
    }
    if (event->key() == Qt::Key_V && (event->modifiers() & Qt::ControlModifier)) {
        dlg->pasteMappingKeyFromCopiedList();
        return;
    }
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        dlg->insertMappingKeyFromCopiedList();
        return;
    }

    QTableWidget::keyPressEvent(event);
}

void QMappingSequenceEdit::on_insertMappingKeyButton_clicked()
{
    insertMappingKeyToTable();
}

void QMappingSequenceEdit::on_confirmButton_clicked()
{
    QString joined_mappingsequence = joinCurentMappingSequenceTable();

    if (m_MappingSequenceEditType == MAPPINGSEQUENCEEDIT_TYPE_ITEMSETUP_MAPPINGKEYS) {
        QItemSetupDialog::getInstance()->updateMappingKeyLineEdit(joined_mappingsequence);
    }
    else if (m_MappingSequenceEditType == MAPPINGSEQUENCEEDIT_TYPE_ITEMSETUP_MAPPINGKEYS_KEYUP) {
        QItemSetupDialog::getInstance()->updateMappingKey_KeyUpLineEdit(joined_mappingsequence);
    }

    close();
}

void QMappingSequenceEdit::on_cancelButton_clicked()
{
    close();
}

void QMappingSequenceEdit::on_deleteButton_clicked()
{
    deleteMappingKeySelectedItems();
    updateUndoRedoButtonsEnabled();
}


void QMappingSequenceEdit::on_undoButton_clicked()
{
    undo();
    updateUndoRedoButtonsEnabled();
}


void QMappingSequenceEdit::on_redoButton_clicked()
{
    redo();
    updateUndoRedoButtonsEnabled();
}

void QMappingSequenceEdit::initMappingSequenceEditTable(MappingSequenceEditTableWidget *mappingSequenceEditTable)
{
    mappingSequenceEditTable->setFocusPolicy(Qt::ClickFocus);
    mappingSequenceEditTable->setColumnCount(MAPPINGSEQUENCEEDIT_TABLE_COLUMN_COUNT);

    mappingSequenceEditTable->horizontalHeader()->setStretchLastSection(true);
    mappingSequenceEditTable->horizontalHeader()->setHighlightSections(false);

    mappingSequenceEditTable->verticalHeader()->setDefaultSectionSize(25);
    mappingSequenceEditTable->verticalHeader()->setStyleSheet("QHeaderView::section { color: #1A9EDB; padding-left: 2px; padding-right: 1px;}");
    mappingSequenceEditTable->setSelectionBehavior(QAbstractItemView::SelectRows);;
    mappingSequenceEditTable->setSelectionMode(QAbstractItemView::ContiguousSelection);
    // Enable double-click editing for category column
    mappingSequenceEditTable->setEditTriggers(QAbstractItemView::DoubleClicked);

    /* Support Drag&Drop for macroDataTable Table */
    mappingSequenceEditTable->setDragEnabled(true);
    mappingSequenceEditTable->setDragDropMode(QAbstractItemView::InternalMove);
    mappingSequenceEditTable->setDefaultDropAction(Qt::MoveAction);

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
    if (!mappingSequenceEditTable) {
        return;
    }

    QObject::connect(mappingSequenceEditTable, &QTableWidget::cellChanged,
                     this, &QMappingSequenceEdit::mappingSequenceTableCellChanged, Qt::UniqueConnection);
    QObject::connect(mappingSequenceEditTable, &QTableWidget::itemSelectionChanged,
                     this, &QMappingSequenceEdit::mappingSequenceTableItemSelectionChanged, Qt::UniqueConnection);
    QObject::connect(mappingSequenceEditTable, &QTableWidget::itemDoubleClicked,
                     this, &QMappingSequenceEdit::mappingSequenceTableItemDoubleClicked, Qt::UniqueConnection);
}

int QMappingSequenceEdit::getInsertRowFromSelectionOrAppend() const
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return -1;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return m_MappingSequenceList.size();
    }

    const QTableWidgetSelectionRange range = ranges.first();
    const int topRow = range.topRow();
    return qBound(0, topRow, m_MappingSequenceList.size());
}

void QMappingSequenceEdit::reselectionRangeAndScroll(int top_row, int bottom_row)
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table || table->rowCount() <= 0) {
        return;
    }

    top_row = qBound(0, top_row, table->rowCount() - 1);
    bottom_row = qBound(0, bottom_row, table->rowCount() - 1);

    table->clearSelection();
    QTableWidgetSelectionRange sel(top_row, 0, bottom_row, MAPPINGSEQUENCEEDIT_TABLE_COLUMN_COUNT - 1);
    table->setRangeSelected(sel, true);

    // Keep current cell aligned to the selection for Ctrl/Shift+Click consistency.
    table->setCurrentCell(top_row, 0, QItemSelectionModel::NoUpdate);

    if (QTableWidgetItem *item = table->item(top_row, 0)) {
        table->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
}

bool QMappingSequenceEdit::validateOrAllowEmptyMappingKey(QString *trimmedMappingKey, QString *popupMessage) const
{
    if (!trimmedMappingKey) {
        return false;
    }

    *trimmedMappingKey = QKeyMapper::getTrimmedMappingKeyString(*trimmedMappingKey);
    if (trimmedMappingKey->isEmpty()) {
        // Empty mapping key is allowed in this editor.
        return true;
    }

    ValidationResult result = QKeyMapper::validateMappingKeyString(*trimmedMappingKey);
    if (!result.isValid) {
        if (popupMessage) {
            *popupMessage = tr("MappingKey") + " -> " + result.errorMessage;
        }
        return false;
    }

    return true;
}

void QMappingSequenceEdit::emitValidationFailurePopup(const QString &popupMessage) const
{
    if (popupMessage.isEmpty()) {
        return;
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, FAILURE_COLOR, POPUP_MESSAGE_DISPLAY_TIME_DEFAULT);
}

void MappingSequenceEditTableWidget::startDrag(Qt::DropActions supportedActions)
{
    QList<QTableWidgetSelectionRange> ranges = this->selectedRanges();
    if (!ranges.isEmpty()) {
        const QTableWidgetSelectionRange range = ranges.first();
        m_DraggedTopRow = range.topRow();
        m_DraggedBottomRow = range.bottomRow();
    }
    QTableWidget::startDrag(supportedActions);
}

void MappingSequenceEditTableWidget::dropEvent(QDropEvent *event)
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

        if (QMappingSequenceEdit::getInstance()) {
            emit QMappingSequenceEdit::getInstance()->mappingSequenceTableDragDropMove_Signal(m_DraggedTopRow, m_DraggedBottomRow, droppedRow);
        }
    }
}

void QMappingSequenceEdit::mappingSequenceTableDragDropMove(int top_row, int bottom_row, int dragged_to)
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    const int rowCount = m_MappingSequenceList.size();
    if (rowCount <= 0) {
        return;
    }

    top_row = qBound(0, top_row, rowCount - 1);
    bottom_row = qBound(0, bottom_row, rowCount - 1);
    if (top_row > bottom_row) {
        qSwap(top_row, bottom_row);
    }

    dragged_to = qBound(0, dragged_to, rowCount - 1);

    const int draggedCount = bottom_row - top_row + 1;
    if (draggedCount <= 0) {
        return;
    }

    // If dropping inside the dragged range, no-op.
    if (dragged_to >= top_row && dragged_to <= bottom_row) {
        return;
    }

    ensureBaseSnapshotBeforeListChange();

    const bool isDraggedToBottom = (dragged_to > bottom_row);

    QStringList moved;
    moved.reserve(draggedCount);
    for (int i = top_row; i <= bottom_row; ++i) {
        moved.append(m_MappingSequenceList.at(i));
    }

    // Remove original block
    for (int i = 0; i < draggedCount; ++i) {
        m_MappingSequenceList.removeAt(top_row);
    }

    int insertPos = dragged_to;
    if (isDraggedToBottom) {
        // List shrinks above the drop target.
        insertPos = dragged_to - draggedCount + 1;
    }
    insertPos = qBound(0, insertPos, m_MappingSequenceList.size());

    for (int i = 0; i < moved.size(); ++i) {
        m_MappingSequenceList.insert(insertPos + i, moved.at(i));
    }

    refreshMappingSequenceEditTableWidget(table, m_MappingSequenceList);

    const int newTop = insertPos;
    const int newBottom = insertPos + draggedCount - 1;
    reselectionRangeAndScroll(newTop, newBottom);
    table->setCurrentCell(isDraggedToBottom ? newBottom : newTop, 0, QItemSelectionModel::NoUpdate);

    commitHistorySnapshotIfNeeded();
}

void QMappingSequenceEdit::mappingSequenceTableCellChanged(int row, int column)
{
    if (column != MAPPINGSEQUENCEEDIT_MAPPINGKEY_COLUMN) {
        return;
    }

    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    if (row < 0 || row >= table->rowCount()) {
        return;
    }

    QTableWidgetItem *item = table->item(row, column);
    if (!item) {
        return;
    }

    QString newText = item->text();
    QString trimmed = newText;

    QString popupMessage;
    if (!validateOrAllowEmptyMappingKey(&trimmed, &popupMessage)) {
        // Roll back UI from the source-of-truth list.
        const QString oldText = (row >= 0 && row < m_MappingSequenceList.size()) ? m_MappingSequenceList.at(row) : QString();
        QSignalBlocker blocker(table);
        item->setText(oldText);
        // item->setToolTip(oldText);
        emitValidationFailurePopup(popupMessage);
        return;
    }

    // Commit into the source-of-truth list (only after validation).
    const QString oldCommitted = (row >= 0 && row < m_MappingSequenceList.size()) ? m_MappingSequenceList.at(row) : QString();
    const bool listWillChange = (row >= m_MappingSequenceList.size()) || (oldCommitted != trimmed);

    if (listWillChange) {
        ensureBaseSnapshotBeforeListChange();
    }

    if (row >= m_MappingSequenceList.size()) {
        m_MappingSequenceList.resize(row + 1);
    }
    m_MappingSequenceList[row] = trimmed;

    // Keep tooltip in sync for better UX.
    // if (item->toolTip() != trimmed) {
    //     item->setToolTip(trimmed);
    // }

    // Normalize UI text (e.g., remove spaces) without re-triggering cellChanged.
    if (newText != trimmed) {
        QSignalBlocker blocker(table);
        item->setText(trimmed);
        // item->setToolTip(trimmed);
    }

    if (listWillChange) {
        commitHistorySnapshotIfNeeded();
    }
}

void QMappingSequenceEdit::mappingSequenceTableItemSelectionChanged()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    // Clear current cell to avoid unexpected Ctrl/Shift+Click selection behavior.
    if (table->selectedRanges().isEmpty()) {
        if (table->currentRow() != -1 || table->currentColumn() != -1) {
            table->setCurrentCell(-1, -1);
        }
    }
}

void QMappingSequenceEdit::selectedMappingKeyItemsMoveUp()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return;
    }
    QTableWidgetSelectionRange range = ranges.first();
    int top = range.topRow();
    int bottom = range.bottomRow();
    if (top <= 0 || top >= m_MappingSequenceList.size()) {
        return;
    }

    ensureBaseSnapshotBeforeListChange();
    bottom = qBound(top, bottom, m_MappingSequenceList.size() - 1);

    const int count = bottom - top + 1;
    QStringList moved;
    for (int i = 0; i < count; ++i) {
        moved.append(m_MappingSequenceList.at(top + i));
    }
    for (int i = 0; i < count; ++i) {
        m_MappingSequenceList.removeAt(top);
    }
    const int insertPos = top - 1;
    for (int i = 0; i < moved.size(); ++i) {
        m_MappingSequenceList.insert(insertPos + i, moved.at(i));
    }

    refreshMappingSequenceEditTableWidget(table, m_MappingSequenceList);
    reselectionRangeAndScroll(insertPos, insertPos + count - 1);
    table->setCurrentCell(insertPos, 0, QItemSelectionModel::NoUpdate);

    commitHistorySnapshotIfNeeded();
}

void QMappingSequenceEdit::selectedMappingKeyItemsMoveDown()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return;
    }
    QTableWidgetSelectionRange range = ranges.first();
    int top = range.topRow();
    int bottom = range.bottomRow();
    const int last = m_MappingSequenceList.size() - 1;
    if (last < 0) {
        return;
    }
    top = qBound(0, top, last);
    bottom = qBound(top, bottom, last);
    if (bottom >= last) {
        return;
    }

    ensureBaseSnapshotBeforeListChange();

    const int count = bottom - top + 1;
    QStringList moved;
    for (int i = 0; i < count; ++i) {
        moved.append(m_MappingSequenceList.at(top + i));
    }
    for (int i = 0; i < count; ++i) {
        m_MappingSequenceList.removeAt(top);
    }
    const int insertPos = top + 1;
    for (int i = 0; i < moved.size(); ++i) {
        m_MappingSequenceList.insert(insertPos + i, moved.at(i));
    }

    refreshMappingSequenceEditTableWidget(table, m_MappingSequenceList);
    reselectionRangeAndScroll(insertPos, insertPos + count - 1);
    table->setCurrentCell(insertPos + count - 1, 0, QItemSelectionModel::NoUpdate);

    commitHistorySnapshotIfNeeded();
}

void QMappingSequenceEdit::selectedMappingKeyItemsMoveToTop()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return;
    }
    QTableWidgetSelectionRange range = ranges.first();
    int top = range.topRow();
    int bottom = range.bottomRow();
    const int last = m_MappingSequenceList.size() - 1;
    if (last < 0) {
        return;
    }
    top = qBound(0, top, last);
    bottom = qBound(top, bottom, last);
    if (top == 0) {
        return;
    }

    ensureBaseSnapshotBeforeListChange();

    const int count = bottom - top + 1;
    QStringList moved;
    for (int i = 0; i < count; ++i) {
        moved.append(m_MappingSequenceList.at(top + i));
    }
    for (int i = 0; i < count; ++i) {
        m_MappingSequenceList.removeAt(top);
    }
    for (int i = 0; i < moved.size(); ++i) {
        m_MappingSequenceList.insert(i, moved.at(i));
    }

    refreshMappingSequenceEditTableWidget(table, m_MappingSequenceList);
    reselectionRangeAndScroll(0, count - 1);
    table->setCurrentCell(0, 0, QItemSelectionModel::NoUpdate);

    commitHistorySnapshotIfNeeded();
}

void QMappingSequenceEdit::selectedMappingKeyItemsMoveToBottom()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return;
    }
    QTableWidgetSelectionRange range = ranges.first();
    int top = range.topRow();
    int bottom = range.bottomRow();
    const int last = m_MappingSequenceList.size() - 1;
    if (last < 0) {
        return;
    }
    top = qBound(0, top, last);
    bottom = qBound(top, bottom, last);
    if (bottom == last) {
        return;
    }

    ensureBaseSnapshotBeforeListChange();

    const int count = bottom - top + 1;
    QStringList moved;
    for (int i = 0; i < count; ++i) {
        moved.append(m_MappingSequenceList.at(top + i));
    }
    for (int i = 0; i < count; ++i) {
        m_MappingSequenceList.removeAt(top);
    }

    const int insertPos = m_MappingSequenceList.size();
    for (const QString &s : std::as_const(moved)) {
        m_MappingSequenceList.append(s);
    }

    refreshMappingSequenceEditTableWidget(table, m_MappingSequenceList);
    reselectionRangeAndScroll(insertPos, insertPos + count - 1);
    table->setCurrentCell(insertPos + count - 1, 0, QItemSelectionModel::NoUpdate);

    commitHistorySnapshotIfNeeded();
}

void QMappingSequenceEdit::deleteMappingKeySelectedItems()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return;
    }

    QTableWidgetSelectionRange range = ranges.first();
    int top = range.topRow();
    int bottom = range.bottomRow();
    const int last = m_MappingSequenceList.size() - 1;
    if (last < 0) {
        return;
    }
    top = qBound(0, top, last);
    bottom = qBound(top, bottom, last);

    ensureBaseSnapshotBeforeListChange();

    const int count = bottom - top + 1;
    for (int i = 0; i < count; ++i) {
        m_MappingSequenceList.removeAt(top);
    }

    refreshMappingSequenceEditTableWidget(table, m_MappingSequenceList);

    if (m_MappingSequenceList.isEmpty()) {
        clearHighlightSelection();
        commitHistorySnapshotIfNeeded();
        return;
    }

    const int newRow = qMin(top, m_MappingSequenceList.size() - 1);
    reselectionRangeAndScroll(newRow, newRow);

    commitHistorySnapshotIfNeeded();
}

void QMappingSequenceEdit::highlightSelectUp()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table || table->rowCount() <= 0) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        highlightSelectLast();
        return;
    }
    const QTableWidgetSelectionRange range = ranges.first();
    const int top = range.topRow();
    if (top <= 0) {
        return;
    }
    const int newRow = top - 1;
    reselectionRangeAndScroll(newRow, newRow);
}

void QMappingSequenceEdit::highlightSelectDown()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table || table->rowCount() <= 0) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        highlightSelectFirst();
        return;
    }
    const QTableWidgetSelectionRange range = ranges.first();
    const int bottom = range.bottomRow();
    if (bottom >= table->rowCount() - 1) {
        return;
    }
    const int newRow = bottom + 1;
    reselectionRangeAndScroll(newRow, newRow);
}

void QMappingSequenceEdit::highlightSelectExtendUp()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table || table->rowCount() <= 0) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        highlightSelectUp();
        return;
    }

    QTableWidgetSelectionRange range = ranges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();
    int currentRow = table->currentRow();
    if (currentRow < 0) {
        currentRow = topRow;
    }

    const bool currentAtTop = (currentRow <= topRow);
    const bool currentAtBottom = (currentRow >= bottomRow);

    int newTop = topRow;
    int newBottom = bottomRow;
    int newCurrent = currentRow;

    if (currentAtTop || (!currentAtTop && !currentAtBottom)) {
        if (topRow <= 0) {
            return;
        }
        newTop = topRow - 1;
        newCurrent = newTop;
    }
    else if (currentAtBottom) {
        if (topRow >= bottomRow) {
            return;
        }
        newBottom = bottomRow - 1;
        newCurrent = newBottom;
    }

    table->clearSelection();
    QTableWidgetSelectionRange sel(newTop, 0, newBottom, MAPPINGSEQUENCEEDIT_TABLE_COLUMN_COUNT - 1);
    table->setRangeSelected(sel, true);
    table->setCurrentCell(newCurrent, 0, QItemSelectionModel::NoUpdate);
    if (QTableWidgetItem *item = table->item(newCurrent, 0)) {
        table->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
}

void QMappingSequenceEdit::highlightSelectExtendDown()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table || table->rowCount() <= 0) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        highlightSelectDown();
        return;
    }

    QTableWidgetSelectionRange range = ranges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();
    int currentRow = table->currentRow();
    if (currentRow < 0) {
        currentRow = bottomRow;
    }

    const bool currentAtTop = (currentRow <= topRow);
    const bool currentAtBottom = (currentRow >= bottomRow);

    int newTop = topRow;
    int newBottom = bottomRow;
    int newCurrent = currentRow;

    if (currentAtBottom || (!currentAtTop && !currentAtBottom)) {
        if (bottomRow >= table->rowCount() - 1) {
            return;
        }
        newBottom = bottomRow + 1;
        newCurrent = newBottom;
    }
    else if (currentAtTop) {
        if (topRow >= bottomRow) {
            return;
        }
        newTop = topRow + 1;
        newCurrent = newTop;
    }

    table->clearSelection();
    QTableWidgetSelectionRange sel(newTop, 0, newBottom, MAPPINGSEQUENCEEDIT_TABLE_COLUMN_COUNT - 1);
    table->setRangeSelected(sel, true);
    table->setCurrentCell(newCurrent, 0, QItemSelectionModel::NoUpdate);
    if (QTableWidgetItem *item = table->item(newCurrent, 0)) {
        table->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
}

void QMappingSequenceEdit::highlightSelectFirst()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table || table->rowCount() <= 0) {
        return;
    }
    reselectionRangeAndScroll(0, 0);
}

void QMappingSequenceEdit::highlightSelectLast()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table || table->rowCount() <= 0) {
        return;
    }
    const int last = table->rowCount() - 1;
    reselectionRangeAndScroll(last, last);
}

void QMappingSequenceEdit::clearHighlightSelection()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }
    table->clearSelection();
    table->setCurrentCell(-1, -1);
}

void QMappingSequenceEdit::highlightSelectLoadData()
{
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return;
    }

    QTableWidgetSelectionRange range = ranges.first();
    if (range.topRow() != range.bottomRow()) {
        return;
    }

    const int row = range.topRow();
    if (row < 0 || row >= table->rowCount()) {
        return;
    }

    if (QTableWidgetItem *item = table->item(row, MAPPINGSEQUENCEEDIT_MAPPINGKEY_COLUMN)) {
        ui->MappingSequenceEdit_MappingKeyLineEdit->setText(item->text());
    }
}

int QMappingSequenceEdit::copySelectedMappingKeyToCopiedList()
{
    int copiedCount = -1;
    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return copiedCount;
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return copiedCount;
    }

    QTableWidgetSelectionRange range = ranges.first();
    int top = range.topRow();
    int bottom = range.bottomRow();
    const int last = m_MappingSequenceList.size() - 1;
    if (last < 0) {
        return copiedCount;
    }
    top = qBound(0, top, last);
    bottom = qBound(top, bottom, last);

    s_CopiedMappingSequenceList.clear();
    for (int row = top; row <= bottom; ++row) {
        s_CopiedMappingSequenceList.append(m_MappingSequenceList.at(row));
    }

    copiedCount = s_CopiedMappingSequenceList.size();
    return copiedCount;
}

int QMappingSequenceEdit::insertMappingKeyFromCopiedList()
{
    int insertedCount = 0;
    if (s_CopiedMappingSequenceList.isEmpty()) {
        return insertedCount;
    }

    const int insertRow = getInsertRowFromSelectionOrAppend();
    if (insertRow < 0) {
        return insertedCount;
    }

    ensureBaseSnapshotBeforeListChange();

    if (insertRow >= m_MappingSequenceList.size()) {
        for (const QString &s : std::as_const(s_CopiedMappingSequenceList)) {
            m_MappingSequenceList.append(s);
            insertedCount++;
        }
    } else {
        int pos = insertRow;
        for (const QString &s : std::as_const(s_CopiedMappingSequenceList)) {
            m_MappingSequenceList.insert(pos, s);
            pos++;
            insertedCount++;
        }
    }

    refreshMappingSequenceEditTableWidget(ui->mappingSequenceEditTable, m_MappingSequenceList);
    if (insertedCount > 0) {
        reselectionRangeAndScroll(insertRow, insertRow + insertedCount - 1);
        commitHistorySnapshotIfNeeded();
    }

    return insertedCount;
}

int QMappingSequenceEdit::pasteMappingKeyFromCopiedList()
{
    int pastedCount = 0;

    MappingSequenceEditTableWidget *table = ui ? ui->mappingSequenceEditTable : Q_NULLPTR;
    if (!table) {
        return pastedCount;
    }

    if (s_CopiedMappingSequenceList.size() != 1) {
        // Fallback to Ctrl+V behavior when copied content is not a single row.
        return insertMappingKeyFromCopiedList();
    }

    QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
    if (ranges.isEmpty()) {
        return pastedCount;
    }

    const QTableWidgetSelectionRange range = ranges.first();
    const int topRow = range.topRow();
    const int bottomRow = range.bottomRow();
    if (topRow != bottomRow) {
        // Fallback to Ctrl+V behavior when selection is not a single row.
        return insertMappingKeyFromCopiedList();
    }

    if (topRow < 0 || topRow >= table->rowCount()) {
        return pastedCount;
    }

    QTableWidgetItem *item = table->item(topRow, MAPPINGSEQUENCEEDIT_MAPPINGKEY_COLUMN);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(topRow, MAPPINGSEQUENCEEDIT_MAPPINGKEY_COLUMN, item);
    }

    const QString oldText = (topRow >= 0 && topRow < m_MappingSequenceList.size())
        ? m_MappingSequenceList.at(topRow)
        : QString();
    const QString expected = QKeyMapper::getTrimmedMappingKeyString(s_CopiedMappingSequenceList.first());

    // Trigger validation through the existing cellChanged mechanism.
    // On failure: cellChanged will roll back from m_MappingSequenceList and show the popup.
    reselectionRangeAndScroll(topRow, topRow);
    item->setText(s_CopiedMappingSequenceList.first());

    const QString committed = (topRow >= 0 && topRow < m_MappingSequenceList.size())
        ? m_MappingSequenceList.at(topRow)
        : QString();
    if (committed != oldText && committed == expected) {
        pastedCount = 1;
    }
    return pastedCount;
}
