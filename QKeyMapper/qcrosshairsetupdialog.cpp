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

    QObject::connect(m_CenterColorPicker, &ColorPickerWidget::colorChanged, this, &QCrosshairSetupDialog::onCenterColorChanged);
    QObject::connect(m_CrosshairColorPicker, &ColorPickerWidget::colorChanged, this, &QCrosshairSetupDialog::onCrosshairColorChanged);
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

void QCrosshairSetupDialog::reShow()
{
    if (QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus) {
        if (QKeyMapper::getInstance()->isHidden() == false) {
            if (!isVisible()) {
                show();
            }
        }
    }
}

bool QCrosshairSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            if (ColorPickerWidget::s_isColorSelecting) {
            }
            else {
                close();
            }
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

        /* Crosshair-Center config values */
        m_CenterColorPicker->setColor(keymapdata.Crosshair_CenterColor);
        ui->centerSizeSpinBox->setValue(keymapdata.Crosshair_CenterSize);
        ui->centerOpacitySpinBox->setValue(keymapdata.Crosshair_CenterOpacity);

        /* Crosshair-Crosshair config values */
        m_CrosshairColorPicker->setColor(keymapdata.Crosshair_CrosshairColor);
        ui->crosshairLineWidthSpinBox->setValue(keymapdata.Crosshair_CrosshairWidth);
        ui->crosshairLineLengthSpinBox->setValue(keymapdata.Crosshair_CrosshairLength);
        ui->crosshairOpacitySpinBox->setValue(keymapdata.Crosshair_CrosshairOpacity);

        /* Crosshair-Show config values */
        ui->showCenterCheckBox->setChecked(keymapdata.Crosshair_ShowCenter);
        ui->showTopCheckBox->setChecked(keymapdata.Crosshair_ShowTop);
        ui->showBottomCheckBox->setChecked(keymapdata.Crosshair_ShowBottom);
        ui->showLeftCheckBox->setChecked(keymapdata.Crosshair_ShowLeft);
        ui->showRightCheckBox->setChecked(keymapdata.Crosshair_ShowRight);

        /* Crosshair-Offset config values */
        ui->x_offsetSpinBox->setValue(keymapdata.Crosshair_X_Offset);
        ui->y_offsetSpinBox->setValue(keymapdata.Crosshair_Y_Offset);
    }

    QDialog::showEvent(event);
}

void QCrosshairSetupDialog::onCenterColorChanged(QColor &color)
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    if (color != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_CenterColor) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_CenterColor = color;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[onCenterColorSelected]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Center Color -> " << color;
#endif
    }

    // reShow();
}

void QCrosshairSetupDialog::onCrosshairColorChanged(QColor &color)
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    if (color != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_CrosshairColor) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_CrosshairColor = color;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[onCrosshairColorSelected]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Crosshair Color -> " << color;
#endif
    }

    // reShow();
}

void QCrosshairSetupDialog::on_showCenterCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool show = ui->showCenterCheckBox->isChecked();
    if (show != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_ShowCenter) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_ShowCenter = show;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Show Center -> " << show;
#endif
    }
}

void QCrosshairSetupDialog::on_showTopCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool show = ui->showTopCheckBox->isChecked();
    if (show != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_ShowTop) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_ShowTop = show;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Show Top -> " << show;
#endif
    }
}

void QCrosshairSetupDialog::on_showBottomCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool show = ui->showBottomCheckBox->isChecked();
    if (show != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_ShowBottom) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_ShowBottom = show;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Show Bottom -> " << show;
#endif
    }
}

void QCrosshairSetupDialog::on_showLeftCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool show = ui->showLeftCheckBox->isChecked();
    if (show != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_ShowLeft) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_ShowLeft = show;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Show Left -> " << show;
#endif
    }
}

void QCrosshairSetupDialog::on_showRightCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    bool show = ui->showRightCheckBox->isChecked();
    if (show != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_ShowRight) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_ShowRight = show;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[" << __func__ << "] Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Show Right -> " << show;
#endif
    }
}

void QCrosshairSetupDialog::on_centerSizeSpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->centerSizeSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_CenterSize) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_CenterSize = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_centerSizeSpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Center Size -> " << current_value;
#endif
    }
}

void QCrosshairSetupDialog::on_centerOpacitySpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->centerOpacitySpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_CenterOpacity) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_CenterOpacity = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_centerOpacitySpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Center Opacity -> " << current_value;
#endif
    }
}

void QCrosshairSetupDialog::on_crosshairLineWidthSpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->crosshairLineWidthSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_CrosshairWidth) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_CrosshairWidth = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_crosshairLineWidthSpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Crosshair Width -> " << current_value;
#endif
    }
}

void QCrosshairSetupDialog::on_crosshairLineLengthSpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->crosshairLineLengthSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_CrosshairLength) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_CrosshairLength = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_crosshairLineLengthSpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Crosshair Length -> " << current_value;
#endif
    }
}

void QCrosshairSetupDialog::on_crosshairOpacitySpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->crosshairOpacitySpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_CrosshairOpacity) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_CrosshairOpacity = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_crosshairOpacitySpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Crosshair Opacity -> " << current_value;
#endif
    }
}

void QCrosshairSetupDialog::on_x_offsetSpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->x_offsetSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_X_Offset) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_X_Offset = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_x_offsetSpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Crosshair Offset X -> " << current_value;
#endif
    }
}

void QCrosshairSetupDialog::on_y_offsetSpinBox_editingFinished()
{
    if (m_ItemRow < 0 || m_ItemRow >= QKeyMapper::KeyMappingDataList->size()) {
        return;
    }

    int current_value = ui->y_offsetSpinBox->value();

    if (current_value != QKeyMapper::KeyMappingDataList->at(m_ItemRow).Crosshair_Y_Offset) {
        (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Crosshair_Y_Offset = current_value;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[on_y_offsetSpinBox_editingFinished]" << " Row[" << m_ItemRow << "]["<< (*QKeyMapper::KeyMappingDataList)[m_ItemRow].Original_Key << "] Crosshair Offset Y -> " << current_value;
#endif
    }
}
