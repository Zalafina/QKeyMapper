#ifndef QMAPPINGSEQUENCEEDIT_H
#define QMAPPINGSEQUENCEEDIT_H

#include <QDialog>
#include <QTableWidget>

namespace Ui {
class QMappingSequenceEdit;
}

// MappingSequenceEditTableWidget: Custom table widget with drag-drop support
class MappingSequenceEditTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit MappingSequenceEditTableWidget(QWidget *parent = nullptr)
        : QTableWidget(parent), m_DraggedTopRow(-1), m_DraggedBottomRow(-1) {}

protected:
    // Override keyPressEvent to handle keyboard shortcuts for the editor.
    void keyPressEvent(QKeyEvent *event) override;
    // Drag and drop support for row reordering
    void startDrag(Qt::DropActions supportedActions) override;
    void dropEvent(QDropEvent *event) override;

private:
    int m_DraggedTopRow;                  // Top row index when dragging multi-selection
    int m_DraggedBottomRow;               // Bottom row index when dragging multi-selection
};

class QMappingSequenceEdit : public QDialog
{
    Q_OBJECT

public:
    explicit QMappingSequenceEdit(QWidget *parent = nullptr);
    ~QMappingSequenceEdit();

    static QMappingSequenceEdit *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);
    void setTitle(const QString &title);
    void setMappingSequence(const QString &mappingsequence);
    void setMappingSequenceEditType(int edit_type);
    void refreshMappingSequenceEditTableWidget(MappingSequenceEditTableWidget *mappingSequenceEditTable, const QStringList& mappingSequenceList);
    void updateMappingKeyListComboBox(void);
    QString joinCurentMappingSequenceTable(void);

    QPushButton* getMapListSelectKeyboardButton(void) const;
    QPushButton* getMapListSelectMouseButton(void) const;
    QPushButton* getMapListSelectGamepadButton(void) const;
    QPushButton* getMapListSelectFunctionButton(void) const;

    int getMappingSequenceEditType(void);

public:
    static QString getEditingMappingKeyText(void);
    static int getEditingMappingKeyCursorPosition(void);
    static void setEditingMappingKeyText(const QString &new_mappingkeytext);
    static QString getCurrentMapKeyListText(void);

    // Mapping sequence item operations (selected range)
    void selectedMappingKeyItemsMoveUp(void);
    void selectedMappingKeyItemsMoveDown(void);
    void selectedMappingKeyItemsMoveToTop(void);
    void selectedMappingKeyItemsMoveToBottom(void);
    void deleteMappingKeySelectedItems(void);

    // Mapping sequence highlight selection operations
    void highlightSelectUp(void);
    void highlightSelectDown(void);
    void highlightSelectExtendUp(void);
    void highlightSelectExtendDown(void);
    void highlightSelectFirst(void);
    void highlightSelectLast(void);
    void clearHighlightSelection(void);
    void highlightSelectLoadData(void);

    // Mapping sequence copy/paste operations
    int copySelectedMappingKeyToCopiedList(void);
    int insertMappingKeyFromCopiedList(void);
    int pasteMappingKeyFromCopiedList(void);

    // Mapping sequence undo/redo operations
    void undo(void);
    void redo(void);

public:
    static QStringList s_CopiedMappingSequenceList;

signals:
    // Signal for drag and drop move operation
    void mappingSequenceTableDragDropMove_Signal(int top_row, int bottom_row, int dragged_to);

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void MapkeyComboBox_currentTextChangedSlot(const QString &text);
    void insertMappingKeyToTable(void);

    // Slot for handling mapping sequence table item double click
    void mappingSequenceTableItemDoubleClicked(QTableWidgetItem *item);

    // Slot for handling drag and drop move
    void mappingSequenceTableDragDropMove(int top_row, int bottom_row, int dragged_to);

    // Slot for handling mapping sequence table cell changed
    void mappingSequenceTableCellChanged(int row, int column);

    void mappingSequenceTableItemSelectionChanged();

    void on_insertMappingKeyButton_clicked();

    void on_confirmButton_clicked();

    void on_cancelButton_clicked();

private:
    void initMappingSequenceEditTable(MappingSequenceEditTableWidget *mappingSequenceEditTable);
    void initKeyListComboBoxes(void);
    void updateMappingSequenceEditTableConnection(MappingSequenceEditTableWidget *mappingSequenceEditTable);

    int getInsertRowFromSelectionOrAppend(void) const;
public:
    void reselectionRangeAndScroll(int top_row, int bottom_row);
private:
    bool validateOrAllowEmptyMappingKey(QString *trimmedMappingKey, QString *popupMessage) const;
    void emitValidationFailurePopup(const QString &popupMessage) const;

private:
    struct MappingSequenceHistorySnapshot {
        QStringList mappingSequenceList;
        int selectionTopRow = -1;
        int selectionBottomRow = -1;
        int currentRow = -1;
        int currentColumn = -1;
        int verticalScrollValue = 0;
    };

    void clearHistory(void);
    MappingSequenceHistorySnapshot captureHistorySnapshot(void) const;
    void restoreHistorySnapshot(const MappingSequenceHistorySnapshot &snapshot);
    void commitHistorySnapshotIfNeeded(void);

    static QMappingSequenceEdit *m_instance;
    Ui::QMappingSequenceEdit *ui;
    QStringList m_MappingSequenceList;
    int m_MappingSequenceEditType;

    QVector<MappingSequenceHistorySnapshot> m_HistorySnapshots;
    int m_HistoryIndex;
    bool m_IsRestoringHistory;
};

#endif // QMAPPINGSEQUENCEEDIT_H
