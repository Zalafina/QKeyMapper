#ifndef QFLOATINGWINDOWSETUPDIALOG_H
#define QFLOATINGWINDOWSETUPDIALOG_H

#include <QDialog>

namespace Ui {
class QFloatingWindowSetupDialog;
}

class QFloatingWindowSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QFloatingWindowSetupDialog(QWidget *parent = nullptr);
    ~QFloatingWindowSetupDialog();

    static QFloatingWindowSetupDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);
    void resetFontSize(void);
    void setTabIndex(int tabindex);

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    static QFloatingWindowSetupDialog *m_instance;
    Ui::QFloatingWindowSetupDialog *ui;
    int m_TabIndex;
};

#endif // QFLOATINGWINDOWSETUPDIALOG_H
