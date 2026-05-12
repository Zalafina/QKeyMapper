#include "qfloatingbuttonsetupdialog.h"

#include <QFormLayout>
#include <QFontComboBox>
#include <QGroupBox>
#include "qkeymapper.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

namespace {

QString sanitizeFloatingButtonMousePassThroughSwitchKey(const QString &switchKey)
{
    if (switchKey == FUNCTION_KEY_NONE || QKeyMapper_Worker::MultiKeyboardInputList.contains(switchKey)) {
        return switchKey;
    }

    return FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_DEFAULT;
}

QList<MAP_KEYDATA> *currentTabFullKeyMappingDataList()
{
    const int currentTabIndex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (currentTabIndex >= 0 && currentTabIndex < QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return QKeyMapper::s_KeyMappingTabInfoList.at(currentTabIndex).KeyMappingData;
    }

    return QKeyMapper::KeyMappingDataList;
}

}

QFloatingButtonSetupDialog::QFloatingButtonSetupDialog(QWidget *parent)
    : QDialog(parent)
    , m_ItemRow(-1)
    , m_isLoading(false)
    , m_hasBackup(false)
    , m_BackupMousePassThroughSwitchKey(FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_DEFAULT)
    , m_FontFamily()
    , m_InfoGroup(new QGroupBox(this))
    , m_ItemOriginalKeyLabel(new QLabel(this))
    , m_ItemOriginalKeyLineEdit(new QLineEdit(this))
    , m_ItemNoteLabel(new QLabel(this))
    , m_ItemNoteLineEdit(new QLineEdit(this))
    , m_ItemIndexLabel(new QLabel(this))
    , m_ItemIndexLineEdit(new QLineEdit(this))
    , m_EnableCheckBox(new QCheckBox(this))
    , m_LabelTextLabel(new QLabel(this))
    , m_LabelLineEdit(new QLineEdit(this))
    , m_ShowOnStartCheckBox(new QCheckBox(this))
    , m_ShowToolTipCheckBox(new QCheckBox(this))
    , m_SyncPressedLockedStateCheckBox(new QCheckBox(this))
    , m_AlwaysOnTopCheckBox(new QCheckBox(this))
    , m_MousePassThroughCheckBox(new QCheckBox(this))
    , m_DragToMoveCheckBox(new QCheckBox(this))
    , m_EnableGradientFillCheckBox(new QCheckBox(this))
    , m_EnableHoverAnimationCheckBox(new QCheckBox(this))
    , m_MousePassThroughSwitchKeyLabel(new QLabel(this))
    , m_MousePassThroughSwitchKeyComboBox(new QComboBox(this))
    , m_WidthLabel(new QLabel(this))
    , m_HeightLabel(new QLabel(this))
    , m_FontSizeLabel(new QLabel(this))
    , m_FontWeightLabel(new QLabel(this))
    , m_FontFamilyLabel(new QLabel(this))
    , m_RadiusLabel(new QLabel(this))
    , m_BorderWidthLabel(new QLabel(this))
    , m_NormalOpacityLabel(new QLabel(this))
    , m_PressedOpacityLabel(new QLabel(this))
    , m_LockedOpacityLabel(new QLabel(this))
    , m_HoverEffectStrengthLabel(new QLabel(this))
    , m_HoverGlowStrengthLabel(new QLabel(this))
    , m_HoverContrastModeLabel(new QLabel(this))
    , m_HoverAnimationDurationLabel(new QLabel(this))
    , m_StyleCodeLabel(new QLabel(this))
    , m_WidthSpinBox(new QSpinBox(this))
    , m_HeightSpinBox(new QSpinBox(this))
    , m_FontSizeSpinBox(new QSpinBox(this))
    , m_FontWeightComboBox(new QComboBox(this))
    , m_FontFamilyComboBox(new QFontComboBox(this))
    , m_FontFamilyDefaultButton(new QPushButton(this))
    , m_RadiusSpinBox(new QSpinBox(this))
    , m_BorderWidthSpinBox(new QSpinBox(this))
    , m_NormalOpacitySpinBox(new QDoubleSpinBox(this))
    , m_PressedOpacitySpinBox(new QDoubleSpinBox(this))
    , m_LockedOpacitySpinBox(new QDoubleSpinBox(this))
    , m_HoverEffectStrengthSpinBox(new QSpinBox(this))
    , m_HoverGlowStrengthSpinBox(new QSpinBox(this))
    , m_HoverContrastModeComboBox(new QComboBox(this))
    , m_HoverAnimationDurationSpinBox(new QSpinBox(this))
    , m_StyleCodeLineEdit(new QLineEdit(this))
    , m_CopyStyleCodeButton(new QPushButton(this))
    , m_ApplyClipboardStyleCodeButton(new QPushButton(this))
    , m_ReferencePointLabel(new QLabel(this))
    , m_OffsetXLabel(new QLabel(this))
    , m_OffsetYLabel(new QLabel(this))
    , m_ReferencePointComboBox(new QComboBox(this))
    , m_OffsetXSpinBox(new QSpinBox(this))
    , m_OffsetYSpinBox(new QSpinBox(this))
    , m_ApplyButton(Q_NULLPTR)
    , m_RevertButton(Q_NULLPTR)
    , m_ButtonColorPicker(new ColorPickerWidget(this, "FloatBtn_BtnColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR))
    , m_PressedColorPicker(new ColorPickerWidget(this, "FloatBtn_PressedColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR))
    , m_LockedColorPicker(new ColorPickerWidget(this, "FloatBtn_LockedColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR))
    , m_TextColorPicker(new ColorPickerWidget(this, "FloatBtn_TextColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_TEXTCOLOR))
    , m_BorderColorPicker(new ColorPickerWidget(this, "FloatBtn_BorderColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR))
    , m_HoverCustomColorPicker(new ColorPickerWidget(this, "FloatBtn_HoverCustomColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ButtonColorPicker->setShowAlphaChannel(true);
    m_PressedColorPicker->setShowAlphaChannel(true);
    m_LockedColorPicker->setShowAlphaChannel(true);
    m_TextColorPicker->setShowAlphaChannel(true);
    m_BorderColorPicker->setShowAlphaChannel(true);
    m_HoverCustomColorPicker->setShowAlphaChannel(true);

    for (QLineEdit *lineEdit : {m_ItemOriginalKeyLineEdit, m_ItemNoteLineEdit, m_ItemIndexLineEdit}) {
        lineEdit->setReadOnly(true);
    }
    m_ItemIndexLineEdit->setAlignment(Qt::AlignCenter);
    m_ItemIndexLineEdit->setMaximumWidth(96);

    m_WidthSpinBox->setRange(FLOATINGBUTTON_WIDTH_MIN, FLOATINGBUTTON_WIDTH_MAX);
    m_HeightSpinBox->setRange(FLOATINGBUTTON_HEIGHT_MIN, FLOATINGBUTTON_HEIGHT_MAX);
    m_FontSizeSpinBox->setRange(FLOATINGBUTTON_FONT_SIZE_MIN, FLOATINGBUTTON_FONT_SIZE_MAX);
    m_RadiusSpinBox->setRange(FLOATINGBUTTON_RADIUS_MIN, FLOATINGBUTTON_RADIUS_MAX);
    m_BorderWidthSpinBox->setRange(FLOATINGBUTTON_BORDER_WIDTH_MIN, FLOATINGBUTTON_BORDER_WIDTH_MAX);
    for (QDoubleSpinBox *opacitySpinBox : {m_NormalOpacitySpinBox, m_PressedOpacitySpinBox, m_LockedOpacitySpinBox}) {
        opacitySpinBox->setDecimals(FLOATINGBUTTON_OPACITY_DECIMALS);
        opacitySpinBox->setSingleStep(FLOATINGBUTTON_OPACITY_SINGLESTEP);
        opacitySpinBox->setRange(FLOATINGBUTTON_OPACITY_MIN, FLOATINGBUTTON_OPACITY_MAX);
    }
    m_HoverEffectStrengthSpinBox->setRange(FLOATINGBUTTON_HOVER_EFFECT_STRENGTH_MIN, FLOATINGBUTTON_HOVER_EFFECT_STRENGTH_MAX);
    m_HoverEffectStrengthSpinBox->setSuffix(QStringLiteral("%"));
    m_HoverGlowStrengthSpinBox->setRange(FLOATINGBUTTON_HOVER_GLOW_STRENGTH_MIN, FLOATINGBUTTON_HOVER_GLOW_STRENGTH_MAX);
    m_HoverGlowStrengthSpinBox->setSuffix(QStringLiteral("%"));
    m_HoverAnimationDurationSpinBox->setRange(FLOATINGBUTTON_HOVER_ANIMATION_DURATION_MIN, FLOATINGBUTTON_HOVER_ANIMATION_DURATION_MAX);
    m_HoverAnimationDurationSpinBox->setSuffix(QStringLiteral(" ms"));
    m_OffsetXSpinBox->setRange(FLOATINGBUTTON_OFFSET_MIN, FLOATINGBUTTON_OFFSET_MAX);
    m_OffsetYSpinBox->setRange(FLOATINGBUTTON_OFFSET_MIN, FLOATINGBUTTON_OFFSET_MAX);
    m_MousePassThroughSwitchKeyComboBox->addItem(tr(FUNCTION_KEY_NONE));
    m_MousePassThroughSwitchKeyComboBox->addItems(QKeyMapper_Worker::MultiKeyboardInputList);
    m_MousePassThroughSwitchKeyComboBox->setCurrentText(FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_DEFAULT);
    m_StyleCodeLineEdit->setReadOnly(true);
    m_StyleCodeLineEdit->setMinimumWidth(100);
    m_CopyStyleCodeButton->setAutoDefault(false);
    m_CopyStyleCodeButton->setDefault(false);
    m_ApplyClipboardStyleCodeButton->setAutoDefault(false);
    m_ApplyClipboardStyleCodeButton->setDefault(false);

    setupReferencePointComboBox();

    m_LabelLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_MousePassThroughSwitchKeyComboBox->setFocusPolicy(Qt::ClickFocus);
    m_ReferencePointComboBox->setFocusPolicy(Qt::ClickFocus);
    m_WidthSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_HeightSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_FontSizeSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_FontWeightComboBox->setFocusPolicy(Qt::ClickFocus);
    m_FontFamilyComboBox->setFocusPolicy(Qt::ClickFocus);
    m_RadiusSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_BorderWidthSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_NormalOpacitySpinBox->setFocusPolicy(Qt::ClickFocus);
    m_PressedOpacitySpinBox->setFocusPolicy(Qt::ClickFocus);
    m_LockedOpacitySpinBox->setFocusPolicy(Qt::ClickFocus);
    m_HoverEffectStrengthSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_HoverGlowStrengthSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_HoverContrastModeComboBox->setFocusPolicy(Qt::ClickFocus);
    m_HoverAnimationDurationSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_StyleCodeLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_OffsetXSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_OffsetYSpinBox->setFocusPolicy(Qt::ClickFocus);
    m_FontFamilyDefaultButton->setFocusPolicy(Qt::NoFocus);
    m_CopyStyleCodeButton->setFocusPolicy(Qt::NoFocus);
    m_ApplyClipboardStyleCodeButton->setFocusPolicy(Qt::NoFocus);

    QWidget *styleCodeWidget = new QWidget(this);
    QHBoxLayout *styleCodeLayout = new QHBoxLayout(styleCodeWidget);
    styleCodeLayout->setContentsMargins(0, 0, 0, 0);
    styleCodeLayout->setSpacing(6);
    styleCodeLayout->addWidget(m_StyleCodeLineEdit, 1);
    styleCodeLayout->addWidget(m_CopyStyleCodeButton);
    styleCodeLayout->addWidget(m_ApplyClipboardStyleCodeButton);
    styleCodeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QWidget *mousePassThroughWidget = new QWidget(this);
    QHBoxLayout *mousePassThroughLayout = new QHBoxLayout(mousePassThroughWidget);
    mousePassThroughLayout->setContentsMargins(0, 0, 0, 0);
    mousePassThroughLayout->setSpacing(8);
    mousePassThroughLayout->addWidget(m_MousePassThroughCheckBox);
    // mousePassThroughLayout->addStretch();
    mousePassThroughLayout->addWidget(m_MousePassThroughSwitchKeyLabel);
    mousePassThroughLayout->addWidget(m_MousePassThroughSwitchKeyComboBox);
    m_MousePassThroughCheckBox->setFixedWidth(140);
    m_MousePassThroughSwitchKeyComboBox->setMinimumWidth(130);
    m_MousePassThroughSwitchKeyLabel->setFixedWidth(120);

    QGridLayout *infoGrid = new QGridLayout(m_InfoGroup);
    infoGrid->setContentsMargins(8, 0, 8, 0);
    infoGrid->addWidget(m_ItemOriginalKeyLabel, 0, 0);
    infoGrid->addWidget(m_ItemOriginalKeyLineEdit, 0, 1, 1, 3);
    infoGrid->addWidget(m_ItemNoteLabel, 1, 0);
    infoGrid->addWidget(m_ItemNoteLineEdit, 1, 1);
    infoGrid->addWidget(m_ItemIndexLabel, 1, 2);
    infoGrid->addWidget(m_ItemIndexLineEdit, 1, 3);
    infoGrid->setColumnStretch(1, 1);
    m_ItemOriginalKeyLineEdit->setFocusPolicy(Qt::NoFocus);
    m_ItemNoteLineEdit->setFocusPolicy(Qt::NoFocus);
    m_ItemIndexLineEdit->setFocusPolicy(Qt::NoFocus);

    QGroupBox *basicGroup = new QGroupBox(this);
    QFormLayout *basicForm = new QFormLayout(basicGroup);
    basicForm->addRow(m_EnableCheckBox);
    basicForm->addRow(m_LabelTextLabel, m_LabelLineEdit);
    basicForm->addRow(m_ShowOnStartCheckBox);
    basicForm->addRow(m_ShowToolTipCheckBox);
    basicForm->addRow(m_SyncPressedLockedStateCheckBox);
    basicForm->addRow(m_AlwaysOnTopCheckBox);
    basicForm->addRow(mousePassThroughWidget);
    basicForm->addRow(m_DragToMoveCheckBox);

    QGroupBox *styleGroup = new QGroupBox(this);
    QGridLayout *styleGrid = new QGridLayout(styleGroup);
    styleGrid->addWidget(m_WidthLabel, 0, 0);
    styleGrid->addWidget(m_WidthSpinBox, 0, 1);
    styleGrid->addWidget(m_HeightLabel, 0, 2);
    styleGrid->addWidget(m_HeightSpinBox, 0, 3);
    styleGrid->addWidget(m_BorderWidthLabel, 0, 4);
    styleGrid->addWidget(m_BorderWidthSpinBox, 0, 5);

    styleGrid->addWidget(m_RadiusLabel, 1, 0);
    styleGrid->addWidget(m_RadiusSpinBox, 1, 1);

    styleGrid->addWidget(m_NormalOpacityLabel, 1, 2);
    styleGrid->addWidget(m_NormalOpacitySpinBox, 1, 3);
    styleGrid->addWidget(m_PressedOpacityLabel, 1, 4);
    styleGrid->addWidget(m_PressedOpacitySpinBox, 1, 5);

    styleGrid->addWidget(m_LockedOpacityLabel, 2, 0);
    styleGrid->addWidget(m_LockedOpacitySpinBox, 2, 1);
    styleGrid->addWidget(m_FontSizeLabel, 2, 2);
    styleGrid->addWidget(m_FontSizeSpinBox, 2, 3);
    styleGrid->addWidget(m_FontWeightLabel, 2, 4);
    styleGrid->addWidget(m_FontWeightComboBox, 2, 5);

    styleGrid->addWidget(m_FontFamilyLabel, 3, 0);
    styleGrid->addWidget(m_FontFamilyComboBox, 3, 1, 1, 4);
    styleGrid->addWidget(m_FontFamilyDefaultButton, 3, 5);

    styleGrid->addWidget(m_EnableGradientFillCheckBox, 4, 1, 1, 3);
    styleGrid->addWidget(m_EnableHoverAnimationCheckBox, 4, 4, 1, 3);

    styleGrid->addWidget(m_HoverEffectStrengthLabel, 5, 0);
    styleGrid->addWidget(m_HoverEffectStrengthSpinBox, 5, 1);
    styleGrid->addWidget(m_HoverGlowStrengthLabel, 5, 2);
    styleGrid->addWidget(m_HoverGlowStrengthSpinBox, 5, 3);
    styleGrid->addWidget(m_HoverAnimationDurationLabel, 5, 4);
    styleGrid->addWidget(m_HoverAnimationDurationSpinBox, 5, 5);

    styleGrid->addWidget(m_HoverContrastModeLabel, 6, 0);
    // Keep the combo in a single control column so it matches other controls' width
    styleGrid->addWidget(m_HoverContrastModeComboBox, 6, 1, 1, 1);
    // Place the custom color picker immediately to the right of the combo
    styleGrid->addWidget(m_HoverCustomColorPicker, 6, 2, 1, 3);

    QGridLayout *colorLayout = new QGridLayout();
    colorLayout->addWidget(m_ButtonColorPicker, 0, 0);
    colorLayout->addWidget(m_TextColorPicker, 0, 1);
    colorLayout->addWidget(m_PressedColorPicker, 1, 0);
    colorLayout->addWidget(m_LockedColorPicker, 1, 1);
    colorLayout->addWidget(m_BorderColorPicker, 2, 0, 1, 2);
    styleGrid->addLayout(colorLayout, 7, 1, 1, 5);

    // Diagnostic: print current column min widths and stretch factors
#ifdef DEBUG_LOGOUT_ON
    for (int _col = 0; _col < 6; ++_col) {
        qDebug() << "styleGrid BEFORE: col" << _col
                 << "minWidth=" << styleGrid->columnMinimumWidth(_col)
                 << "stretch=" << styleGrid->columnStretch(_col);
    }
#endif

    // Minimal fix: make control columns stretchable so columns with SpinBox/
    // ComboBox share remaining width evenly (cols 1,3,5). Keep label columns
    // non-stretch (0,2,4) so labels remain compact.
    styleGrid->setColumnStretch(0, 0);
    styleGrid->setColumnStretch(1, 1);
    styleGrid->setColumnStretch(2, 0);
    styleGrid->setColumnStretch(3, 1);
    styleGrid->setColumnStretch(4, 0);
    styleGrid->setColumnStretch(5, 1);
    styleGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Ensure controls that live in the "control" columns (1,3,5) expand
    // when those columns receive extra space. This makes SpinBox/ComboBox
    // width behavior consistent across rows.
    QWidget *expandWidgets[] = { m_WidthSpinBox, m_HeightSpinBox, m_BorderWidthSpinBox,
                                 m_RadiusSpinBox, m_NormalOpacitySpinBox, m_PressedOpacitySpinBox,
                                 m_LockedOpacitySpinBox, m_FontSizeSpinBox, m_FontWeightComboBox,
                                 m_FontFamilyComboBox, m_HoverEffectStrengthSpinBox, m_HoverGlowStrengthSpinBox,
                                 m_HoverAnimationDurationSpinBox };
    for (QWidget *w : expandWidgets) {
        if (w != Q_NULLPTR) {
            w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        }
    }

    // Put the Style Code row into a full-width container so its horizontal
    // layout is independent from the grid columns. This keeps the label
    // flush-left and the action buttons flush-right while the LineEdit
    // in the middle expands when the dialog is resized.
    QWidget *styleCodeFullRow = new QWidget(this);
    QHBoxLayout *styleCodeFullLayout = new QHBoxLayout(styleCodeFullRow);
    styleCodeFullLayout->setContentsMargins(0, 0, 0, 0);
    styleCodeFullLayout->setSpacing(6);
    styleCodeFullLayout->addWidget(m_StyleCodeLabel);
    styleCodeFullLayout->addWidget(styleCodeWidget, 1);
    styleCodeFullRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    styleGrid->addWidget(styleCodeFullRow, 8, 0, 1, 6);

    // Diagnostic: print grid state and size policies after adding the full-row
#ifdef DEBUG_LOGOUT_ON
    for (int _col = 0; _col < 6; ++_col) {
        qDebug() << "styleGrid AFTER: col" << _col
                 << "minWidth=" << styleGrid->columnMinimumWidth(_col)
                 << "stretch=" << styleGrid->columnStretch(_col);
    }
    qDebug() << "styleGroup sizePolicy=" << styleGroup->sizePolicy()
             << "styleCodeFullRow sizePolicy=" << styleCodeFullRow->sizePolicy()
             << "styleCodeWidget sizePolicy=" << styleCodeWidget->sizePolicy()
             << "m_StyleCodeLineEdit minW=" << m_StyleCodeLineEdit->minimumWidth()
             << "lineEdit sizePolicy=" << m_StyleCodeLineEdit->sizePolicy();
#endif

    QGroupBox *positionGroup = new QGroupBox(this);
    QGridLayout *positionGrid = new QGridLayout(positionGroup);
    positionGrid->addWidget(m_ReferencePointLabel, 0, 0);
    positionGrid->addWidget(m_ReferencePointComboBox, 0, 1, 1, 3);
    positionGrid->addWidget(m_OffsetXLabel, 1, 0);
    positionGrid->addWidget(m_OffsetXSpinBox, 1, 1);
    positionGrid->addWidget(m_OffsetYLabel, 1, 2);
    positionGrid->addWidget(m_OffsetYSpinBox, 1, 3);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    m_ApplyButton = buttonBox->addButton(QString(), QDialogButtonBox::ApplyRole);
    m_RevertButton = buttonBox->addButton(QString(), QDialogButtonBox::ResetRole);
    m_ApplyButton->setFixedSize(75, 28);
    m_RevertButton->setFixedSize(75, 28);
    m_ApplyButton->setDefault(false);
    m_ApplyButton->setAutoDefault(false);
    m_RevertButton->setDefault(false);
    m_RevertButton->setAutoDefault(false);
    m_ApplyButton->setFocusPolicy(Qt::NoFocus);
    m_RevertButton->setFocusPolicy(Qt::NoFocus);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_InfoGroup);
    mainLayout->addWidget(basicGroup);
    mainLayout->addWidget(styleGroup);
    mainLayout->addWidget(positionGroup);
    mainLayout->addWidget(buttonBox);

    m_ItemOriginalKeyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ItemNoteLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ItemIndexLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_LabelTextLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_WidthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_HeightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_FontSizeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_FontWeightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_FontFamilyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_RadiusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_BorderWidthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_NormalOpacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_PressedOpacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_LockedOpacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_HoverEffectStrengthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_HoverGlowStrengthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_HoverContrastModeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_HoverAnimationDurationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_StyleCodeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ReferencePointLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_OffsetXLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_OffsetYLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_MousePassThroughSwitchKeyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_FontFamilyDefaultButton->setFixedWidth(75);
    m_FontFamilyDefaultButton->setAutoDefault(false);
    m_FontFamilyDefaultButton->setDefault(false);

    m_ButtonColorPicker->raise();
    m_PressedColorPicker->raise();
    m_LockedColorPicker->raise();
    m_TextColorPicker->raise();
    m_BorderColorPicker->raise();
    m_HoverCustomColorPicker->raise();

    connect(m_ApplyButton, &QPushButton::clicked, this, &QFloatingButtonSetupDialog::onApplyButtonClicked);
    connect(m_RevertButton, &QPushButton::clicked, this, &QFloatingButtonSetupDialog::onRevertButtonClicked);
    connect(m_CopyStyleCodeButton, &QPushButton::clicked, this, &QFloatingButtonSetupDialog::copyStyleCodeToClipboard);
    connect(m_ApplyClipboardStyleCodeButton, &QPushButton::clicked, this, &QFloatingButtonSetupDialog::applyClipboardStyleCode);

    #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(m_EnableCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    #else
    connect(m_EnableCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    #endif
    connect(m_LabelLineEdit, &QLineEdit::textChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(m_ShowOnStartCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_ShowToolTipCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_SyncPressedLockedStateCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_AlwaysOnTopCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_MousePassThroughCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_DragToMoveCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_EnableGradientFillCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_EnableHoverAnimationCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    #else
    connect(m_ShowOnStartCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_ShowToolTipCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_SyncPressedLockedStateCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_AlwaysOnTopCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_MousePassThroughCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_DragToMoveCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_EnableGradientFillCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_EnableHoverAnimationCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    #endif
    connect(m_MousePassThroughSwitchKeyComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_WidthSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_HeightSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_FontSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_FontWeightComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_FontFamilyComboBox, &QFontComboBox::currentFontChanged, this,
            [this](const QFont &font) {
                m_FontFamily = font.family().trimmed();
                m_FontFamilyDefaultButton->setEnabled(!m_FontFamily.isEmpty());
                onAnyControlChanged();
            });
    connect(m_FontFamilyDefaultButton, &QPushButton::clicked, this,
            [this]() {
                m_FontFamily.clear();
                syncFontFamilyControls();
                onAnyControlChanged();
            });
    connect(m_RadiusSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_BorderWidthSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_NormalOpacitySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_PressedOpacitySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_LockedOpacitySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_HoverEffectStrengthSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_HoverGlowStrengthSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_HoverAnimationDurationSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_HoverContrastModeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) {
            updateHoverCustomizationState();
            onAnyControlChanged();
            });
    connect(m_ReferencePointComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_OffsetXSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_OffsetYSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);

    m_ButtonColorPicker->setLivePreviewEnabled(true);
    m_PressedColorPicker->setLivePreviewEnabled(true);
    m_LockedColorPicker->setLivePreviewEnabled(true);
    m_TextColorPicker->setLivePreviewEnabled(true);
    m_BorderColorPicker->setLivePreviewEnabled(true);
    m_HoverCustomColorPicker->setLivePreviewEnabled(true);

    connect(m_ButtonColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_PressedColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_LockedColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_TextColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_BorderColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_HoverCustomColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);

    connect(m_ButtonColorPicker, &ColorPickerWidget::previewColorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_PressedColorPicker, &ColorPickerWidget::previewColorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_LockedColorPicker, &ColorPickerWidget::previewColorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_TextColorPicker, &ColorPickerWidget::previewColorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_BorderColorPicker, &ColorPickerWidget::previewColorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_HoverCustomColorPicker, &ColorPickerWidget::previewColorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);

    syncFontFamilyControls();

    setUILanguage(QKeyMapper::getLanguageIndex());
    updateHoverCustomizationState();
    adjustSize();
    setMinimumWidth(qMax(minimumSizeHint().width(), sizeHint().width()));
}

QFloatingButtonSetupDialog::~QFloatingButtonSetupDialog() = default;

void QFloatingButtonSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);

    setWindowTitle(tr("Floating Button Setup"));
    // m_InfoGroup->setTitle(tr("Button Info"));
    m_ItemOriginalKeyLabel->setText(tr("OriginalKey"));
    m_ItemNoteLabel->setText(tr("Note"));
    m_ItemIndexLabel->setText(QObject::tr("No."));
    m_EnableCheckBox->setText(tr("Enable Floating Button"));
    m_LabelTextLabel->setText(tr("Label"));
    m_ShowOnStartCheckBox->setText(tr("Show on Mapping Start"));
    m_ShowToolTipCheckBox->setText(tr("Show Tooltip"));
    m_SyncPressedLockedStateCheckBox->setText(tr("Sync Pressed State"));
    m_AlwaysOnTopCheckBox->setText(tr("Always On Top"));
    m_MousePassThroughCheckBox->setText(tr("MousePassThrough"));
    m_MousePassThroughSwitchKeyLabel->setText(tr("MouseSwitchKey"));
    if (m_MousePassThroughSwitchKeyComboBox->count() > FUNCTION_KEY_NONE_INDEX) {
        m_MousePassThroughSwitchKeyComboBox->setItemText(FUNCTION_KEY_NONE_INDEX, tr("None"));
    }
    m_DragToMoveCheckBox->setText(tr("Enable Drag to Move"));
    m_DragToMoveCheckBox->setToolTip(tr("Supports Ctrl+drag. Context menu Move is always available."));
    m_EnableGradientFillCheckBox->setText(tr("Enable Gradient Fill"));
    m_EnableHoverAnimationCheckBox->setText(tr("Enable Hover Animation"));
    m_HoverEffectStrengthLabel->setText(tr("Hover Effect"));
    m_HoverGlowStrengthLabel->setText(tr("Hover Glow"));
    m_HoverContrastModeLabel->setText(tr("Hover Contrast"));
    m_HoverAnimationDurationLabel->setText(tr("Hover Duration"));
    m_HoverAnimationDurationSpinBox->setSuffix(tr(" ms"));
    m_StyleCodeLabel->setText(tr("Style Code"));

    m_WidthLabel->setText(tr("Width"));
    m_HeightLabel->setText(tr("Height"));
    m_FontSizeLabel->setText(tr("Font Size"));
    m_FontWeightLabel->setText(tr("Font Weight"));
    m_FontFamilyLabel->setText(tr("Font Family"));
    m_RadiusLabel->setText(tr("Radius"));
    m_BorderWidthLabel->setText(tr("Border Width"));
    m_NormalOpacityLabel->setText(tr("Normal Opacity"));
    m_PressedOpacityLabel->setText(tr("Pressed Opacity"));
    m_LockedOpacityLabel->setText(tr("Locked Opacity"));

    m_ReferencePointLabel->setText(tr("Ref Point"));
    m_OffsetXLabel->setText(tr("Offset X"));
    m_OffsetYLabel->setText(tr("Offset Y"));

    if (m_ApplyButton != Q_NULLPTR) {
        m_ApplyButton->setText(tr("Apply"));
    }
    if (m_RevertButton != Q_NULLPTR) {
        m_RevertButton->setText(tr("Revert"));
    }
    if (m_FontFamilyDefaultButton != Q_NULLPTR) {
        m_FontFamilyDefaultButton->setText(tr("Default"));
        m_FontFamilyDefaultButton->setToolTip(tr("Use application default font"));
    }
    if (m_CopyStyleCodeButton != Q_NULLPTR) {
        m_CopyStyleCodeButton->setText(tr("Copy Style Code"));
    }
    if (m_ApplyClipboardStyleCodeButton != Q_NULLPTR) {
        m_ApplyClipboardStyleCodeButton->setText(tr("Apply Clipboard Style Code"));
    }

    if (m_FontWeightComboBox->count() != 3) {
        m_FontWeightComboBox->clear();
        m_FontWeightComboBox->addItem(tr("Light"));
        m_FontWeightComboBox->addItem(tr("Normal"));
        m_FontWeightComboBox->addItem(tr("Bold"));
    }
    else {
        m_FontWeightComboBox->setItemText(0, tr("Light"));
        m_FontWeightComboBox->setItemText(1, tr("Normal"));
        m_FontWeightComboBox->setItemText(2, tr("Bold"));
    }

    if (m_HoverContrastModeComboBox->count() != 4) {
        m_HoverContrastModeComboBox->clear();
        m_HoverContrastModeComboBox->addItem(tr("Auto"));
        m_HoverContrastModeComboBox->addItem(tr("Lighten"));
        m_HoverContrastModeComboBox->addItem(tr("Darken"));
        m_HoverContrastModeComboBox->addItem(tr("Custom"));
    }
    else {
        m_HoverContrastModeComboBox->setItemText(FLOATINGBUTTON_HOVER_CONTRASTMODE_AUTO, tr("Auto"));
        m_HoverContrastModeComboBox->setItemText(FLOATINGBUTTON_HOVER_CONTRASTMODE_LIGHTEN, tr("Lighten"));
        m_HoverContrastModeComboBox->setItemText(FLOATINGBUTTON_HOVER_CONTRASTMODE_DARKEN, tr("Darken"));
        m_HoverContrastModeComboBox->setItemText(FLOATINGBUTTON_HOVER_CONTRASTMODE_CUSTOM, tr("Custom"));
    }

    m_LabelLineEdit->setPlaceholderText(tr("Empty = use original key name"));

    m_ButtonColorPicker->setButtonText(tr("BtnColor"));
    m_ButtonColorPicker->setWindowTitle(tr("Floating Button Color"));
    m_PressedColorPicker->setButtonText(tr("PressedColor"));
    m_PressedColorPicker->setWindowTitle(tr("Floating Button Pressed Color"));
    m_LockedColorPicker->setButtonText(tr("LockedColor"));
    m_LockedColorPicker->setWindowTitle(tr("Floating Button Locked Color"));
    m_TextColorPicker->setButtonText(tr("TextColor"));
    m_TextColorPicker->setWindowTitle(tr("Floating Button Text Color"));
    m_BorderColorPicker->setButtonText(tr("BorderColor"));
    m_BorderColorPicker->setWindowTitle(tr("Floating Button Border Color"));
    m_HoverCustomColorPicker->setButtonText(tr("HoverColor"));
    m_HoverCustomColorPicker->setWindowTitle(tr("Floating Button Hover Color"));

    setupReferencePointComboBox();
    updateHoverCustomizationState();
}

void QFloatingButtonSetupDialog::setItemRow(int row)
{
    m_ItemRow = row;
}

int QFloatingButtonSetupDialog::getItemRow() const
{
    return m_ItemRow;
}

void QFloatingButtonSetupDialog::refreshFromCurrentItem()
{
    loadFromCurrentItem();
}

bool QFloatingButtonSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow() && !QKeyMapper::isSelectColorDialogVisible()) {
            close();
        }
    }

    return QDialog::event(event);
}

void QFloatingButtonSetupDialog::showEvent(QShowEvent *event)
{
    QList<MAP_KEYDATA> *mappingDataList = currentTabFullKeyMappingDataList();
    loadFromCurrentItem();

    if (mappingDataList != Q_NULLPTR
        && m_ItemRow >= 0 && m_ItemRow < mappingDataList->size()) {
        m_BackupData = mappingDataList->at(m_ItemRow);
        m_hasBackup = true;
    }
    else {
        m_hasBackup = false;
    }

    const int currentTabIndex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (currentTabIndex >= 0 && currentTabIndex < QKeyMapper::s_KeyMappingTabInfoList.size()) {
        m_BackupMousePassThroughSwitchKey = sanitizeFloatingButtonMousePassThroughSwitchKey(
            QKeyMapper::s_KeyMappingTabInfoList.at(currentTabIndex).FloatingWindow_MousePassThroughSwitchKey);
    }
    else {
        m_BackupMousePassThroughSwitchKey = FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_DEFAULT;
    }

    QDialog::showEvent(event);
}

void QFloatingButtonSetupDialog::closeEvent(QCloseEvent *event)
{
    m_hasBackup = false;
    QDialog::closeEvent(event);
}

void QFloatingButtonSetupDialog::onApplyButtonClicked()
{
    QList<MAP_KEYDATA> *mappingDataList = currentTabFullKeyMappingDataList();
    applyToCurrentItem();

    if (mappingDataList != Q_NULLPTR
        && m_ItemRow >= 0 && m_ItemRow < mappingDataList->size()) {
        m_BackupData = mappingDataList->at(m_ItemRow);
        m_hasBackup = true;
    }

    emit settingsApplied();
}

void QFloatingButtonSetupDialog::onRevertButtonClicked()
{
    QList<MAP_KEYDATA> *mappingDataList = currentTabFullKeyMappingDataList();
    if (!m_hasBackup) {
        return;
    }

    if (mappingDataList == Q_NULLPTR
        || m_ItemRow < 0 || m_ItemRow >= mappingDataList->size()) {
        return;
    }

    (*mappingDataList)[m_ItemRow] = m_BackupData;

    const int currentTabIndex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (currentTabIndex >= 0 && currentTabIndex < QKeyMapper::s_KeyMappingTabInfoList.size()) {
        QKeyMapper::s_KeyMappingTabInfoList[currentTabIndex].FloatingWindow_MousePassThroughSwitchKey = m_BackupMousePassThroughSwitchKey;
    }

    loadFromCurrentItem();
    emit settingsApplied();
}

void QFloatingButtonSetupDialog::onAnyControlChanged()
{
    if (m_isLoading) {
        return;
    }

    applyToCurrentItem();
    emit settingsApplied();
}

void QFloatingButtonSetupDialog::loadFromCurrentItem()
{
    QList<MAP_KEYDATA> *mappingDataList = currentTabFullKeyMappingDataList();
    if (mappingDataList == Q_NULLPTR
        || m_ItemRow < 0 || m_ItemRow >= mappingDataList->size()) {
        return;
    }

    m_isLoading = true;

    const MAP_KEYDATA &keymapdata = mappingDataList->at(m_ItemRow);

    m_ItemOriginalKeyLineEdit->setText(keymapdata.Original_Key);
    m_ItemOriginalKeyLineEdit->setToolTip(keymapdata.Original_Key);
    m_ItemNoteLineEdit->setText(keymapdata.Note);
    m_ItemNoteLineEdit->setToolTip(keymapdata.Note);
    m_ItemIndexLineEdit->setText(QString::number(m_ItemRow + 1));

    m_EnableCheckBox->setChecked(keymapdata.FloatingButton_Enable);
    m_LabelLineEdit->setText(keymapdata.FloatingButton_Label);
    m_ShowOnStartCheckBox->setChecked(keymapdata.FloatingButton_ShowOnMappingStart);
    m_ShowToolTipCheckBox->setChecked(keymapdata.FloatingButton_ShowToolTip);
    m_SyncPressedLockedStateCheckBox->setChecked(keymapdata.FloatingButton_SyncPressedLockedState);
    m_AlwaysOnTopCheckBox->setChecked(keymapdata.FloatingButton_AlwaysOnTop);
    m_MousePassThroughCheckBox->setChecked(keymapdata.FloatingButton_MousePassThrough);
    m_DragToMoveCheckBox->setChecked(keymapdata.FloatingButton_DragToMove);
    m_EnableGradientFillCheckBox->setChecked(keymapdata.FloatingButton_EnableGradientFill);
    m_EnableHoverAnimationCheckBox->setChecked(keymapdata.FloatingButton_EnableHoverAnimation);
    m_HoverEffectStrengthSpinBox->setValue(qBound(FLOATINGBUTTON_HOVER_EFFECT_STRENGTH_MIN,
                                                  keymapdata.FloatingButton_HoverEffectStrength,
                                                  FLOATINGBUTTON_HOVER_EFFECT_STRENGTH_MAX));
    m_HoverGlowStrengthSpinBox->setValue(qBound(FLOATINGBUTTON_HOVER_GLOW_STRENGTH_MIN,
                                                keymapdata.FloatingButton_HoverGlowStrength,
                                                FLOATINGBUTTON_HOVER_GLOW_STRENGTH_MAX));
    m_HoverContrastModeComboBox->setCurrentIndex(qBound(FLOATINGBUTTON_HOVER_CONTRASTMODE_MIN,
                                                        keymapdata.FloatingButton_HoverContrastMode,
                                                        FLOATINGBUTTON_HOVER_CONTRASTMODE_MAX));
    m_HoverAnimationDurationSpinBox->setValue(qBound(FLOATINGBUTTON_HOVER_ANIMATION_DURATION_MIN,
                                                     keymapdata.FloatingButton_HoverAnimationDuration,
                                                     FLOATINGBUTTON_HOVER_ANIMATION_DURATION_MAX));

    const int currentTabIndex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    const QString switchKey = (currentTabIndex >= 0 && currentTabIndex < QKeyMapper::s_KeyMappingTabInfoList.size())
        ? sanitizeFloatingButtonMousePassThroughSwitchKey(QKeyMapper::s_KeyMappingTabInfoList.at(currentTabIndex).FloatingWindow_MousePassThroughSwitchKey)
        : QString(FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_DEFAULT);
    if (switchKey == FUNCTION_KEY_NONE) {
        m_MousePassThroughSwitchKeyComboBox->setCurrentIndex(FUNCTION_KEY_NONE_INDEX);
    }
    else if (QKeyMapper_Worker::MultiKeyboardInputList.contains(switchKey)) {
        m_MousePassThroughSwitchKeyComboBox->setCurrentText(switchKey);
    }
    else {
        m_MousePassThroughSwitchKeyComboBox->setCurrentText(FLOATINGWINDOW_MOUSE_PASSTHROUGH_SWITCHKEY_DEFAULT);
    }

    m_WidthSpinBox->setValue(keymapdata.FloatingButton_Width);
    m_HeightSpinBox->setValue(keymapdata.FloatingButton_Height);
    m_FontSizeSpinBox->setValue(keymapdata.FloatingButton_FontSize);
    m_FontWeightComboBox->setCurrentIndex(qBound(FLOATINGBUTTON_FONT_WEIGHT_MIN, keymapdata.FloatingButton_FontWeight, FLOATINGBUTTON_FONT_WEIGHT_MAX));
    m_FontFamily = keymapdata.FloatingButton_FontFamily.trimmed();
    syncFontFamilyControls();
    m_RadiusSpinBox->setValue(keymapdata.FloatingButton_Radius);
    m_BorderWidthSpinBox->setValue(keymapdata.FloatingButton_BorderWidth);
    m_NormalOpacitySpinBox->setValue((FLOATINGBUTTON_OPACITY_MIN <= keymapdata.FloatingButton_NormalOpacity && keymapdata.FloatingButton_NormalOpacity <= FLOATINGBUTTON_OPACITY_MAX)
                                     ? keymapdata.FloatingButton_NormalOpacity
                                     : keymapdata.FloatingButton_Opacity);
    m_PressedOpacitySpinBox->setValue((FLOATINGBUTTON_OPACITY_MIN <= keymapdata.FloatingButton_PressedOpacity && keymapdata.FloatingButton_PressedOpacity <= FLOATINGBUTTON_OPACITY_MAX)
                                      ? keymapdata.FloatingButton_PressedOpacity
                                      : keymapdata.FloatingButton_Opacity);
    m_LockedOpacitySpinBox->setValue((FLOATINGBUTTON_OPACITY_MIN <= keymapdata.FloatingButton_LockedOpacity && keymapdata.FloatingButton_LockedOpacity <= FLOATINGBUTTON_OPACITY_MAX)
                                     ? keymapdata.FloatingButton_LockedOpacity
                                     : keymapdata.FloatingButton_Opacity);

    int referencePoint = keymapdata.FloatingButton_ReferencePoint;
    if (referencePoint < FLOATINGWINDOW_REFERENCEPOINT_MIN || referencePoint > FLOATINGWINDOW_REFERENCEPOINT_MAX) {
        referencePoint = FLOATINGWINDOW_REFERENCEPOINT_DEFAULT;
    }
    m_ReferencePointComboBox->setCurrentIndex(referencePoint);
    m_OffsetXSpinBox->setValue(keymapdata.FloatingButton_X_Offset);
    m_OffsetYSpinBox->setValue(keymapdata.FloatingButton_Y_Offset);

    m_ButtonColorPicker->setColor(keymapdata.FloatingButton_ButtonColor);
    m_PressedColorPicker->setColor(keymapdata.FloatingButton_PressedColor);
    m_LockedColorPicker->setColor(keymapdata.FloatingButton_LockedColor);
    m_TextColorPicker->setColor(keymapdata.FloatingButton_TextColor);
    m_BorderColorPicker->setColor(keymapdata.FloatingButton_BorderColor);
    m_HoverCustomColorPicker->setColor(keymapdata.FloatingButton_HoverCustomColor.isValid()
                                           ? keymapdata.FloatingButton_HoverCustomColor
                                           : (keymapdata.FloatingButton_ButtonColor.isValid()
                                                  ? keymapdata.FloatingButton_ButtonColor
                                                  : FLOATINGBUTTON_BUTTON_COLOR_DEFAULT_QCOLOR));

    updateHoverCustomizationState();
    updateStyleCodeDisplay();

    m_isLoading = false;
}

void QFloatingButtonSetupDialog::applyToCurrentItem()
{
    QList<MAP_KEYDATA> *mappingDataList = currentTabFullKeyMappingDataList();
    if (mappingDataList == Q_NULLPTR
        || m_ItemRow < 0 || m_ItemRow >= mappingDataList->size()) {
        return;
    }

    MAP_KEYDATA &keymapdata = (*mappingDataList)[m_ItemRow];

    keymapdata.FloatingButton_Enable = m_EnableCheckBox->isChecked();
    keymapdata.FloatingButton_Label = m_LabelLineEdit->text();
    keymapdata.FloatingButton_ShowOnMappingStart = m_ShowOnStartCheckBox->isChecked();
    keymapdata.FloatingButton_ShowToolTip = m_ShowToolTipCheckBox->isChecked();
    keymapdata.FloatingButton_SyncPressedLockedState = m_SyncPressedLockedStateCheckBox->isChecked();
    keymapdata.FloatingButton_AlwaysOnTop = m_AlwaysOnTopCheckBox->isChecked();
    keymapdata.FloatingButton_MousePassThrough = m_MousePassThroughCheckBox->isChecked();
    keymapdata.FloatingButton_DragToMove = m_DragToMoveCheckBox->isChecked();
    keymapdata.FloatingButton_EnableGradientFill = m_EnableGradientFillCheckBox->isChecked();
    keymapdata.FloatingButton_EnableHoverAnimation = m_EnableHoverAnimationCheckBox->isChecked();
    keymapdata.FloatingButton_HoverEffectStrength = m_HoverEffectStrengthSpinBox->value();
    keymapdata.FloatingButton_HoverGlowStrength = m_HoverGlowStrengthSpinBox->value();
    keymapdata.FloatingButton_HoverContrastMode = qBound(FLOATINGBUTTON_HOVER_CONTRASTMODE_MIN,
                                                         m_HoverContrastModeComboBox->currentIndex(),
                                                         FLOATINGBUTTON_HOVER_CONTRASTMODE_MAX);
    keymapdata.FloatingButton_HoverAnimationDuration = m_HoverAnimationDurationSpinBox->value();
    if (keymapdata.FloatingButton_HoverContrastMode == FLOATINGBUTTON_HOVER_CONTRASTMODE_CUSTOM) {
        keymapdata.FloatingButton_HoverCustomColor = m_HoverCustomColorPicker->getColor();
    }

    const int currentTabIndex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    if (currentTabIndex >= 0 && currentTabIndex < QKeyMapper::s_KeyMappingTabInfoList.size()) {
        if (m_MousePassThroughSwitchKeyComboBox->currentIndex() == FUNCTION_KEY_NONE_INDEX) {
            QKeyMapper::s_KeyMappingTabInfoList[currentTabIndex].FloatingWindow_MousePassThroughSwitchKey = FUNCTION_KEY_NONE;
        }
        else {
            QKeyMapper::s_KeyMappingTabInfoList[currentTabIndex].FloatingWindow_MousePassThroughSwitchKey =
                sanitizeFloatingButtonMousePassThroughSwitchKey(m_MousePassThroughSwitchKeyComboBox->currentText());
        }
    }

    keymapdata.FloatingButton_Width = m_WidthSpinBox->value();
    keymapdata.FloatingButton_Height = m_HeightSpinBox->value();
    keymapdata.FloatingButton_FontSize = m_FontSizeSpinBox->value();
    keymapdata.FloatingButton_FontWeight = qBound(FLOATINGBUTTON_FONT_WEIGHT_MIN, m_FontWeightComboBox->currentIndex(), FLOATINGBUTTON_FONT_WEIGHT_MAX);
    keymapdata.FloatingButton_FontFamily = m_FontFamily;
    keymapdata.FloatingButton_Radius = m_RadiusSpinBox->value();
    keymapdata.FloatingButton_BorderWidth = m_BorderWidthSpinBox->value();
    keymapdata.FloatingButton_NormalOpacity = m_NormalOpacitySpinBox->value();
    keymapdata.FloatingButton_PressedOpacity = m_PressedOpacitySpinBox->value();
    keymapdata.FloatingButton_LockedOpacity = m_LockedOpacitySpinBox->value();
    keymapdata.FloatingButton_Opacity = keymapdata.FloatingButton_NormalOpacity;

    keymapdata.FloatingButton_ReferencePoint = m_ReferencePointComboBox->currentIndex();
    keymapdata.FloatingButton_X_Offset = m_OffsetXSpinBox->value();
    keymapdata.FloatingButton_Y_Offset = m_OffsetYSpinBox->value();

    keymapdata.FloatingButton_ButtonColor = m_ButtonColorPicker->getColor();
    keymapdata.FloatingButton_PressedColor = m_PressedColorPicker->getColor();
    keymapdata.FloatingButton_LockedColor = m_LockedColorPicker->getColor();
    keymapdata.FloatingButton_TextColor = m_TextColorPicker->getColor();
    keymapdata.FloatingButton_BorderColor = m_BorderColorPicker->getColor();

    updateStyleCodeDisplay();
}

void QFloatingButtonSetupDialog::updateStyleCodeDisplay()
{
    if (m_StyleCodeLineEdit == Q_NULLPTR) {
        return;
    }

    QList<MAP_KEYDATA> *mappingDataList = currentTabFullKeyMappingDataList();
    if (mappingDataList == Q_NULLPTR
        || m_ItemRow < 0
        || m_ItemRow >= mappingDataList->size()) {
        m_StyleCodeLineEdit->clear();
        m_StyleCodeLineEdit->setToolTip(QString());
        return;
    }

    const QString styleCode = QKeyMapper::generateFloatingButtonStyleCode(mappingDataList->at(m_ItemRow));
    m_StyleCodeLineEdit->setText(styleCode);
    m_StyleCodeLineEdit->setCursorPosition(0);
    m_StyleCodeLineEdit->setToolTip(tr("Generated automatically from Style Group settings"));
}

void QFloatingButtonSetupDialog::copyStyleCodeToClipboard()
{
    updateStyleCodeDisplay();

    const QString styleCode = m_StyleCodeLineEdit->text().trimmed();
    QKeyMapper *keyMapper = QKeyMapper::getInstance();
    if (styleCode.isEmpty()) {
        if (keyMapper != Q_NULLPTR) {
            keyMapper->showFailurePopup(tr("Failed to generate floating button style code."));
        }
        return;
    }

    QKeyMapper::copyStringToClipboard(styleCode);
    if (keyMapper != Q_NULLPTR) {
        keyMapper->showInformationPopup(tr("Floating button style code copied to clipboard."));
    }
}

void QFloatingButtonSetupDialog::applyClipboardStyleCode()
{
    QList<MAP_KEYDATA> *mappingDataList = currentTabFullKeyMappingDataList();
    if (mappingDataList == Q_NULLPTR
        || m_ItemRow < 0
        || m_ItemRow >= mappingDataList->size()) {
        return;
    }

    QString clipboardText;
    QKeyMapper *keyMapper = QKeyMapper::getInstance();
    if (!QKeyMapper::readClipboardText(clipboardText) || clipboardText.trimmed().isEmpty()) {
        if (keyMapper != Q_NULLPTR) {
            keyMapper->showFailurePopup(tr("Clipboard does not contain a floating button style code."));
        }
        return;
    }

    MAP_KEYDATA updatedData = mappingDataList->at(m_ItemRow);
    const QKeyMapper::FloatingButtonStyleCodeApplyResult result = QKeyMapper::applyFloatingButtonStyleCode(clipboardText, updatedData);
    if (!result.success) {
        if (keyMapper != Q_NULLPTR) {
            keyMapper->showFailurePopup(result.errorMessage);
        }
        return;
    }

    (*mappingDataList)[m_ItemRow] = updatedData;
    loadFromCurrentItem();
    emit settingsApplied();

    if (keyMapper != Q_NULLPTR) {
        keyMapper->showInformationPopup(QKeyMapper::floatingButtonStyleCodeApplySuccessMessage(result));
    }
}

void QFloatingButtonSetupDialog::syncFontFamilyControls()
{
    const QString previewFamily = QKeyMapper::resolveConfiguredFontFamily(m_FontFamily);

    const QSignalBlocker blocker(m_FontFamilyComboBox);
    if (!previewFamily.isEmpty()) {
        m_FontFamilyComboBox->setCurrentFont(QFont(previewFamily));
    }

    m_FontFamilyDefaultButton->setEnabled(!m_FontFamily.isEmpty());
}

void QFloatingButtonSetupDialog::updateHoverCustomizationState()
{
    const int hoverContrastMode = qBound(FLOATINGBUTTON_HOVER_CONTRASTMODE_MIN,
                                         m_HoverContrastModeComboBox->currentIndex(),
                                         FLOATINGBUTTON_HOVER_CONTRASTMODE_MAX);
    const bool customEnabled = (hoverContrastMode == FLOATINGBUTTON_HOVER_CONTRASTMODE_CUSTOM);

    if (customEnabled && !m_HoverCustomColorPicker->getColor().isValid()) {
        QColor resolvedColor = m_ButtonColorPicker->getColor();
        if (!resolvedColor.isValid()) {
            resolvedColor = FLOATINGBUTTON_BUTTON_COLOR_DEFAULT_QCOLOR;
        }
        m_HoverCustomColorPicker->setColor(resolvedColor);
    }

    m_HoverCustomColorPicker->setEnabled(customEnabled);
}

void QFloatingButtonSetupDialog::setupReferencePointComboBox()
{
    m_ReferencePointComboBox->clear();
    m_ReferencePointComboBox->addItem(tr("ScreenTopLeft"));
    m_ReferencePointComboBox->addItem(tr("ScreenTopRight"));
    m_ReferencePointComboBox->addItem(tr("ScreenTopCenter"));
    m_ReferencePointComboBox->addItem(tr("ScreenBottomLeft"));
    m_ReferencePointComboBox->addItem(tr("ScreenBottomRight"));
    m_ReferencePointComboBox->addItem(tr("ScreenBottomCenter"));
    m_ReferencePointComboBox->addItem(tr("WindowTopLeft"));
    m_ReferencePointComboBox->addItem(tr("WindowTopRight"));
    m_ReferencePointComboBox->addItem(tr("WindowTopCenter"));
    m_ReferencePointComboBox->addItem(tr("WindowBottomLeft"));
    m_ReferencePointComboBox->addItem(tr("WindowBottomRight"));
    m_ReferencePointComboBox->addItem(tr("WindowBottomCenter"));
}

