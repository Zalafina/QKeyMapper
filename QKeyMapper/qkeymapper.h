#ifndef QLINKKEEPER_H
#define QLINKKEEPER_H

#include <QDialog>
#include <QDebug>
#include <QMetaEnum>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <QSystemTrayIcon>
#include <windows.h>
//#include <QKeyEvent>
//#include <QProcess>
//#include <QTextCodec>

namespace Ui {
class QKeyMapper;
}

class QKeyMapper : public QDialog
{
    Q_OBJECT

public:
    explicit QKeyMapper(QWidget *parent = 0);
    ~QKeyMapper();

    enum KeyMapStatus
    {
        KEYMAP_IDLE = 0U,
        KEYMAP_MAPPING
    };
    Q_ENUM(KeyMapStatus)

    enum VirtualKeyCode
    {
        VK_0 = 0x30,
        VK_1,
        VK_2,
        VK_3,
        VK_4,
        VK_5,
        VK_6,
        VK_7,
        VK_8,
        VK_9,

        VK_A = 0x41,
        VK_B,
        VK_C,
        VK_D,
        VK_E,
        VK_F,
        VK_G,
        VK_H,
        VK_I,
        VK_J,
        VK_K,
        VK_L,
        VK_M,
        VK_N,
        VK_O,
        VK_P,
        VK_Q,
        VK_R,
        VK_S,
        VK_T,
        VK_U,
        VK_V,
        VK_W,
        VK_X,
        VK_Y,
        VK_Z
    };
    Q_ENUM(VirtualKeyCode)

    Q_INVOKABLE void WindowStateChangedProc(void);

    void setKeyHook(void);

protected:
    void changeEvent(QEvent *event);
    //void keyPressEvent(QKeyEvent *event) override;
    //bool eventFilter(QObject *widgetobject, QEvent *event);

private slots:
    void SystrayIconActivated(QSystemTrayIcon::ActivationReason reason);


    void on_savemaplistButton_clicked();

    void on_keymapButton_clicked();

private:
    static LRESULT CALLBACK LowLevelKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    void saveKeyMapList(void);
    void loadKeyMapList(void);

private:
    Ui::QKeyMapper *ui;
    quint8 m_KeyMapStatus;
    QSystemTrayIcon *m_SysTrayIcon;
    HHOOK m_KeyHook;
};

#endif // QLINKKEEPER_H
