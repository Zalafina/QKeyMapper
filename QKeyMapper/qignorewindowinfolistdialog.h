#ifndef QIGNOREWINDOWINFOLISTDIALOG_H
#define QIGNOREWINDOWINFOLISTDIALOG_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class QIgnoreWindowInfoListDialog;
}

class QIgnoreWindowInfoListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QIgnoreWindowInfoListDialog(QWidget *parent = nullptr);
    ~QIgnoreWindowInfoListDialog();

    static QIgnoreWindowInfoListDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);
    void updateRulesListWidget(void);

protected:
    // bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void on_saveRuleButton_clicked();

    void on_clearRuleButton_clicked();

    void on_deleteRuleButton_clicked();

    void on_ruleNameLineEdit_textChanged(const QString &text);

private:
    void initRuleWindowInfoArea(void);

private:
    static QIgnoreWindowInfoListDialog *m_instance;
    Ui::QIgnoreWindowInfoListDialog *ui;
};

#endif // QIGNOREWINDOWINFOLISTDIALOG_H
