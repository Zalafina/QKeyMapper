#ifndef QMAPPINGADVANCEDDIALOG_H
#define QMAPPINGADVANCEDDIALOG_H

#include <QDialog>
#include <QMouseEvent>
#include <QStyleFactory>

namespace Ui {
class QMappingAdvancedDialog;
}

class QMappingAdvancedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QMappingAdvancedDialog(QWidget *parent = nullptr);
    ~QMappingAdvancedDialog();

    static QMappingAdvancedDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

    void initGamepadThresholdSpinBoxes(void);

    int getMouseXSpeed(void);
    int getMouseYSpeed(void);
    int getMousePollingInterval(void);
    bool getProcessIconAsTrayIcon(void);
    bool getAcceptVirtualGamepadInput(void);
    QString getShowWindowPointKey(void);
    QString getShowScreenPointKey(void);
    int getLeftTriggerPressThreshold(void);
    int getLeftTriggerLightPressThreshold(void);
    int getLeftTriggerReleaseThreshold(void);
    int getRightTriggerPressThreshold(void);
    int getRightTriggerLightPressThreshold(void);
    int getRightTriggerReleaseThreshold(void);
    int getLeftStickPushThreshold(void);
    int getLeftStickLightPushThreshold(void);
    int getLeftStickReleaseThreshold(void);
    int getRightStickPushThreshold(void);
    int getRightStickLightPushThreshold(void);
    int getRightStickReleaseThreshold(void);
    bool getCustomNotificationEnabled(void);
    int getCustomNotificationPosition(void);

    void setMouseXSpeed(int speed);
    void setMouseYSpeed(int speed);
    void setMousePollingInterval(int interval);
    void setProcessIconAsTrayIcon(bool checked);
    void setAcceptVirtualGamepadInput(bool checked);
    void setShowWindowPointKey(const QString &keyname);
    void setShowScreenPointKey(const QString &keyname);
    void setLeftTriggerPressThreshold(int threshold);
    void setLeftTriggerLightPressThreshold(int threshold);
    void setLeftTriggerReleaseThreshold(int threshold);
    void setRightTriggerPressThreshold(int threshold);
    void setRightTriggerLightPressThreshold(int threshold);
    void setRightTriggerReleaseThreshold(int threshold);
    void setLeftStickPushThreshold(int threshold);
    void setLeftStickLightPushThreshold(int threshold);
    void setLeftStickReleaseThreshold(int threshold);
    void setRightStickPushThreshold(int threshold);
    void setRightStickLightPushThreshold(int threshold);
    void setRightStickReleaseThreshold(int threshold);
    void setCustomNotificationEnabled(bool enabled);
    void setCustomNotificationPosition(int position);

    void setProcessIconAsTrayIconEnabled(bool enabled);

signals:
    void customNotificationSetupRequested(void);
    void customNotificationEnabledChanged(bool enabled);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateCustomNotificationState(void);

    static QMappingAdvancedDialog *m_instance;
    Ui::QMappingAdvancedDialog *ui;
};

#endif // QMAPPINGADVANCEDDIALOG_H
