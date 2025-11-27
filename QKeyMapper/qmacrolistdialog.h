#ifndef QMACROLISTDIALOG_H
#define QMACROLISTDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>

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
    bool isCategoryColumnVisible() const { return m_CategoryColumnVisible; }
    void setCategoryColumnVisible(bool visible);

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
    bool m_CategoryColumnVisible;         // Category column visibility state
    QSet<QString> m_CategoryFilter;       // Set of categories to filter by
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
    void refreshMacroListTabWidget(void);
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

protected:
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void on_addMacroButton_clicked();

private slots:
    void addMacroToList(void);

private:
    void initMacroListTabWidget(void);
    void initMacroListTable(MacroListDataTableWidget *macroDataTable);
    void initKeyListComboBoxes(void);
    void resizeMacroListTabWidgetColumnWidth(void);
    void resizeMacroListTableColumnWidth(MacroListDataTableWidget *macroDataTable);

private:
    static QMacroListDialog *m_instance;
    Ui::QMacroListDialog *ui;
};

#endif // QMACROLISTDIALOG_H
