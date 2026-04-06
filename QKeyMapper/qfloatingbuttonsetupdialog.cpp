#include "qfloatingbuttonsetupdialog.h"

#include <QFormLayout>
#include <QFontComboBox>
#include <QGroupBox>

#include "qkeymapper.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QFloatingButtonSetupDialog::QFloatingButtonSetupDialog(QWidget *parent)
    : QDialog(parent)
    , m_ItemRow(-1)
    , m_isLoading(false)
    , m_hasBackup(false)
    , m_FontFamily()
    , m_EnableCheckBox(new QCheckBox(this))
    , m_LabelTextLabel(new QLabel(this))
    , m_LabelLineEdit(new QLineEdit(this))
    , m_ShowOnStartCheckBox(new QCheckBox(this))
    , m_ShowToolTipCheckBox(new QCheckBox(this))
    , m_SyncPressedLockedStateCheckBox(new QCheckBox(this))
    , m_AlwaysOnTopCheckBox(new QCheckBox(this))
    , m_DragToMoveCheckBox(new QCheckBox(this))
    , m_WidthLabel(new QLabel(this))
    , m_HeightLabel(new QLabel(this))
    , m_FontSizeLabel(new QLabel(this))
    , m_FontWeightLabel(new QLabel(this))
    , m_FontFamilyLabel(new QLabel(this))
    , m_RadiusLabel(new QLabel(this))
    , m_NormalOpacityLabel(new QLabel(this))
    , m_PressedOpacityLabel(new QLabel(this))
    , m_LockedOpacityLabel(new QLabel(this))
    , m_WidthSpinBox(new QSpinBox(this))
    , m_HeightSpinBox(new QSpinBox(this))
    , m_FontSizeSpinBox(new QSpinBox(this))
    , m_FontWeightComboBox(new QComboBox(this))
    , m_FontFamilyComboBox(new QFontComboBox(this))
    , m_FontFamilyDefaultButton(new QPushButton(this))
    , m_RadiusSpinBox(new QSpinBox(this))
    , m_NormalOpacitySpinBox(new QDoubleSpinBox(this))
    , m_PressedOpacitySpinBox(new QDoubleSpinBox(this))
    , m_LockedOpacitySpinBox(new QDoubleSpinBox(this))
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
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ButtonColorPicker->setShowAlphaChannel(true);
    m_PressedColorPicker->setShowAlphaChannel(true);
    m_LockedColorPicker->setShowAlphaChannel(true);
    m_TextColorPicker->setShowAlphaChannel(true);

    m_WidthSpinBox->setRange(FLOATINGBUTTON_WIDTH_MIN, FLOATINGBUTTON_WIDTH_MAX);
    m_HeightSpinBox->setRange(FLOATINGBUTTON_HEIGHT_MIN, FLOATINGBUTTON_HEIGHT_MAX);
    m_FontSizeSpinBox->setRange(FLOATINGBUTTON_FONT_SIZE_MIN, FLOATINGBUTTON_FONT_SIZE_MAX);
    m_RadiusSpinBox->setRange(FLOATINGBUTTON_RADIUS_MIN, FLOATINGBUTTON_RADIUS_MAX);
    for (QDoubleSpinBox *opacitySpinBox : {m_NormalOpacitySpinBox, m_PressedOpacitySpinBox, m_LockedOpacitySpinBox}) {
        opacitySpinBox->setDecimals(FLOATINGBUTTON_OPACITY_DECIMALS);
        opacitySpinBox->setSingleStep(FLOATINGBUTTON_OPACITY_SINGLESTEP);
        opacitySpinBox->setRange(FLOATINGBUTTON_OPACITY_MIN, FLOATINGBUTTON_OPACITY_MAX);
    }
    m_OffsetXSpinBox->setRange(FLOATINGBUTTON_OFFSET_MIN, FLOATINGBUTTON_OFFSET_MAX);
    m_OffsetYSpinBox->setRange(FLOATINGBUTTON_OFFSET_MIN, FLOATINGBUTTON_OFFSET_MAX);

    setupReferencePointComboBox();

    QGroupBox *basicGroup = new QGroupBox(this);
    QFormLayout *basicForm = new QFormLayout(basicGroup);
    basicForm->addRow(m_EnableCheckBox);
    basicForm->addRow(m_LabelTextLabel, m_LabelLineEdit);
    basicForm->addRow(m_ShowOnStartCheckBox);
    basicForm->addRow(m_ShowToolTipCheckBox);
    basicForm->addRow(m_SyncPressedLockedStateCheckBox);
    basicForm->addRow(m_AlwaysOnTopCheckBox);
    basicForm->addRow(m_DragToMoveCheckBox);

    QGroupBox *styleGroup = new QGroupBox(this);
    QGridLayout *styleGrid = new QGridLayout(styleGroup);
    styleGrid->addWidget(m_WidthLabel, 0, 0);
    styleGrid->addWidget(m_WidthSpinBox, 0, 1);
    styleGrid->addWidget(m_HeightLabel, 0, 2);
    styleGrid->addWidget(m_HeightSpinBox, 0, 3);

    styleGrid->addWidget(m_RadiusLabel, 1, 0);
    styleGrid->addWidget(m_RadiusSpinBox, 1, 1);

    styleGrid->addWidget(m_NormalOpacityLabel, 1, 2);
    styleGrid->addWidget(m_NormalOpacitySpinBox, 1, 3);

    styleGrid->addWidget(m_PressedOpacityLabel, 2, 0);
    styleGrid->addWidget(m_PressedOpacitySpinBox, 2, 1);
    styleGrid->addWidget(m_LockedOpacityLabel, 2, 2);
    styleGrid->addWidget(m_LockedOpacitySpinBox, 2, 3);

    styleGrid->addWidget(m_FontSizeLabel, 3, 0);
    styleGrid->addWidget(m_FontSizeSpinBox, 3, 1);
    styleGrid->addWidget(m_FontWeightLabel, 3, 2);
    styleGrid->addWidget(m_FontWeightComboBox, 3, 3);

    styleGrid->addWidget(m_FontFamilyLabel, 4, 0);
    styleGrid->addWidget(m_FontFamilyComboBox, 4, 1, 1, 2);
    styleGrid->addWidget(m_FontFamilyDefaultButton, 4, 3);

    QGridLayout *colorLayout = new QGridLayout();
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->addWidget(m_ButtonColorPicker, 0, 0);
    colorLayout->addWidget(m_TextColorPicker, 0, 1);
    colorLayout->addWidget(m_PressedColorPicker, 1, 0);
    colorLayout->addWidget(m_LockedColorPicker, 1, 1);
    styleGrid->addLayout(colorLayout, 5, 0, 1, 4);

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

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(basicGroup);
    mainLayout->addWidget(styleGroup);
    mainLayout->addWidget(positionGroup);
    mainLayout->addWidget(buttonBox);

    m_LabelTextLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_WidthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_HeightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_FontSizeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_FontWeightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_FontFamilyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_RadiusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_NormalOpacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_PressedOpacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_LockedOpacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ReferencePointLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_OffsetXLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_OffsetYLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_FontFamilyDefaultButton->setFixedWidth(75);
    m_FontFamilyDefaultButton->setAutoDefault(false);
    m_FontFamilyDefaultButton->setDefault(false);

    m_ButtonColorPicker->raise();
    m_PressedColorPicker->raise();
    m_LockedColorPicker->raise();
    m_TextColorPicker->raise();

    connect(m_ApplyButton, &QPushButton::clicked, this, &QFloatingButtonSetupDialog::onApplyButtonClicked);
    connect(m_RevertButton, &QPushButton::clicked, this, &QFloatingButtonSetupDialog::onRevertButtonClicked);

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
    connect(m_DragToMoveCheckBox, &QCheckBox::checkStateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    #else
    connect(m_ShowOnStartCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_ShowToolTipCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_SyncPressedLockedStateCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_AlwaysOnTopCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_DragToMoveCheckBox, &QCheckBox::stateChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    #endif
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
    connect(m_NormalOpacitySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_PressedOpacitySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_LockedOpacitySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_ReferencePointComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_OffsetXSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_OffsetYSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QFloatingButtonSetupDialog::onAnyControlChanged);

    connect(m_ButtonColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_PressedColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_LockedColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);
    connect(m_TextColorPicker, &ColorPickerWidget::colorChanged, this, &QFloatingButtonSetupDialog::onAnyControlChanged);

    syncFontFamilyControls();

    setUILanguage(QKeyMapper::getLanguageIndex());
}

QFloatingButtonSetupDialog::~QFloatingButtonSetupDialog() = default;

void QFloatingButtonSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);

    setWindowTitle(tr("Floating Button Setup"));
    m_EnableCheckBox->setText(tr("Enable Floating Button"));
    m_LabelTextLabel->setText(tr("Label"));
    m_ShowOnStartCheckBox->setText(tr("Show on Mapping Start"));
    m_ShowToolTipCheckBox->setText(tr("Show Tooltip"));
    m_SyncPressedLockedStateCheckBox->setText(tr("Sync Pressed State"));
    m_AlwaysOnTopCheckBox->setText(tr("Always On Top"));
    m_DragToMoveCheckBox->setText(tr("Enable Drag to Move"));
    m_DragToMoveCheckBox->setToolTip(tr("Supports Ctrl+drag and context menu Move."));

    m_WidthLabel->setText(tr("Width"));
    m_HeightLabel->setText(tr("Height"));
    m_FontSizeLabel->setText(tr("Font Size"));
    m_FontWeightLabel->setText(tr("Font Weight"));
    m_FontFamilyLabel->setText(tr("Font Family"));
    m_RadiusLabel->setText(tr("Radius"));
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

    m_LabelLineEdit->setPlaceholderText(tr("Empty = use original key name"));

    m_ButtonColorPicker->setButtonText(tr("BtnColor"));
    m_ButtonColorPicker->setWindowTitle(tr("Floating Button Color"));
    m_PressedColorPicker->setButtonText(tr("PressedColor"));
    m_PressedColorPicker->setWindowTitle(tr("Floating Button Pressed Color"));
    m_LockedColorPicker->setButtonText(tr("LockedColor"));
    m_LockedColorPicker->setWindowTitle(tr("Floating Button Locked Color"));
    m_TextColorPicker->setButtonText(tr("TextColor"));
    m_TextColorPicker->setWindowTitle(tr("Floating Button Text Color"));

    setupReferencePointComboBox();
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
    loadFromCurrentItem();

    if (m_ItemRow >= 0 && m_ItemRow < QKeyMapper::KeyMappingDataList->size()) {
        m_BackupData = QKeyMapper::KeyMappingDataList->at(m_ItemRow);
        m_hasBackup = true;
    }
    else {
        m_hasBackup = false;
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
    applyToCurrentItem();

    if (m_ItemRow >= 0 && m_ItemRow < QKeyMapper::KeyMappingDataList->size()) {
        m_BackupData = QKeyMapper::KeyMappingDataList->at(m_ItemRow);
        m_hasBackup = true;
    }

    emit settingsApplied();
}

void QFloatingButtonSetupDialog::onRevertButtonClicked()
{
    if (!m_hasBackup) {
        return;
    }

    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    (*QKeyMapper::KeyMappingDataList)[m_ItemRow] = m_BackupData;
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
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    m_isLoading = true;

    const MAP_KEYDATA &keymapdata = QKeyMapper::KeyMappingDataList->at(m_ItemRow);

    m_EnableCheckBox->setChecked(keymapdata.FloatingButton_Enable);
    m_LabelLineEdit->setText(keymapdata.FloatingButton_Label);
    m_ShowOnStartCheckBox->setChecked(keymapdata.FloatingButton_ShowOnMappingStart);
    m_ShowToolTipCheckBox->setChecked(keymapdata.FloatingButton_ShowToolTip);
    m_SyncPressedLockedStateCheckBox->setChecked(keymapdata.FloatingButton_SyncPressedLockedState);
    m_AlwaysOnTopCheckBox->setChecked(keymapdata.FloatingButton_AlwaysOnTop);
    m_DragToMoveCheckBox->setChecked(keymapdata.FloatingButton_DragToMove);

    m_WidthSpinBox->setValue(keymapdata.FloatingButton_Width);
    m_HeightSpinBox->setValue(keymapdata.FloatingButton_Height);
    m_FontSizeSpinBox->setValue(keymapdata.FloatingButton_FontSize);
    m_FontWeightComboBox->setCurrentIndex(qBound(FLOATINGBUTTON_FONT_WEIGHT_MIN, keymapdata.FloatingButton_FontWeight, FLOATINGBUTTON_FONT_WEIGHT_MAX));
    m_FontFamily = keymapdata.FloatingButton_FontFamily.trimmed();
    syncFontFamilyControls();
    m_RadiusSpinBox->setValue(keymapdata.FloatingButton_Radius);
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

    m_isLoading = false;
}

void QFloatingButtonSetupDialog::applyToCurrentItem()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    MAP_KEYDATA &keymapdata = (*QKeyMapper::KeyMappingDataList)[m_ItemRow];

    keymapdata.FloatingButton_Enable = m_EnableCheckBox->isChecked();
    keymapdata.FloatingButton_Label = m_LabelLineEdit->text();
    keymapdata.FloatingButton_ShowOnMappingStart = m_ShowOnStartCheckBox->isChecked();
    keymapdata.FloatingButton_ShowToolTip = m_ShowToolTipCheckBox->isChecked();
    keymapdata.FloatingButton_SyncPressedLockedState = m_SyncPressedLockedStateCheckBox->isChecked();
    keymapdata.FloatingButton_AlwaysOnTop = m_AlwaysOnTopCheckBox->isChecked();
    keymapdata.FloatingButton_DragToMove = m_DragToMoveCheckBox->isChecked();

    keymapdata.FloatingButton_Width = m_WidthSpinBox->value();
    keymapdata.FloatingButton_Height = m_HeightSpinBox->value();
    keymapdata.FloatingButton_FontSize = m_FontSizeSpinBox->value();
    keymapdata.FloatingButton_FontWeight = qBound(FLOATINGBUTTON_FONT_WEIGHT_MIN, m_FontWeightComboBox->currentIndex(), FLOATINGBUTTON_FONT_WEIGHT_MAX);
    keymapdata.FloatingButton_FontFamily = m_FontFamily;
    keymapdata.FloatingButton_Radius = m_RadiusSpinBox->value();
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

