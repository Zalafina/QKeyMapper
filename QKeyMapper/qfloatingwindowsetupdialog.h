#ifndef QFLOATINGWINDOWSETUPDIALOG_H
#define QFLOATINGWINDOWSETUPDIALOG_H

#include <QDialog>
#include "colorpickerwidget.h"

namespace Ui {
class QFloatingWindowSetupDialog;
}

class QFloatingWindowSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QFloatingWindowSetupDialog(QWidget *parent = nullptr);
    ~QFloatingWindowSetupDialog();

    static QFloatingWindowSetupDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);
    void resetFontSize(void);
    void setTabIndex(int tabindex);

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

public slots:
    void onBackgroundColorChanged(QColor &color);

private slots:
    void on_windowSizeSpinBox_valueChanged(int value);

    void on_windowPositionXSpinBox_valueChanged(int position_x);

    void on_windowPositionYSpinBox_valueChanged(int position_y);

    void on_windowOpacitySpinBox_valueChanged(double value);

    void on_mousePassThroughCheckBox_stateChanged(int state);

    void on_windowRadiusSpinBox_valueChanged(int value);

    void on_referencePointComboBox_currentIndexChanged(int index);

    void on_mousePassThroughSwitchKeyComboBox_currentIndexChanged(int index);

private:
    static QFloatingWindowSetupDialog *m_instance;
    Ui::QFloatingWindowSetupDialog *ui;
    int m_TabIndex;
    ColorPickerWidget *m_FloatingWindow_BGColorPicker;
};

#endif // QFLOATINGWINDOWSETUPDIALOG_H
