#ifndef QFLOATINGBUTTONSETUPDIALOG_H
#define QFLOATINGBUTTONSETUPDIALOG_H

#include <QDialog>

#include "colorpickerwidget.h"
#include "qkeymapper_worker.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;

namespace Ui {
class QFloatingButtonSetupDialog;
}

class QFloatingButtonSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QFloatingButtonSetupDialog(QWidget *parent = nullptr);
    ~QFloatingButtonSetupDialog() override;

    void setUILanguage(int languageindex);
    void setItemRow(int row);
    int getItemRow() const;

signals:
    void settingsApplied();

protected:
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onApplyButtonClicked();
    void onRevertButtonClicked();
    void onAnyControlChanged();

private:
    void loadFromCurrentItem();
    void applyToCurrentItem();
    void setupReferencePointComboBox();

private:
    int m_ItemRow;
    bool m_isLoading;
    bool m_hasBackup;
    MAP_KEYDATA m_BackupData;

    QCheckBox *m_EnableCheckBox;
    QLineEdit *m_LabelLineEdit;
    QCheckBox *m_ShowOnStartCheckBox;
    QCheckBox *m_AlwaysOnTopCheckBox;
    QCheckBox *m_DragToMoveCheckBox;

    QSpinBox *m_WidthSpinBox;
    QSpinBox *m_HeightSpinBox;
    QSpinBox *m_FontSizeSpinBox;
    QComboBox *m_FontWeightComboBox;
    QSpinBox *m_RadiusSpinBox;
    QDoubleSpinBox *m_OpacitySpinBox;

    QComboBox *m_ReferencePointComboBox;
    QSpinBox *m_OffsetXSpinBox;
    QSpinBox *m_OffsetYSpinBox;

    ColorPickerWidget *m_ButtonColorPicker;
    ColorPickerWidget *m_PressedColorPicker;
    ColorPickerWidget *m_TextColorPicker;
};

#endif // QFLOATINGBUTTONSETUPDIALOG_H

