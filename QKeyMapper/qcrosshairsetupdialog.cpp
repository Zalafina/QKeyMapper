#include "qkeymapper.h"
#include "qcrosshairsetupdialog.h"
#include "ui_qcrosshairsetupdialog.h"
#include "qstyle_singletons.h"

namespace {
template <typename Mutator>
void applyCrosshairControlChange(int itemRow, Mutator &&mutate)
{
    QKeyMapper *keyMapper = QKeyMapper::getInstance();
    if (!keyMapper || itemRow < 0) {
        return;
    }
    const int tabIndex = QKeyMapper::s_KeyMappingTabWidgetCurrentIndex;
    MappingCrosshairSettings settings = keyMapper->getMappingSourceRowCrosshairSettings(tabIndex, itemRow);
    mutate(settings);
    keyMapper->setMappingSourceRowCrosshairSettings(tabIndex, itemRow, settings);
}
}

using namespace QKeyMapperConstants;

QCrosshairSetupDialog *QCrosshairSetupDialog::m_instance = Q_NULLPTR;

QCrosshairSetupDialog::QCrosshairSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QCrosshairSetupDialog)
    , m_ItemRow(-1)
{
    m_instance = this;
    ui->setupUi(this);

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->centerGroupBox->setStyle(windowsStyle);
        ui->crosshairGroupBox->setStyle(windowsStyle);
        ui->offsetGroupBox->setStyle(windowsStyle);
    }

    if (QStyle *fusionStyle = QKeyMapperStyle::fusionStyle()) {
        const auto children = findChildren<QWidget*>();
        for (QWidget *child : children) {
            if (!qobject_cast<QGroupBox*>(child)) {
                child->setStyle(fusionStyle);
            }
        }
    }

    ui->centerColorPicker->setColorType("CenterColor");
    ui->crosshairColorPicker->setColorType("CrosshairColor");

    ui->centerSizeSpinBox->setRange(CROSSHAIR_CENTERSIZE_MIN, CROSSHAIR_CENTERSIZE_MAX);
    ui->centerOpacitySpinBox->setRange(CROSSHAIR_OPACITY_MIN, CROSSHAIR_OPACITY_MAX);
    ui->crosshairLineWidthSpinBox->setRange(CROSSHAIR_CROSSHAIRWIDTH_MIN, CROSSHAIR_CROSSHAIRWIDTH_MAX);
    ui->crosshairLineLengthSpinBox->setRange(CROSSHAIR_CROSSHAIRLENGTH_MIN, CROSSHAIR_CROSSHAIRLENGTH_MAX);
    ui->crosshairOpacitySpinBox->setRange(CROSSHAIR_OPACITY_MIN, CROSSHAIR_OPACITY_MAX);
    ui->x_offsetSpinBox->setRange(CROSSHAIR_X_OFFSET_MIN, CROSSHAIR_X_OFFSET_MAX);
    ui->y_offsetSpinBox->setRange(CROSSHAIR_Y_OFFSET_MIN, CROSSHAIR_Y_OFFSET_MAX);

    ui->centerOpacityLabel->setVisible(false);
    ui->centerOpacitySpinBox->setVisible(false);

    QObject::connect(ui->centerColorPicker, &ColorPickerWidget::colorChanged, this, &QCrosshairSetupDialog::onCenterColorChanged);
    QObject::connect(ui->crosshairColorPicker, &ColorPickerWidget::colorChanged, this, &QCrosshairSetupDialog::onCrosshairColorChanged);
}

QCrosshairSetupDialog::~QCrosshairSetupDialog()
{
    delete ui;
}

void QCrosshairSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    ui->centerColorPicker->setButtonText(tr("Color"));
    ui->crosshairColorPicker->setButtonText(tr("Color"));
    ui->centerColorPicker->setWindowTitle(tr("Select Crosshair Center Color"));
    ui->crosshairColorPicker->setWindowTitle(tr("Select Crosshair Color"));

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
            if (QKeyMapper::isSelectColorDialogVisible()) {
            }
            else {
                close();
            }
        }
    }
    return QDialog::event(event);
}

void QCrosshairSetupDialog::closeEvent(QCloseEvent *event)
{
    m_ItemRow = -1;

    QDialog::closeEvent(event);
}

void QCrosshairSetupDialog::showEvent(QShowEvent *event)
{
    QKeyMapper *keyMapper = QKeyMapper::getInstance();
    if (keyMapper && m_ItemRow >= 0 && QKeyMapper::KeyMappingDataList
        && m_ItemRow < QKeyMapper::KeyMappingDataList->size()) {
        const MappingCrosshairSettings settings =
            keyMapper->getMappingSourceRowCrosshairSettings(QKeyMapper::s_KeyMappingTabWidgetCurrentIndex, m_ItemRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QCrosshairSetupDialog::showEvent]" << " Load Crosshair Data[" << m_ItemRow << "]";
#endif

        const QSignalBlocker blockCenterColor(ui->centerColorPicker);
        const QSignalBlocker blockCenterSize(ui->centerSizeSpinBox);
        const QSignalBlocker blockCenterOpacity(ui->centerOpacitySpinBox);
        const QSignalBlocker blockCrosshairColor(ui->crosshairColorPicker);
        const QSignalBlocker blockCrosshairWidth(ui->crosshairLineWidthSpinBox);
        const QSignalBlocker blockCrosshairLength(ui->crosshairLineLengthSpinBox);
        const QSignalBlocker blockCrosshairOpacity(ui->crosshairOpacitySpinBox);
        const QSignalBlocker blockShowCenter(ui->showCenterCheckBox);
        const QSignalBlocker blockShowTop(ui->showTopCheckBox);
        const QSignalBlocker blockShowBottom(ui->showBottomCheckBox);
        const QSignalBlocker blockShowLeft(ui->showLeftCheckBox);
        const QSignalBlocker blockShowRight(ui->showRightCheckBox);
        const QSignalBlocker blockXOffset(ui->x_offsetSpinBox);
        const QSignalBlocker blockYOffset(ui->y_offsetSpinBox);

        /* Crosshair-Center config values */
        ui->centerColorPicker->setColor(settings.centerColor);
        ui->centerSizeSpinBox->setValue(settings.centerSize);
        ui->centerOpacitySpinBox->setValue(settings.centerOpacity);

        /* Crosshair-Crosshair config values */
        ui->crosshairColorPicker->setColor(settings.crosshairColor);
        ui->crosshairLineWidthSpinBox->setValue(settings.crosshairWidth);
        ui->crosshairLineLengthSpinBox->setValue(settings.crosshairLength);
        ui->crosshairOpacitySpinBox->setValue(settings.crosshairOpacity);

        /* Crosshair-Show config values */
        ui->showCenterCheckBox->setChecked(settings.showCenter);
        ui->showTopCheckBox->setChecked(settings.showTop);
        ui->showBottomCheckBox->setChecked(settings.showBottom);
        ui->showLeftCheckBox->setChecked(settings.showLeft);
        ui->showRightCheckBox->setChecked(settings.showRight);

        /* Crosshair-Offset config values */
        ui->x_offsetSpinBox->setValue(settings.xOffset);
        ui->y_offsetSpinBox->setValue(settings.yOffset);
    }

    QDialog::showEvent(event);
}

void QCrosshairSetupDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

void QCrosshairSetupDialog::onCenterColorChanged(QColor &color)
{
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.centerColor = color; });
}

void QCrosshairSetupDialog::onCrosshairColorChanged(QColor &color)
{
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.crosshairColor = color; });
}

void QCrosshairSetupDialog::on_showCenterCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    const bool show = ui->showCenterCheckBox->isChecked();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.showCenter = show; });
}

void QCrosshairSetupDialog::on_showTopCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    const bool show = ui->showTopCheckBox->isChecked();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.showTop = show; });
}

void QCrosshairSetupDialog::on_showBottomCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    const bool show = ui->showBottomCheckBox->isChecked();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.showBottom = show; });
}

void QCrosshairSetupDialog::on_showLeftCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    const bool show = ui->showLeftCheckBox->isChecked();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.showLeft = show; });
}

void QCrosshairSetupDialog::on_showRightCheckBox_stateChanged(int state)
{
    Q_UNUSED(state);
    const bool show = ui->showRightCheckBox->isChecked();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.showRight = show; });
}

void QCrosshairSetupDialog::on_centerSizeSpinBox_editingFinished()
{
    const int value = ui->centerSizeSpinBox->value();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.centerSize = value; });
}

void QCrosshairSetupDialog::on_centerOpacitySpinBox_editingFinished()
{
    const int value = ui->centerOpacitySpinBox->value();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.centerOpacity = value; });
}

void QCrosshairSetupDialog::on_crosshairLineWidthSpinBox_editingFinished()
{
    const int value = ui->crosshairLineWidthSpinBox->value();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.crosshairWidth = value; });
}

void QCrosshairSetupDialog::on_crosshairLineLengthSpinBox_editingFinished()
{
    const int value = ui->crosshairLineLengthSpinBox->value();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.crosshairLength = value; });
}

void QCrosshairSetupDialog::on_crosshairOpacitySpinBox_editingFinished()
{
    const int value = ui->crosshairOpacitySpinBox->value();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.crosshairOpacity = value; });
}

void QCrosshairSetupDialog::on_x_offsetSpinBox_editingFinished()
{
    const int value = ui->x_offsetSpinBox->value();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.xOffset = value; });
}

void QCrosshairSetupDialog::on_y_offsetSpinBox_editingFinished()
{
    const int value = ui->y_offsetSpinBox->value();
    applyCrosshairControlChange(m_ItemRow, [&](MappingCrosshairSettings &s) { s.yOffset = value; });
}
