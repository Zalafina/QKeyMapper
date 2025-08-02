#ifndef QGYRO2MOUSEOPTIONDIALOG_H
#define QGYRO2MOUSEOPTIONDIALOG_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class QGyro2MouseOptionDialog;
}

class QGyro2MouseOptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QGyro2MouseOptionDialog(QWidget *parent = nullptr);
    ~QGyro2MouseOptionDialog();

    static QGyro2MouseOptionDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

    static int getGyro2Mouse_MouseXSource(void);
    static int getGyro2Mouse_MouseYSource(void);
    static bool getGyro2Mouse_MouseXRevert(void);
    static bool getGyro2Mouse_MouseYRevert(void);

    void setGyro2Mouse_MouseXSource(int source_index);
    void setGyro2Mouse_MouseYSource(int source_index);
    void setGyro2Mouse_MouseXRevert(bool revert);
    void setGyro2Mouse_MouseYRevert(bool revert);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    static QGyro2MouseOptionDialog *m_instance;
    Ui::QGyro2MouseOptionDialog *ui;
};

#endif // QGYRO2MOUSEOPTIONDIALOG_H
