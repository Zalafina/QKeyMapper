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
    // Override keyPressEvent to handle Esc key press for closing dialog
    void keyPressEvent(QKeyEvent *event) override;
    // Drag and drop support for row reordering
    // void startDrag(Qt::DropActions supportedActions) override;
    // void dropEvent(QDropEvent *event) override;

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

protected:
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void MapkeyComboBox_currentTextChangedSlot(const QString &text);
    void insertMappingKeyToTable(void);

    // Slot for handling mapping sequence table item double click
    void mappingSequenceTableItemDoubleClicked(QTableWidgetItem *item);

    void on_insertMappingKeyButton_clicked();

    void on_confirmButton_clicked();

    void on_cancelButton_clicked();

private:
    void initMappingSequenceEditTable(MappingSequenceEditTableWidget *mappingSequenceEditTable);
    void initKeyListComboBoxes(void);
    void updateMappingSequenceEditTableConnection(MappingSequenceEditTableWidget *mappingSequenceEditTable);

private:
    static QMappingSequenceEdit *m_instance;
    Ui::QMappingSequenceEdit *ui;
    QStringList m_MappingSequenceList;
    int m_MappingSequenceEditType;
};

#endif // QMAPPINGSEQUENCEEDIT_H
