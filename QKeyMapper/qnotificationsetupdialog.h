#ifndef QNOTIFICATIONSETUPDIALOG_H
#define QNOTIFICATIONSETUPDIALOG_H

#include <QDialog>
#include <QStyleFactory>
#include "colorpickerwidget.h"
#include "qkeymapper_constants.h"

namespace Ui {
class QNotificationSetupDialog;
}

class QNotificationSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QNotificationSetupDialog(QWidget *parent = nullptr);
    ~QNotificationSetupDialog();

    static QNotificationSetupDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

    // Get Notification Color
    QColor getNotification_FontColor(void);
    QColor getNotification_BackgroundColor(void);

    // Get Notification Font Data
    int getNotification_FontSize(void);
    int getNotification_FontWeight(void);
    bool getNotification_FontIsItalic(void);

    // Get Notification Duration Data
    int getNotification_Duration(void);
    int getNotification_FadeInDuration(void);
    int getNotification_FadeOutDuration(void);

    // Get Notification Border Data
    int getNotification_BorderRadius(void);
    int getNotification_Padding(void);
    double getNotification_Opacity(void);

protected:
    bool event(QEvent *event) override;

public slots:
    void onFontColorChanged(QColor &color);
    void onBackgroundColorChanged(QColor &color);

private:
    static QNotificationSetupDialog *m_instance;
    Ui::QNotificationSetupDialog *ui;
    ColorPickerWidget *m_FontColorPicker;
    ColorPickerWidget *m_BackgroundColorPicker;
    QColor m_NotificationFontColor = NOTIFICATION_COLOR_NORMAL_DEFAULT;
    QColor m_NotificationBackgroundColor = NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
};

#endif // QNOTIFICATIONSETUPDIALOG_H
