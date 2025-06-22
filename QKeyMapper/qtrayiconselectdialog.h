#ifndef QTRAYICONSELECTDIALOG_H
#define QTRAYICONSELECTDIALOG_H

#include <QDialog>

namespace Ui {
class QTrayIconSelectDialog;
}

class QTrayIconSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QTrayIconSelectDialog(QWidget *parent = nullptr);
    ~QTrayIconSelectDialog();

    static QTrayIconSelectDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

protected:
    bool event(QEvent *event) override;

private:
    static QTrayIconSelectDialog *m_instance;
    Ui::QTrayIconSelectDialog *ui;
};

#endif // QTRAYICONSELECTDIALOG_H
