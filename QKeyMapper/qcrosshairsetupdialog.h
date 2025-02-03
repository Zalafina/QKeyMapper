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

public slots:
    void onCenterColorChanged(QColor &color);
    void onCrosshairColorChanged(QColor &color);

private slots:
    void on_showCenterCheckBox_stateChanged(int state);

    void on_showTopCheckBox_stateChanged(int state);

    void on_showBottomCheckBox_stateChanged(int state);

    void on_showLeftCheckBox_stateChanged(int state);

    void on_showRightCheckBox_stateChanged(int state);

    void on_centerSizeSpinBox_editingFinished();

    void on_centerOpacitySpinBox_editingFinished();

    void on_crosshairLineWidthSpinBox_editingFinished();

    void on_crosshairLineLengthSpinBox_editingFinished();

    void on_crosshairOpacitySpinBox_editingFinished();

    void on_x_offsetSpinBox_editingFinished();

    void on_y_offsetSpinBox_editingFinished();

private:
    static QCrosshairSetupDialog *m_instance;
    Ui::QCrosshairSetupDialog *ui;
    int m_ItemRow;
    ColorPickerWidget *m_CenterColorPicker;
    ColorPickerWidget *m_CrosshairColorPicker;
};

#endif // QCROSSHAIRSETUPDIALOG_H
