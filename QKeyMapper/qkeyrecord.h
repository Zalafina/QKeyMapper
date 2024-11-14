#ifndef QKEYRECORD_H
#define QKEYRECORD_H

#include <QDialog>
#include <QClipboard>

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

    enum KeyRecordState
    {
        KEYRECORD_STATE_STOP = 0U,
        KEYRECORD_STATE_START
    };
    Q_ENUM(KeyRecordState)

signals:
    void updateKeyRecordLineEdit_Signal(bool finished);

public slots:
    void updateKeyRecordLineEdit(bool finished);

public:
    void procKeyRecordStart(void);
    void procKeyRecordStop(void);

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setKeyRecordLabel(KeyRecordState record_state);

private:
    static QKeyRecord *m_instance;
    Ui::QKeyRecord *ui;
};

#endif // QKEYRECORD_H
