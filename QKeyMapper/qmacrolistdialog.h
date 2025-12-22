#ifndef QMACROLISTDIALOG_H
#define QMACROLISTDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>

#include "qkeymapper_worker.h"

class ActionPopup;

namespace Ui {
class QMacroListDialog;
}

// MacroListTabWidget: Custom tab widget to handle keyboard events
class MacroListTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit MacroListTabWidget(QWidget *parent = nullptr) : QTabWidget(parent) {}

protected:
    // Override keyPressEvent to handle Esc key press for closing dialog
    void keyPressEvent(QKeyEvent *event) override;
};

// MacroListDataTableWidget: Custom table widget with drag-drop support and category filtering
class MacroListDataTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit MacroListDataTableWidget(QWidget *parent = nullptr)
        : QTableWidget(parent), m_DraggedTopRow(-1), m_DraggedBottomRow(-1) {}

    // Category filtering methods
    void setCategoryFilter(const QString &category);
    void clearCategoryFilter();
    QStringList getAvailableCategories() const;

protected:
    // Drag and drop support for row reordering
    void startDrag(Qt::DropActions supportedActions) override;
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;

private:
    void updateRowVisibility();

private:
    int m_DraggedTopRow;                  // Top row index when dragging multi-selection
    int m_DraggedBottomRow;               // Bottom row index when dragging multi-selection
public:
    QString m_CategoryFilter = QString();  // Current category filter
};

// QMacroListDialog: Main dialog class for displaying and managing macro lists
class QMacroListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QMacroListDialog(QWidget *parent = nullptr);
    ~QMacroListDialog() override;

    static QMacroListDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);
    void refreshMacroListTabWidget(MacroListDataTableWidget *macroDataTable, const OrderedMap<QString, MappingMacroData>& mappingMacroDataList);
    void refreshAllMacroListTabWidget(void);
    void updateMappingKeyListComboBox(void);

    QPushButton* getMapListSelectKeyboardButton(void) const;
    QPushButton* getMapListSelectMouseButton(void) const;
    QPushButton* getMapListSelectGamepadButton(void) const;
    QPushButton* getMapListSelectFunctionButton(void) const;

public:
    static QString getEditingMacroText(void);
    static int getEditingMacroCursorPosition(void);
    static void setEditingMacroText(const QString &new_macrotext);
    static QString getCurrentMapKeyListText(void);

    bool isMacroDataTableFiltered(void);
    void onMacroCategoryFilterChanged(int index);
    void updateMacroCategoryFilterComboBox(void);

    // Macro list item operations
    void selectedMacroItemsMoveUp(void);
    void selectedMacroItemsMoveDown(void);
    void deleteMacroSelectedItems(void);

signals:
    // Signal for drag and drop move operation
    void macroListTableDragDropMove_Signal(int top_row, int bottom_row, int dragged_to);

protected:
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    // Override resizeEvent to adjust table column widths when window is resized
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_addMacroButton_clicked();
    void on_clearButton_clicked();
    void on_deleteMacroButton_clicked();
    void on_macroListBackupButton_clicked();

    // Slot for handling macro table item double click
    void macroTableItemDoubleClicked(QTableWidgetItem *item);
    void macroListTabWidgetCurrentChanged(int index);

    // Slot for handling drag and drop move
    void macroListTableDragDropMove(int top_row, int bottom_row, int dragged_to);

    // Slot for handling macro table cell changed (category editing)
    void macroTableCellChanged(int row, int column);

private slots:
    void addMacroToList(void);

    // Slots for MacroList backup popup actions
    void macroListBackupActionTriggered(const QString &actionName);

private:
    void initMacroListTabWidget(void);
    void initMacroListTable(MacroListDataTableWidget *macroDataTable);
    void initKeyListComboBoxes(void);
    void initMacroListBackupActionPopup(void);
    void resizeMacroListTabWidgetColumnWidth(void);
    void resizeMacroListTableColumnWidth(MacroListDataTableWidget *macroDataTable);
    void updateMacroDataTableConnection(MacroListDataTableWidget *macroDataTable);

    // Helper methods to get current macro table and data list
    MacroListDataTableWidget* getCurrentMacroDataTable(void);
    OrderedMap<QString, MappingMacroData>* getCurrentMacroDataList(void);

    // MacroList export and import methods
    void exportMacroListToFile(void);
    void importMacroListFromFile(void);

private:
    static QMacroListDialog *m_instance;
    Ui::QMacroListDialog *ui;
    ActionPopup *m_MacroListBackupActionPopup = Q_NULLPTR;
};

#endif // QMACROLISTDIALOG_H
