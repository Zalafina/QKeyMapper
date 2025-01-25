#ifndef QCROSSHAIRSETUPDIALOG_H
#define QCROSSHAIRSETUPDIALOG_H

#include <QDialog>
#include "colorpickerwidget.h"

namespace Ui {
class QCrosshairSetupDialog;
}

class QCrosshairSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QCrosshairSetupDialog(QWidget *parent = nullptr);
    ~QCrosshairSetupDialog();

    static QCrosshairSetupDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

private:
    static QCrosshairSetupDialog *m_instance;
    Ui::QCrosshairSetupDialog *ui;
    ColorPickerWidget *m_CenterColorPicker;
    ColorPickerWidget *m_CrosshairColorPicker;
};

#endif // QCROSSHAIRSETUPDIALOG_H
