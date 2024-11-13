#ifndef QKEYRECORD_H
#define QKEYRECORD_H

#include <QDialog>

namespace Ui {
class QKeyRecord;
}

class QKeyRecord : public QDialog
{
    Q_OBJECT

public:
    explicit QKeyRecord(QWidget *parent = nullptr);
    ~QKeyRecord();

    static QKeyRecord *getInstance()
    {
        return m_instance;
    }

protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    static QKeyRecord *m_instance;
    Ui::QKeyRecord *ui;
};

#endif // QKEYRECORD_H
