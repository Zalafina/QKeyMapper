#include "qmappingsequenceedit.h"
#include "ui_qmappingsequenceedit.h"
#include "qkeymapper.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QMappingSequenceEdit::QMappingSequenceEdit(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QMappingSequenceEdit)
{
    ui->setupUi(this);

    // Connect drag and drop move signal
    QObject::connect(this, &QMappingSequenceEdit::mappingSequenceTableDragDropMove_Signal,
                     this, &QMappingSequenceEdit::mappingSequenceTableDragDropMove);
}

QMappingSequenceEdit::~QMappingSequenceEdit()
{
    delete ui;
}

void QMappingSequenceEdit::showEvent(QShowEvent *event)
{
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

void QMappingSequenceEdit::mappingSequenceTableItemDoubleClicked(QTableWidgetItem *item)
{
    if (item == Q_NULLPTR) {
        return;
    }

    int rowindex = item->row();
    int columnindex = item->column();
    Q_UNUSED(columnindex);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[macroTableItemDoubleClicked]" << "Row" << rowindex << "Column" << columnindex << "DoubleClicked";
#endif

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
        QTableWidgetItem *mappingkeyItem = mappingSequenceTable->item(rowindex, MACRO_NAME_COLUMN);

        if (mappingkeyItem) {
            // ui->macroNameLineEdit->setText(nameItem->text());
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

void QMappingSequenceEdit::mappingSequenceTableDragDropMove(int top_row, int bottom_row, int dragged_to)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[mappingSequenceTableDragDropMove] DragDrop : Rows" << top_row << ":" << bottom_row << "->" << dragged_to;
#endif

#if 0
    MappingSequenceEditTableWidget *mappingSequenceTable = ui->mappingSequenceEditTable;

    int mappingdata_size = macroDataList->size();
    if (top_row >= 0 && bottom_row < mappingdata_size && dragged_to >= 0 && dragged_to < mappingdata_size
        && (dragged_to > bottom_row || dragged_to < top_row)) {

        int draged_row_count = bottom_row - top_row + 1;
        bool isDraggedToBottom = (dragged_to > bottom_row);

        // Get all keys in order
        QList<QString> keysList = macroDataList->keys();

        // Extract keys to be moved
        QList<QString> movedKeys;
        for (int i = top_row; i <= bottom_row; ++i) {
            movedKeys.append(keysList.at(i));
        }

        // Remove moved keys from list
        for (int i = bottom_row; i >= top_row; --i) {
            keysList.removeAt(i);
        }

        // Calculate insert position
        int insertPos;
        if (isDraggedToBottom) {
            insertPos = dragged_to - draged_row_count + 1;
        } else {
            insertPos = dragged_to;
        }

        // Insert moved keys at new position
        for (int i = 0; i < movedKeys.size(); ++i) {
            keysList.insert(insertPos + i, movedKeys.at(i));
        }

        // Rebuild the OrderedMap with new order
        OrderedMap<QString, MappingMacroData> newMacroList;
        for (const QString &key : std::as_const(keysList)) {
            newMacroList[key] = macroDataList->value(key);
        }
        *macroDataList = newMacroList;

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[macroListTableDragDropMove] : refreshMacroListTabWidget()";
#endif
        refreshMacroListTabWidget(macroDataTable, *macroDataList);

        // Reselect the moved rows
        QTableWidgetSelectionRange newSelection;
        if (isDraggedToBottom) {
            newSelection = QTableWidgetSelectionRange(dragged_to - draged_row_count + 1, 0, dragged_to, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
        } else {
            newSelection = QTableWidgetSelectionRange(dragged_to, 0, dragged_to + draged_row_count - 1, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
        }
        macroDataTable->clearSelection();
        macroDataTable->setRangeSelected(newSelection, true);

        // Update current cell based on drag direction for Ctrl/Shift+Click consistency
        if (isDraggedToBottom) {
            macroDataTable->setCurrentCell(newSelection.bottomRow(), 0, QItemSelectionModel::NoUpdate);
        } else {
            macroDataTable->setCurrentCell(newSelection.topRow(), 0, QItemSelectionModel::NoUpdate);
        }

#ifdef DEBUG_LOGOUT_ON
        if (macroDataTable->rowCount() != macroDataList->size()) {
            qDebug("macroListTableDragDropMove : MacroData sync error!!! DataTableSize(%d), DataListSize(%d)", macroDataTable->rowCount(), macroDataList->size());
        }
#endif
    }
#endif
}

void MappingSequenceEditTableWidget::keyPressEvent(QKeyEvent *event)
{
    QTableWidget::keyPressEvent(event);
}

void MappingSequenceEditTableWidget::startDrag(Qt::DropActions supportedActions)
{
    QList<QTableWidgetSelectionRange> selectedRanges = this->selectedRanges();
    if (!selectedRanges.isEmpty()) {
        QTableWidgetSelectionRange range = selectedRanges.first();
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

        // Emit signal to handle macro list reordering
        emit qobject_cast<QMappingSequenceEdit*>(this->parentWidget())->mappingSequenceTableDragDropMove_Signal(m_DraggedTopRow, m_DraggedBottomRow, droppedRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MappingSequenceEditTableWidget::dropEvent]" << "Drag from" << m_DraggedTopRow << "to" << m_DraggedBottomRow << "dropped at" << droppedRow;
#endif
    }
}
