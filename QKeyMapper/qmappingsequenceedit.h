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

signals:
    // Signal for drag and drop move operation
    void mappingSequenceTableDragDropMove_Signal(int top_row, int bottom_row, int dragged_to);

protected:
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    // Slot for handling mapping sequence table item double click
    void mappingSequenceTableItemDoubleClicked(QTableWidgetItem *item);

    // Slot for handling drag and drop move
    void mappingSequenceTableDragDropMove(int top_row, int bottom_row, int dragged_to);

private:
    Ui::QMappingSequenceEdit *ui;
};

#endif // QMAPPINGSEQUENCEEDIT_H
