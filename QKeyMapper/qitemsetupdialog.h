#ifndef QITEMSETUPDIALOG_H
#define QITEMSETUPDIALOG_H

#include <QDialog>

namespace Ui {
class QItemSetupDialog;
}

class QItemSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QItemSetupDialog(QWidget *parent = nullptr);
    ~QItemSetupDialog();

    void setItemRow(int row);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    Ui::QItemSetupDialog *ui;
    int m_ItemRow;
};

#endif // QITEMSETUPDIALOG_H
