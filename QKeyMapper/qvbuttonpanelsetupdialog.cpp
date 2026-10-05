#include "qkeymapper.h"
#include "qvbuttonpanelsetupdialog.h"
#include "ui_qvbuttonpanelsetupdialog.h"
#include "qkeymapper_constants.h"
#include "colorpickerwidget.h"
#include "qstyle_singletons.h"

using namespace QKeyMapperConstants;

QVButtonPanelSetupDialog *QVButtonPanelSetupDialog::m_instance = nullptr;

QVButtonPanelSetupDialog::QVButtonPanelSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QVButtonPanelSetupDialog)
    , m_isLoading(false)
    , m_hasBackup(false)
    , m_BackupSettings()
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_instance = this;

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->colorGroupBox->setStyle(windowsStyle);
        ui->panelSettingsGroupBox->setStyle(windowsStyle);
        ui->fontGroupBox->setStyle(windowsStyle);
        ui->positionGroupBox->setStyle(windowsStyle);
    }

    if (QStyle *fusionStyle = QKeyMapperStyle::fusionStyle()) {
        const auto childWidgets = findChildren<QWidget*>();
        for (QWidget *w : childWidgets) {
            if (w != ui->colorGroupBox && w != ui->panelSettingsGroupBox
                && w != ui->fontGroupBox && w != ui->positionGroupBox) {
                w->setStyle(fusionStyle);
            }
        }
    }

    ui->btnColorPicker->setColorType("VBtn_BtnColor");
    ui->btnColorPicker->setButtonWidth(COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR);
    ui->btnColorPicker->setColor(VBTNPANEL_BUTTON_COLOR_DEFAULT);

    ui->bgColorPicker->setColorType("VBtn_BGColor");
    ui->bgColorPicker->setButtonWidth(COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BGCOLOR);
    ui->bgColorPicker->setShowAlphaChannel(true);
    ui->bgColorPicker->setColor(VBTNPANEL_BACKGROUND_COLOR_DEFAULT);

    ui->pressedColorPicker->setColorType("VBtn_PressedColor");
    ui->pressedColorPicker->setButtonWidth(COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR);
    ui->pressedColorPicker->setColor(VBTNPANEL_PRESSED_COLOR_DEFAULT);

    ui->lockedColorPicker->setColorType("VBtn_LockedColor");
    ui->lockedColorPicker->setButtonWidth(COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR);
    ui->lockedColorPicker->setColor(VBTNPANEL_LOCKED_COLOR_DEFAULT);

    ui->textColorPicker->setColorType("VBtn_TextColor");
    ui->textColorPicker->setButtonWidth(COLORPICKER_BUTTON_WIDTH_VBTNPANEL_TEXTCOLOR);
    ui->textColorPicker->setColor(VBTNPANEL_TEXT_COLOR_DEFAULT);
    // Populate reference point combo box — index must match FLOATINGWINDOW_REFERENCEPOINT_* values exactly
    QStringList referencePointList;
    referencePointList.append(tr("ScreenTopLeft"));      // 0  FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPLEFT
    referencePointList.append(tr("ScreenTopRight"));     // 1  FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPRIGHT
    referencePointList.append(tr("ScreenTopCenter"));    // 2  FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPCENTER
    referencePointList.append(tr("ScreenBottomLeft"));   // 3  FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMLEFT
    referencePointList.append(tr("ScreenBottomRight"));  // 4  FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMRIGHT
    referencePointList.append(tr("ScreenBottomCenter")); // 5  FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMCENTER
    referencePointList.append(tr("WindowTopLeft"));      // 6  FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPLEFT
    referencePointList.append(tr("WindowTopRight"));     // 7  FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPRIGHT
    referencePointList.append(tr("WindowTopCenter"));    // 8  FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPCENTER
    referencePointList.append(tr("WindowBottomLeft"));   // 9  FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMLEFT
    referencePointList.append(tr("WindowBottomRight"));  // 10 FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMRIGHT
    referencePointList.append(tr("WindowBottomCenter")); // 11 FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMCENTER
    ui->referencePointComboBox->addItems(referencePointList);

    QStringList fontWeightList;
    fontWeightList.append(tr("Light"));
    fontWeightList.append(tr("Normal"));
    fontWeightList.append(tr("Bold"));
    ui->btnFontWeightComboBox->addItems(fontWeightList);
    ui->btnFontWeightComboBox->setCurrentIndex(VBTNPANEL_DEFAULT_FONT_WEIGHT);
    ui->btnFontSizeSpinBox->setRange(VBTNPANEL_BTNFONTSIZE_MIN, VBTNPANEL_BTNFONTSIZE_MAX);
    ui->btnFontSizeSpinBox->setValue(VBTNPANEL_DEFAULT_BTNFONTSIZE);

    connect(ui->btnFontFamilyComboBox, &QFontComboBox::currentFontChanged, this,
            [this](const QFont &font) {
                m_btnFontFamily = font.family();
                ui->btnFontFamilyDefaultButton->setEnabled(!m_btnFontFamily.isEmpty());
            });
    connect(ui->btnFontFamilyDefaultButton, &QPushButton::clicked, this,
            [this]() {
                m_btnFontFamily.clear();
                syncFontFamilyControls();
                onAnyControlChanged();
            });

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(ui->alwaysOnTopCheckBox, &QCheckBox::checkStateChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->dragEnabledCheckBox, &QCheckBox::checkStateChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->defaultShowCheckBox, &QCheckBox::checkStateChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
#else
    connect(ui->alwaysOnTopCheckBox, &QCheckBox::stateChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->dragEnabledCheckBox, &QCheckBox::stateChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->defaultShowCheckBox, &QCheckBox::stateChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
#endif
    connect(ui->columnsSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->maxRowsSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->btnWidthSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->btnHeightSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->opacitySpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->marginSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->radiusSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->referencePointComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->offsetXSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->offsetYSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->btnFontSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->btnFontWeightComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->btnFontFamilyComboBox, &QFontComboBox::currentFontChanged, this,
            [this](const QFont &font) {
                m_btnFontFamily = font.family().trimmed();
                ui->btnFontFamilyDefaultButton->setEnabled(!m_btnFontFamily.isEmpty());
                onAnyControlChanged();
            });

    ui->bgColorPicker->setLivePreviewEnabled(true);
    ui->btnColorPicker->setLivePreviewEnabled(true);
    ui->pressedColorPicker->setLivePreviewEnabled(true);
    ui->lockedColorPicker->setLivePreviewEnabled(true);
    ui->textColorPicker->setLivePreviewEnabled(true);

    connect(ui->bgColorPicker, &ColorPickerWidget::colorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->btnColorPicker, &ColorPickerWidget::colorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->pressedColorPicker, &ColorPickerWidget::colorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->lockedColorPicker, &ColorPickerWidget::colorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->textColorPicker, &ColorPickerWidget::colorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);

    connect(ui->bgColorPicker, &ColorPickerWidget::previewColorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->btnColorPicker, &ColorPickerWidget::previewColorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->pressedColorPicker, &ColorPickerWidget::previewColorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->lockedColorPicker, &ColorPickerWidget::previewColorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);
    connect(ui->textColorPicker, &ColorPickerWidget::previewColorChanged, this, &QVButtonPanelSetupDialog::onAnyControlChanged);

    syncFontFamilyControls();
}

QVButtonPanelSetupDialog::~QVButtonPanelSetupDialog()
{
    m_instance = nullptr;
    delete ui;
}

void QVButtonPanelSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("VButton Panel Setup"));

    ui->colorGroupBox->setTitle(tr("Color"));
    ui->panelSettingsGroupBox->setTitle(tr("Panel"));
    ui->fontGroupBox->setTitle(tr("Font"));
    ui->positionGroupBox->setTitle(tr("Position"));
    ui->columnsLabel->setText(tr("Columns"));
    ui->maxRowsLabel->setText(tr("Max Rows"));
    ui->btnWidthLabel->setText(tr("Btn Width"));
    ui->btnHeightLabel->setText(tr("Btn Height"));
    ui->opacityLabel->setText(tr("Opacity"));
    ui->marginLabel->setText(tr("Margin"));
    ui->radiusLabel->setText(tr("Radius"));
    ui->offsetXLabel->setText(tr("Offset X"));
    ui->offsetYLabel->setText(tr("Offset Y"));
    ui->referencePointLabel->setText(tr("Ref Point"));
    ui->btnFontSizeLabel->setText(tr("Font Size"));
    ui->btnFontWeightLabel->setText(tr("Font Weight"));
    ui->btnFontFamilyLabel->setText(tr("Font Family"));

    ui->alwaysOnTopCheckBox->setText(tr("Always On Top"));
    ui->dragEnabledCheckBox->setText(tr("Enable Drag to Move"));
    ui->dragEnabledCheckBox->setToolTip(tr("Supports Ctrl+drag. Context menu Move is always available."));
    ui->defaultShowCheckBox->setText(tr("Show on Mapping Start"));

    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPLEFT,       tr("ScreenTopLeft"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPRIGHT,      tr("ScreenTopRight"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_SCREENTOPCENTER,     tr("ScreenTopCenter"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMLEFT,    tr("ScreenBottomLeft"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMRIGHT,   tr("ScreenBottomRight"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_SCREENBOTTOMCENTER,  tr("ScreenBottomCenter"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPLEFT,       tr("WindowTopLeft"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPRIGHT,      tr("WindowTopRight"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_WINDOWTOPCENTER,     tr("WindowTopCenter"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMLEFT,    tr("WindowBottomLeft"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMRIGHT,   tr("WindowBottomRight"));
    ui->referencePointComboBox->setItemText(FLOATINGWINDOW_REFERENCEPOINT_WINDOWBOTTOMCENTER,  tr("WindowBottomCenter"));
    ui->btnFontWeightComboBox->setItemText(VBTNPANEL_FONT_WEIGHT_LIGHT, tr("Light"));
    ui->btnFontWeightComboBox->setItemText(VBTNPANEL_FONT_WEIGHT_NORMAL, tr("Normal"));
    ui->btnFontWeightComboBox->setItemText(VBTNPANEL_FONT_WEIGHT_BOLD, tr("Bold"));

    ui->okButton->setText(tr("Apply"));
    ui->revertButton->setText(tr("Revert"));
    ui->btnFontFamilyDefaultButton->setText(tr("Default"));
    ui->btnFontFamilyDefaultButton->setToolTip(tr("Use application default font"));

    ui->btnColorPicker->setButtonText(tr("BtnColor"));
    ui->btnColorPicker->setWindowTitle(tr("VButton Panel Button Color"));
    ui->bgColorPicker->setButtonText(tr("BGColor"));
    ui->bgColorPicker->setWindowTitle(tr("VButton Panel BG Color"));
    ui->pressedColorPicker->setButtonText(tr("PressedColor"));
    ui->pressedColorPicker->setWindowTitle(tr("VButton Panel Pressed Color"));
    ui->lockedColorPicker->setButtonText(tr("LockedColor"));
    ui->lockedColorPicker->setWindowTitle(tr("VButton Panel Locked Color"));
    ui->textColorPicker->setButtonText(tr("TextColor"));
    ui->textColorPicker->setWindowTitle(tr("VButton Panel Text Color"));
}

void QVButtonPanelSetupDialog::loadSettings(const VButtonPanelSettings &settings)
{
    m_isLoading = true;

    ui->columnsSpinBox->setValue(settings.columns);
    ui->maxRowsSpinBox->setValue(settings.maxRows);
    ui->btnWidthSpinBox->setValue(settings.btnWidth);
    ui->btnHeightSpinBox->setValue(settings.btnHeight);
    ui->opacitySpinBox->setValue(settings.opacity);
    ui->alwaysOnTopCheckBox->setChecked(settings.alwaysOnTop);
    ui->defaultShowCheckBox->setChecked(settings.defaultShow);
    ui->marginSpinBox->setValue(settings.margin);
    ui->radiusSpinBox->setValue(settings.radius);
    ui->dragEnabledCheckBox->setChecked(settings.dragEnabled);
    int idx = settings.referencePoint;
    if (idx < 0 || idx >= ui->referencePointComboBox->count())
        idx = 0;
    ui->referencePointComboBox->setCurrentIndex(idx);
    ui->offsetXSpinBox->setValue(settings.offsetX);
    ui->offsetYSpinBox->setValue(settings.offsetY);
    ui->btnFontSizeSpinBox->setValue(qBound(VBTNPANEL_BTNFONTSIZE_MIN, settings.btnFontSize, VBTNPANEL_BTNFONTSIZE_MAX));
    ui->btnFontWeightComboBox->setCurrentIndex(qBound(VBTNPANEL_FONT_WEIGHT_MIN, settings.btnFontWeight, VBTNPANEL_FONT_WEIGHT_MAX));
    m_btnFontFamily = settings.btnFontFamily.trimmed();
    syncFontFamilyControls();
    ui->bgColorPicker->setColor(settings.bgColor);
    ui->btnColorPicker->setColor(settings.btnColor);
    ui->pressedColorPicker->setColor(settings.pressedColor);
    ui->lockedColorPicker->setColor(settings.lockedColor);
    ui->textColorPicker->setColor(settings.textColor);

    m_isLoading = false;
}

VButtonPanelSettings QVButtonPanelSetupDialog::getSettings() const
{
    VButtonPanelSettings s;
    s.columns        = ui->columnsSpinBox->value();
    s.maxRows        = ui->maxRowsSpinBox->value();
    s.btnWidth       = ui->btnWidthSpinBox->value();
    s.btnHeight      = ui->btnHeightSpinBox->value();
    s.opacity        = ui->opacitySpinBox->value();
    s.alwaysOnTop    = ui->alwaysOnTopCheckBox->isChecked();
    s.defaultShow    = ui->defaultShowCheckBox->isChecked();
    s.margin         = ui->marginSpinBox->value();
    s.radius         = ui->radiusSpinBox->value();
    s.dragEnabled    = ui->dragEnabledCheckBox->isChecked();
    s.referencePoint = ui->referencePointComboBox->currentIndex();
    s.offsetX        = ui->offsetXSpinBox->value();
    s.offsetY        = ui->offsetYSpinBox->value();
    s.btnFontSize    = ui->btnFontSizeSpinBox->value();
    s.btnFontWeight  = ui->btnFontWeightComboBox->currentIndex();
    s.btnFontFamily  = m_btnFontFamily;
    s.bgColor        = ui->bgColorPicker->getColor();
    s.btnColor       = ui->btnColorPicker->getColor();
    s.pressedColor   = ui->pressedColorPicker->getColor();
    s.lockedColor    = ui->lockedColorPicker->getColor();
    s.textColor      = ui->textColorPicker->getColor();
    return s;
}

void QVButtonPanelSetupDialog::syncFontFamilyControls()
{
    const QString previewFamily = QKeyMapper::resolveConfiguredFontFamily(m_btnFontFamily);

    const QSignalBlocker blocker(ui->btnFontFamilyComboBox);
    if (!previewFamily.isEmpty()) {
        ui->btnFontFamilyComboBox->setCurrentFont(QFont(previewFamily));
    }

    ui->btnFontFamilyDefaultButton->setEnabled(!m_btnFontFamily.isEmpty());
}

void QVButtonPanelSetupDialog::showEvent(QShowEvent *event)
{
    m_BackupSettings = getSettings();
    m_hasBackup = true;

    QDialog::showEvent(event);
}

void QVButtonPanelSetupDialog::on_okButton_clicked()
{
    m_BackupSettings = getSettings();
    m_hasBackup = true;

    emit settingsApplied();
}

void QVButtonPanelSetupDialog::on_revertButton_clicked()
{
    if (!m_hasBackup) {
        return;
    }

    loadSettings(m_BackupSettings);
    emit settingsApplied();
}

void QVButtonPanelSetupDialog::onAnyControlChanged()
{
    if (m_isLoading) {
        return;
    }

    emit settingsApplied();
}

bool QVButtonPanelSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            if (QKeyMapper::isSelectColorDialogVisible()) {
            }
            else {
                close();
            }
        }
    }
    return QDialog::event(event);
}

void QVButtonPanelSetupDialog::closeEvent(QCloseEvent *event)
{
    m_hasBackup = false;
    QDialog::closeEvent(event);

    if (event->isAccepted()) {
        emit setupDialogClosed();
    }
}
