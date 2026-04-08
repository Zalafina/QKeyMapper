#ifndef QNOTIFICATIONSETUPDIALOG_H
#define QNOTIFICATIONSETUPDIALOG_H

#include <QDialog>
#include <QString>
#include <QStyleFactory>

#include "qkeymapper_constants.h"
#include "colorpickerwidget.h"

namespace Ui {
class QNotificationSetupDialog;
}

struct NotificationStyleSettings
{
    QColor fontColor = QKeyMapperConstants::NOTIFICATION_COLOR_NORMAL_DEFAULT;
    QColor backgroundColor = QKeyMapperConstants::NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
    QString fontFamily;
    int fontSize = QKeyMapperConstants::NOTIFICATION_FONT_SIZE_DEFAULT;
    int fontWeight = QKeyMapperConstants::NOTIFICATION_FONT_WEIGHT_DEFAULT;
    bool fontItalic = QKeyMapperConstants::NOTIFICATION_FONT_ITALIC_DEFAULT;
    int displayDuration = QKeyMapperConstants::NOTIFICATION_DISPLAY_DURATION_DEFAULT;
    int fadeInDuration = QKeyMapperConstants::NOTIFICATION_FADEIN_DURATION_DEFAULT;
    int fadeOutDuration = QKeyMapperConstants::NOTIFICATION_FADEOUT_DURATION_DEFAULT;
    int borderRadius = QKeyMapperConstants::NOTIFICATION_BORDER_RADIUS_DEFAULT;
    int padding = QKeyMapperConstants::NOTIFICATION_PADDING_DEFAULT;
    double opacity = QKeyMapperConstants::NOTIFICATION_OPACITY_DEFAULT;
    int xOffset = QKeyMapperConstants::NOTIFICATION_X_OFFSET_DEFAULT;
    int yOffset = QKeyMapperConstants::NOTIFICATION_Y_OFFSET_DEFAULT;
};

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
    QString getNotification_FontFamily(void) const;

    // Get Notification Border Data
    int getNotification_BorderRadius(void);
    int getNotification_Padding(void);
    double getNotification_Opacity(void);

    NotificationStyleSettings getNotificationSettings(void);

    // Get Notification Offset Data
    int getNotification_X_Offset(void);
    int getNotification_Y_Offset(void);

    // Set Notification Color
    void setNotification_FontColor(const QColor &color);
    void setNotification_BackgroundColor(const QColor &color);

    void setNotification_FontFamily(const QString &family);
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

    void loadNotificationSettings(const NotificationStyleSettings &settings);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

public slots:
    void onFontColorChanged(QColor &color);
    void onBackgroundColorChanged(QColor &color);

private:
    void syncFontFamilyControls(void);

    static QNotificationSetupDialog *m_instance;
    Ui::QNotificationSetupDialog *ui;
    QString m_FontFamily;
    ColorPickerWidget *m_FontColorPicker;
    ColorPickerWidget *m_BackgroundColorPicker;
};

#endif // QNOTIFICATIONSETUPDIALOG_H
