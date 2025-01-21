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

private:
    Ui::QCrosshairSetupDialog *ui;
    ColorPickerWidget *m_CenterColorPicker;
    ColorPickerWidget *m_CrosshairColorPicker;
};

#endif // QCROSSHAIRSETUPDIALOG_H
