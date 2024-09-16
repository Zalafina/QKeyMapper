#ifndef QTABLESETUPDIALOG_H
#define QTABLESETUPDIALOG_H

#include <QDialog>

namespace Ui {
class QTableSetupDialog;
}

class QTableSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QTableSetupDialog(QWidget *parent = nullptr);
    ~QTableSetupDialog();

    static QTableSetupDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguagee(int languageindex);
    void resetFontSize(void);
    void setTabIndex(int tabindex);

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void on_tabNameUpdateButton_clicked();

    void on_tabHotkeyUpdateButton_clicked();

    void on_exportTableButton_clicked();

    void on_importTableButton_clicked();

    void on_removeTableButton_clicked();

private:
    static QTableSetupDialog *m_instance;
    Ui::QTableSetupDialog *ui;
    int m_TabIndex;
};

#endif // QTABLESETUPDIALOG_H
