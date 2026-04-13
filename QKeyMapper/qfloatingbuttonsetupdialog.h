#ifndef QFLOATINGBUTTONSETUPDIALOG_H
#define QFLOATINGBUTTONSETUPDIALOG_H

#include <QDialog>

#include "colorpickerwidget.h"
#include "qkeymapper_worker.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
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
    void refreshFromCurrentItem();

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
    void syncFontFamilyControls();

private:
    int m_ItemRow;
    bool m_isLoading;
    bool m_hasBackup;
    MAP_KEYDATA m_BackupData;
    QString m_BackupMousePassThroughSwitchKey;
    QString m_FontFamily;

    QCheckBox *m_EnableCheckBox;
    QLabel *m_LabelTextLabel;
    QLineEdit *m_LabelLineEdit;
    QCheckBox *m_ShowOnStartCheckBox;
    QCheckBox *m_ShowToolTipCheckBox;
    QCheckBox *m_SyncPressedLockedStateCheckBox;
    QCheckBox *m_AlwaysOnTopCheckBox;
    QCheckBox *m_MousePassThroughCheckBox;
    QCheckBox *m_DragToMoveCheckBox;

    QLabel *m_MousePassThroughSwitchKeyLabel;
    QComboBox *m_MousePassThroughSwitchKeyComboBox;

    QLabel *m_WidthLabel;
    QLabel *m_HeightLabel;
    QLabel *m_FontSizeLabel;
    QLabel *m_FontWeightLabel;
    QLabel *m_FontFamilyLabel;
    QLabel *m_RadiusLabel;
    QLabel *m_NormalOpacityLabel;
    QLabel *m_PressedOpacityLabel;
    QLabel *m_LockedOpacityLabel;

    QSpinBox *m_WidthSpinBox;
    QSpinBox *m_HeightSpinBox;
    QSpinBox *m_FontSizeSpinBox;
    QComboBox *m_FontWeightComboBox;
    QFontComboBox *m_FontFamilyComboBox;
    QPushButton *m_FontFamilyDefaultButton;
    QSpinBox *m_RadiusSpinBox;
    QDoubleSpinBox *m_NormalOpacitySpinBox;
    QDoubleSpinBox *m_PressedOpacitySpinBox;
    QDoubleSpinBox *m_LockedOpacitySpinBox;

    QLabel *m_ReferencePointLabel;
    QLabel *m_OffsetXLabel;
    QLabel *m_OffsetYLabel;

    QComboBox *m_ReferencePointComboBox;
    QSpinBox *m_OffsetXSpinBox;
    QSpinBox *m_OffsetYSpinBox;

    QPushButton *m_ApplyButton;
    QPushButton *m_RevertButton;

    ColorPickerWidget *m_ButtonColorPicker;
    ColorPickerWidget *m_PressedColorPicker;
    ColorPickerWidget *m_LockedColorPicker;
    ColorPickerWidget *m_TextColorPicker;
};

#endif // QFLOATINGBUTTONSETUPDIALOG_H

