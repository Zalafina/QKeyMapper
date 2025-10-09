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

protected:
    // bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    static QIgnoreWindowInfoListDialog *m_instance;
    Ui::QIgnoreWindowInfoListDialog *ui;
};

#endif // QIGNOREWINDOWINFOLISTDIALOG_H
