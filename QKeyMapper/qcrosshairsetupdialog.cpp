#include "qkeymapper.h"
#include "qcrosshairsetupdialog.h"
#include "ui_qcrosshairsetupdialog.h"

QCrosshairSetupDialog *QCrosshairSetupDialog::m_instance = Q_NULLPTR;

QCrosshairSetupDialog::QCrosshairSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QCrosshairSetupDialog)
    , m_ItemRow(-1)
    , m_CenterColorPicker(new ColorPickerWidget(this))
    , m_CrosshairColorPicker(new ColorPickerWidget(this))
{
    m_instance = this;
    ui->setupUi(this);

    QStyle* windowsStyle = QStyleFactory::create("windows");
    ui->centerGroupBox->setStyle(windowsStyle);
    ui->crosshairGroupBox->setStyle(windowsStyle);
    ui->offsetGroupBox->setStyle(windowsStyle);

    int x_offset = 15;
    int y_offset = 10;
    QRect centerWidgetGeometry = ui->centerGroupBox->geometry();
    int center_color_x = centerWidgetGeometry.x();
    int center_color_y = centerWidgetGeometry.y();
    center_color_x += x_offset;
    center_color_y += y_offset;
    // Set position for the center color picker
    m_CenterColorPicker->move(center_color_x, center_color_y);

    QRect crosshairWidgetGeometry = ui->crosshairGroupBox->geometry();
    int crosshair_color_x = crosshairWidgetGeometry.x();
    int crosshair_color_y = crosshairWidgetGeometry.y();
    crosshair_color_x += x_offset;
    crosshair_color_y += y_offset;
    // Set position for the crosshair color picker
    m_CrosshairColorPicker->move(crosshair_color_x, crosshair_color_y);

    m_CenterColorPicker->raise();
    m_CrosshairColorPicker->raise();

    ui->centerSizeSpinBox->setRange(CROSSHAIR_CENTERSIZE_MIN, CROSSHAIR_CENTERSIZE_MAX);
    ui->centerOpacitySpinBox->setRange(CROSSHAIR_OPACITY_MIN, CROSSHAIR_OPACITY_MAX);
    ui->crosshairLineWidthSpinBox->setRange(CROSSHAIR_CROSSHAIRWIDTH_MIN, CROSSHAIR_CROSSHAIRWIDTH_MAX);
    ui->crosshairLineLengthSpinBox->setRange(CROSSHAIR_CROSSHAIRLENGTH_MIN, CROSSHAIR_CROSSHAIRLENGTH_MAX);
    ui->crosshairOpacitySpinBox->setRange(CROSSHAIR_OPACITY_MIN, CROSSHAIR_OPACITY_MAX);
    ui->x_offsetSpinBox->setRange(CROSSHAIR_X_OFFSET_MIN, CROSSHAIR_X_OFFSET_MAX);
    ui->y_offsetSpinBox->setRange(CROSSHAIR_Y_OFFSET_MIN, CROSSHAIR_Y_OFFSET_MAX);
}

QCrosshairSetupDialog::~QCrosshairSetupDialog()
{
    delete ui;
}

void QCrosshairSetupDialog::setUILanguage(int languageindex)
{
    m_CenterColorPicker->setUILanguage(languageindex);
    m_CrosshairColorPicker->setUILanguage(languageindex);

    setWindowTitle(tr("Crosshair Setup"));
    ui->centerGroupBox->setTitle(tr("Center"));
    ui->crosshairGroupBox->setTitle(tr("Crosshair"));
    ui->offsetGroupBox->setTitle(tr("Offset"));

    ui->centerSizeLabel->setText(tr("Size"));
    ui->centerOpacityLabel->setText(tr("Opacity"));
    ui->crosshairLineWidthLabel->setText(tr("Width"));
    ui->crosshairLineLengthLabel->setText(tr("Length"));
    ui->crosshairOpacityLabel->setText(tr("Opacity"));

    ui->showCenterCheckBox->setText(tr("Show Center"));
    ui->showTopCheckBox->setText(tr("Show Top"));
    ui->showBottomCheckBox->setText(tr("Show Bottom"));
    ui->showLeftCheckBox->setText(tr("Show Left"));
    ui->showRightCheckBox->setText(tr("Show Right"));

    ui->x_offsetLabel->setText(tr("X-Offset"));
    ui->y_offsetLabel->setText(tr("Y-Offset"));
}

void QCrosshairSetupDialog::setItemRow(int row)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QCrosshairSetupDialog::setItemRow]" << "Item Row =" << row;
#endif

    m_ItemRow = row;
}

bool QCrosshairSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QWidget::event(event);
}

void QCrosshairSetupDialog::closeEvent(QCloseEvent *event)
{
    m_ItemRow = -1;

    QDialog::closeEvent(event);
}

void QCrosshairSetupDialog::showEvent(QShowEvent *event)
{
    if (m_ItemRow >= 0 && m_ItemRow < QKeyMapper::KeyMappingDataList->size()) {
        MAP_KEYDATA keymapdata = QKeyMapper::KeyMappingDataList->at(m_ItemRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QCrosshairSetupDialog::showEvent]" << "Load Key Mapping Data[" << m_ItemRow << "] ->" << keymapdata;
#endif

        // ui->centerSizeSpinBox->setValue(keymapdata.center_size);
        // ui->centerOpacitySpinBox->setValue(keymapdata.center_opacity);
        // ui->crosshairLineWidthSpinBox->setValue(keymapdata.crosshair_width);
        // ui->crosshairLineLengthSpinBox->setValue(keymapdata.crosshair_length);
        // ui->crosshairOpacitySpinBox->setValue(keymapdata.crosshair_opacity);
        // ui->x_offsetSpinBox->setValue(keymapdata.x_offset);
        // ui->y_offsetSpinBox->setValue(keymapdata.y_offset);

        // m_CenterColorPicker->setColor(keymapdata.center_color);
        // m_CrosshairColorPicker->setColor(keymapdata.crosshair_color);

        // ui->showCenterCheckBox->setChecked(keymapdata.show_center);
        // ui->showTopCheckBox->setChecked(keymapdata.show_top);
        // ui->showBottomCheckBox->setChecked(keymapdata.show_bottom);
        // ui->showLeftCheckBox->setChecked(keymapdata.show_left);
        // ui->showRightCheckBox->setChecked(keymapdata.show_right);
    }

    QDialog::showEvent(event);
}
