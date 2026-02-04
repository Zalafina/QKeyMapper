#ifndef QKEYRECORD_H
#define QKEYRECORD_H

#include <QDialog>
#include <QClipboard>

#include "qkeymapper_worker.h"

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

    void setUILanguage(int languageindex);
    void resetFontSize(void);

signals:
    void updateKeyRecordLineEdit_Signal(bool finished);

public slots:
    void updateKeyRecordLineEdit(bool finished);

public:
    void procKeyRecordStart(bool clicked);
    void procKeyRecordStop(bool clicked);

protected:
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void on_recordStartStopButton_clicked();

private:
    void setKeyRecordLabel(KeyRecordState record_state);

public:
    static QAtomicBool s_KeyRecordDiagShow;

private:
    static QKeyRecord *m_instance;
    Ui::QKeyRecord *ui;
};

#endif // QKEYRECORD_H
