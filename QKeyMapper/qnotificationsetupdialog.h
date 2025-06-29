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
};

#endif // QNOTIFICATIONSETUPDIALOG_H
