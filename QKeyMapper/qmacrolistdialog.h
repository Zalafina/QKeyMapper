#ifndef QMACROLISTDIALOG_H
#define QMACROLISTDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>

namespace Ui {
class QMacroListDialog;
}

class MacroListTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit MacroListTabWidget(QWidget *parent = Q_NULLPTR)
        : QTabWidget(parent) {}

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

class MacroListDataTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit MacroListDataTableWidget(QWidget *parent = Q_NULLPTR)
        : QTableWidget(parent), m_DraggedTopRow(-1), m_DraggedBottomRow(-1) {}

    // Category filtering methods
    void setCategoryFilter(const QString &category);
    void clearCategoryFilter();
    QStringList getAvailableCategories() const;

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;

private:
    void updateRowVisibility();

private:
    int m_DraggedTopRow;
    int m_DraggedBottomRow;
    bool m_CategoryColumnVisible = false;  // Category column visibility
public:
    QString m_CategoryFilter = QString();  // Current category filter
};

class QMacroListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QMacroListDialog(QWidget *parent = nullptr);
    ~QMacroListDialog();

    static QMacroListDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);
    void refreshMacroListTabWidget(void);

protected:
    void showEvent(QShowEvent *event) override;

private:
    static QMacroListDialog *m_instance;
    Ui::QMacroListDialog *ui;
};

#endif // QMACROLISTDIALOG_H
