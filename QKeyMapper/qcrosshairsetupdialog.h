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
    void setItemRow(int row);

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    static QCrosshairSetupDialog *m_instance;
    Ui::QCrosshairSetupDialog *ui;
    int m_ItemRow;
    ColorPickerWidget *m_CenterColorPicker;
    ColorPickerWidget *m_CrosshairColorPicker;
};

#endif // QCROSSHAIRSETUPDIALOG_H
