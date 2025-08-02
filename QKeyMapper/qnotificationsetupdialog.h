#ifndef QNOTIFICATIONSETUPDIALOG_H
#define QNOTIFICATIONSETUPDIALOG_H

#include <QDialog>
#include <QStyleFactory>
#include "colorpickerwidget.h"

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
    int getNotification_DisplayDuration(void);
    int getNotification_FadeInDuration(void);
    int getNotification_FadeOutDuration(void);

    // Get Notification Border Data
    int getNotification_BorderRadius(void);
    int getNotification_Padding(void);
    double getNotification_Opacity(void);

    // Get Notification Offset Data
    int getNotification_X_Offset(void);
    int getNotification_Y_Offset(void);

    // Set Notification Color
    void setNotification_FontColor(const QColor &color);
    void setNotification_BackgroundColor(const QColor &color);

    // Set Notification Font Data
    void setNotification_FontSize(int size);
    void setNotification_FontWeight(int weight);
    void setNotification_FontIsItalic(bool italic);

    // Set Notification Duration Data
    void setNotification_DisplayDuration(int duration);
    void setNotification_FadeInDuration(int duration);
    void setNotification_FadeOutDuration(int duration);

    // Set Notification Border Data
    void setNotification_BorderRadius(int radius);
    void setNotification_Padding(int padding);
    void setNotification_Opacity(double opacity);

    // Set Notification Offset Data
    void setNotification_X_Offset(int offset);
    void setNotification_Y_Offset(int offset);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

public slots:
    void onFontColorChanged(QColor &color);
    void onBackgroundColorChanged(QColor &color);

private:
    static QNotificationSetupDialog *m_instance;
    Ui::QNotificationSetupDialog *ui;
    ColorPickerWidget *m_FontColorPicker;
    ColorPickerWidget *m_BackgroundColorPicker;
};

#endif // QNOTIFICATIONSETUPDIALOG_H
