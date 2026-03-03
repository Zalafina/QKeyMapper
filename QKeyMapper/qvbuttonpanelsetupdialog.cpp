#include "qvbuttonpanelsetupdialog.h"
#include "ui_qvbuttonpanelsetupdialog.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QVButtonPanelSetupDialog *QVButtonPanelSetupDialog::m_instance = nullptr;

QVButtonPanelSetupDialog::QVButtonPanelSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QVButtonPanelSetupDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_instance = this;

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

    ui->alwaysOnTopCheckBox->setText(tr("Always On Top"));
    ui->dragEnabledCheckBox->setText(tr("Drag Move"));
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

    ui->okButton->setText(tr("OK"));
    ui->cancelButton->setText(tr("Cancel"));
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
    return s;
}
