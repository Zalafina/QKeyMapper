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
    , m_BGColorPicker(new ColorPickerWidget(this, "VBtn_BGColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BGCOLOR))
    , m_BtnColorPicker(new ColorPickerWidget(this, "VBtn_BtnColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR))
    , m_PressedColorPicker(new ColorPickerWidget(this, "VBtn_PressedColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR))
    , m_LockedColorPicker(new ColorPickerWidget(this, "VBtn_LockedColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_BTNCOLOR))
    , m_TextColorPicker(new ColorPickerWidget(this, "VBtn_TextColor", COLORPICKER_BUTTON_WIDTH_VBTNPANEL_TEXTCOLOR))
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_instance = this;

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->colorGroupBox->setStyle(windowsStyle);
    }

    // Position color pickers inside the color group box.
    // raise() is required because the pickers are created in the member initializer list
    // (before setupUi), so colorGroupBox has a higher z-order and would otherwise eat
    // all mouse events over that area.
    m_BtnColorPicker->move(40, 30);
    m_BtnColorPicker->setColor(VBTNPANEL_BUTTON_COLOR_DEFAULT);
    m_BtnColorPicker->raise();

    m_BGColorPicker->setShowAlphaChannel(true);
    m_BGColorPicker->move(185, 30);
    m_BGColorPicker->setColor(VBTNPANEL_BACKGROUND_COLOR_DEFAULT);
    m_BGColorPicker->raise();

    m_PressedColorPicker->move(40, 65);
    m_PressedColorPicker->setColor(VBTNPANEL_PRESSED_COLOR_DEFAULT);
    m_PressedColorPicker->raise();

    m_LockedColorPicker->move(185, 65);
    m_LockedColorPicker->setColor(VBTNPANEL_LOCKED_COLOR_DEFAULT);
    m_LockedColorPicker->raise();

    m_TextColorPicker->move(40, 100);
    m_TextColorPicker->setColor(VBTNPANEL_TEXT_COLOR_DEFAULT);
    m_TextColorPicker->raise();
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
            });
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
    ui->cancelButton->setText(tr("Cancel"));
    ui->btnFontFamilyDefaultButton->setText(tr("Default"));
    ui->btnFontFamilyDefaultButton->setToolTip(tr("Use application default font"));

    m_BtnColorPicker->setButtonText(tr("BtnColor"));
    m_BtnColorPicker->setWindowTitle(tr("VButton Panel Button Color"));
    m_BGColorPicker->setButtonText(tr("BGColor"));
    m_BGColorPicker->setWindowTitle(tr("VButton Panel BG Color"));
    m_PressedColorPicker->setButtonText(tr("PressedColor"));
    m_PressedColorPicker->setWindowTitle(tr("VButton Panel Pressed Color"));
    m_LockedColorPicker->setButtonText(tr("LockedColor"));
    m_LockedColorPicker->setWindowTitle(tr("VButton Panel Locked Color"));
    m_TextColorPicker->setButtonText(tr("TextColor"));
    m_TextColorPicker->setWindowTitle(tr("VButton Panel Text Color"));
}

void QVButtonPanelSetupDialog::loadSettings(const VButtonPanelSettings &settings)
{
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
    m_BGColorPicker->setColor(settings.bgColor);
    m_BtnColorPicker->setColor(settings.btnColor);
    m_PressedColorPicker->setColor(settings.pressedColor);
    m_LockedColorPicker->setColor(settings.lockedColor);
    m_TextColorPicker->setColor(settings.textColor);
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
    s.bgColor        = m_BGColorPicker->getColor();
    s.btnColor       = m_BtnColorPicker->getColor();
    s.pressedColor   = m_PressedColorPicker->getColor();
    s.lockedColor    = m_LockedColorPicker->getColor();
    s.textColor      = m_TextColorPicker->getColor();
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

void QVButtonPanelSetupDialog::on_okButton_clicked()
{
    // Emit signal to apply current settings without closing the dialog,
    // allowing the user to continue adjusting while the panel updates live.
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
    QDialog::closeEvent(event);

    if (event->isAccepted()) {
        emit setupDialogClosed();
    }
}
