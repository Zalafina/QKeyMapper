#ifndef QFLOATINGBUTTONSETUPDIALOG_H
#define QFLOATINGBUTTONSETUPDIALOG_H

#include <QDialog>

#include "colorpickerwidget.h"
#include "qkeymapper_worker.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QDialogButtonBox;
class QFontComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QResizeEvent;
class QSpinBox;
class QWidget;

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
    void setPreferredLayoutMode(int layoutMode);
    int getPreferredLayoutMode() const;
    void refreshFromCurrentItem();

signals:
    void settingsApplied();

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onApplyButtonClicked();
    void onRevertButtonClicked();
    void onAnyControlChanged();

private:
    void rebuildContentLayout();
    void adjustDialogSizeForCurrentLayout();
    void applyDialogLayoutMode(int layoutMode, bool markDirty = false, bool force = false);
    void updateLayoutModeFromWidth(int width, bool markDirty = false);
    int preferredHorizontalEnterWidth() const;
    int preferredVerticalWidth() const;
    void loadFromCurrentItem();
    void applyToCurrentItem();
    void updateStyleCodeDisplay();
    void copyStyleCodeToClipboard();
    void applyClipboardStyleCode();
    void setupReferencePointComboBox();
    void syncFontFamilyControls();
    void updateHoverCustomizationState();

private:
    int m_ItemRow;
    bool m_isLoading;
    bool m_hasBackup;
    MAP_KEYDATA m_BackupData;
    QString m_BackupMousePassThroughSwitchKey;
    QString m_FontFamily;
    int m_LayoutMode;
    bool m_isRelayouting;
    bool m_isAutoResizingForLayout;
    int m_PreferredVerticalWidth;

    QWidget *m_ContentWidget;

    QGroupBox *m_InfoGroup;
    QGroupBox *m_BasicGroup;
    QGroupBox *m_StyleGroup;
    QGroupBox *m_PositionGroup;
    QLabel *m_ItemOriginalKeyLabel;
    QLineEdit *m_ItemOriginalKeyLineEdit;
    QLabel *m_ItemNoteLabel;
    QLineEdit *m_ItemNoteLineEdit;
    QLabel *m_ItemIndexLabel;
    QLineEdit *m_ItemIndexLineEdit;

    QCheckBox *m_EnableCheckBox;
    QLabel *m_LabelTextLabel;
    QLineEdit *m_LabelLineEdit;
    QCheckBox *m_ShowOnStartCheckBox;
    QCheckBox *m_ShowToolTipCheckBox;
    QCheckBox *m_SyncPressedLockedStateCheckBox;
    QCheckBox *m_AlwaysOnTopCheckBox;
    QCheckBox *m_MousePassThroughCheckBox;
    QCheckBox *m_DragToMoveCheckBox;
    QCheckBox *m_EnableGradientFillCheckBox;
    QCheckBox *m_EnableHoverAnimationCheckBox;

    QLabel *m_MousePassThroughSwitchKeyLabel;
    QComboBox *m_MousePassThroughSwitchKeyComboBox;

    QLabel *m_WidthLabel;
    QLabel *m_HeightLabel;
    QLabel *m_FontSizeLabel;
    QLabel *m_FontWeightLabel;
    QLabel *m_FontFamilyLabel;
    QLabel *m_RadiusLabel;
    QLabel *m_BorderWidthLabel;
    QLabel *m_NormalOpacityLabel;
    QLabel *m_PressedOpacityLabel;
    QLabel *m_LockedOpacityLabel;
    QLabel *m_HoverEffectStrengthLabel;
    QLabel *m_HoverGlowStrengthLabel;
    QLabel *m_HoverContrastModeLabel;
    QLabel *m_HoverAnimationDurationLabel;
    QLabel *m_StyleCodeLabel;

    QSpinBox *m_WidthSpinBox;
    QSpinBox *m_HeightSpinBox;
    QSpinBox *m_FontSizeSpinBox;
    QComboBox *m_FontWeightComboBox;
    QFontComboBox *m_FontFamilyComboBox;
    QPushButton *m_FontFamilyDefaultButton;
    QSpinBox *m_RadiusSpinBox;
    QSpinBox *m_BorderWidthSpinBox;
    QDoubleSpinBox *m_NormalOpacitySpinBox;
    QDoubleSpinBox *m_PressedOpacitySpinBox;
    QDoubleSpinBox *m_LockedOpacitySpinBox;
    QSpinBox *m_HoverEffectStrengthSpinBox;
    QSpinBox *m_HoverGlowStrengthSpinBox;
    QComboBox *m_HoverContrastModeComboBox;
    QSpinBox *m_HoverAnimationDurationSpinBox;
    QLineEdit *m_StyleCodeLineEdit;
    QPushButton *m_CopyStyleCodeButton;
    QPushButton *m_ApplyClipboardStyleCodeButton;

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
    ColorPickerWidget *m_BorderColorPicker;
    ColorPickerWidget *m_HoverCustomColorPicker;
};

#endif // QFLOATINGBUTTONSETUPDIALOG_H

