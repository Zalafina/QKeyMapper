#include "qgeneraladvanceddialog.h"
#include "ui_qgeneraladvanceddialog.h"
#include "qkeymapper_constants.h"
#include "qstyle_singletons.h"

#include <QApplication>

using namespace QKeyMapperConstants;

QGeneralAdvancedDialog *QGeneralAdvancedDialog::m_instance = Q_NULLPTR;

QGeneralAdvancedDialog::QGeneralAdvancedDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QGeneralAdvancedDialog)
{
    m_instance = this;
    ui->setupUi(this);

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->startupPositionGroupBox->setStyle(windowsStyle);
        ui->tableEditGroupBox->setStyle(windowsStyle);
        ui->globalMappingGroupBox->setStyle(windowsStyle);
    }

    if (QStyle *fusionStyle = QKeyMapperStyle::fusionStyle()) {
        ui->startupPositionComboBox->setStyle(fusionStyle);
        ui->startupSpecifyPositionXSpinBox->setStyle(fusionStyle);
        ui->startupSpecifyPositionYSpinBox->setStyle(fusionStyle);
        ui->tableEditModeTriggerComboBox->setStyle(fusionStyle);
        ui->tableInsertModeComboBox->setStyle(fusionStyle);
        ui->soundEffectCheckBox->setStyle(fusionStyle);
        ui->globalSettingSwitchTimerSpinBox->setStyle(fusionStyle);
    }

    QStringList startupPositionList;
    startupPositionList.append(tr("Default"));
    startupPositionList.append(tr("LastSaved"));
    startupPositionList.append(tr("Specify"));
    ui->startupPositionComboBox->addItems(startupPositionList);
    ui->startupPositionComboBox->setCurrentIndex(STARTUP_POSITION_LASTSAVED);

    QStringList editmodeList;
    editmodeList.append(tr("R-DoubleClick"));
    editmodeList.append(tr("L-DoubleClick"));
    ui->tableEditModeTriggerComboBox->addItems(editmodeList);
    ui->tableEditModeTriggerComboBox->setCurrentIndex(EDITMODE_RIGHT_DOUBLECLICK);

    QStringList insertModeList;
    insertModeList.append(tr("AboveCurrentRow"));
    insertModeList.append(tr("BelowCurrentRow"));
    ui->tableInsertModeComboBox->addItems(insertModeList);
    ui->tableInsertModeComboBox->setCurrentIndex(TABLE_INSERT_MODE_DEFAULT);

    ui->startupSpecifyPositionXSpinBox->setRange(STARTUP_SPECIFY_POSITION_MIN_X,
                                                 STARTUP_SPECIFY_POSITION_MAX_X);
    ui->startupSpecifyPositionYSpinBox->setRange(STARTUP_SPECIFY_POSITION_MIN_Y,
                                                 STARTUP_SPECIFY_POSITION_MAX_Y);
    ui->startupSpecifyPositionXSpinBox->setValue(STARTUP_SPECIFY_POSITION_DEFAULT.x());
    ui->startupSpecifyPositionYSpinBox->setValue(STARTUP_SPECIFY_POSITION_DEFAULT.y());

    ui->globalSettingSwitchTimerSpinBox->setRange(GLOBALSETTING_SWITCH_TIMEOUT_MIN,
                                                  GLOBALSETTING_SWITCH_TIMEOUT_MAX);
    ui->globalSettingSwitchTimerSpinBox->setValue(static_cast<int>(CHECK_GLOBALSETTING_SWITCH_TIMEOUT));
    ui->soundEffectCheckBox->setChecked(true);

    QObject::connect(ui->startupPositionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, [this](int) { updateStartupSpecifyPositionState(); });

    setUILanguage(0);
    updateStartupSpecifyPositionState();
}

QGeneralAdvancedDialog::~QGeneralAdvancedDialog()
{
    delete ui;
}

void QGeneralAdvancedDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("General Advanced Setting"));

    ui->startupPositionGroupBox->setTitle(tr("Startup Position"));
    ui->startupPositionLabel->setText(tr("Startup Position"));
    ui->startupSpecifyPositionXLabel->setText(tr("Position X"));
    ui->startupSpecifyPositionYLabel->setText(tr("Position Y"));

    ui->startupPositionComboBox->setItemText(STARTUP_POSITION_DEFAULT,      tr("Default"));
    ui->startupPositionComboBox->setItemText(STARTUP_POSITION_LASTSAVED,    tr("LastSaved"));
    ui->startupPositionComboBox->setItemText(STARTUP_POSITION_SPECIFY,      tr("Specify"));

    ui->tableEditGroupBox->setTitle(tr("Table Edit"));
    ui->tableEditModeTriggerLabel->setText(tr("EditMode"));
    ui->tableInsertModeLabel->setText(tr("InsertMode"));
    ui->tableEditModeTriggerComboBox->setItemText(EDITMODE_RIGHT_DOUBLECLICK, tr("R-DoubleClick"));
    ui->tableEditModeTriggerComboBox->setItemText(EDITMODE_LEFT_DOUBLECLICK, tr("L-DoubleClick"));
    ui->tableInsertModeComboBox->setItemText(TABLE_INSERT_MODE_ABOVE, tr("AboveCurrentRow"));
    ui->tableInsertModeComboBox->setItemText(TABLE_INSERT_MODE_BELOW, tr("BelowCurrentRow"));

    ui->globalMappingGroupBox->setTitle(tr("Others"));
    ui->soundEffectCheckBox->setText(tr("Notification Sound"));
    ui->globalSettingSwitchTimerLabel->setText(tr("GlobalSwitchTimer"));
    ui->globalSettingSwitchTimerSpinBox->setSuffix(tr(" ms"));
}

int QGeneralAdvancedDialog::getStartupPosition()
{
    return ui->startupPositionComboBox->currentIndex();
}

QPoint QGeneralAdvancedDialog::getSpecifyStartupPosition()
{
    return QPoint(ui->startupSpecifyPositionXSpinBox->value(),
                  ui->startupSpecifyPositionYSpinBox->value());
}

int QGeneralAdvancedDialog::getTableEditModeTrigger()
{
    return ui->tableEditModeTriggerComboBox->currentIndex();
}

int QGeneralAdvancedDialog::getTableInsertMode()
{
    return ui->tableInsertModeComboBox->currentIndex();
}

bool QGeneralAdvancedDialog::getPlaySoundEffect()

{
    return ui->soundEffectCheckBox->isChecked();
}

unsigned int QGeneralAdvancedDialog::getGlobalSettingSwitchTimeout()
{
    return static_cast<unsigned int>(ui->globalSettingSwitchTimerSpinBox->value());
}

void QGeneralAdvancedDialog::setStartupPosition(int position)
{
    ui->startupPositionComboBox->setCurrentIndex(position);
    updateStartupSpecifyPositionState();
}

void QGeneralAdvancedDialog::setSpecifyStartupPosition(const QPoint &position)
{
    ui->startupSpecifyPositionXSpinBox->setValue(position.x());
    ui->startupSpecifyPositionYSpinBox->setValue(position.y());
}

void QGeneralAdvancedDialog::setTableEditModeTrigger(int trigger)
{
    ui->tableEditModeTriggerComboBox->setCurrentIndex(trigger);
}

void QGeneralAdvancedDialog::setTableInsertMode(int mode)
{
    ui->tableInsertModeComboBox->setCurrentIndex(mode);
}

void QGeneralAdvancedDialog::setPlaySoundEffect(bool enabled)
{
    ui->soundEffectCheckBox->setChecked(enabled);
}

void QGeneralAdvancedDialog::setGlobalSettingSwitchTimeout(unsigned int timeout)
{
    ui->globalSettingSwitchTimerSpinBox->setValue(static_cast<int>(timeout));
}

bool QGeneralAdvancedDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            QWidget *activePopup = QApplication::activePopupWidget();
            if (activePopup && (activePopup == this || isAncestorOf(activePopup))) {
                return QDialog::event(event);
            }
            close();
        }
    }
    return QDialog::event(event);
}

void QGeneralAdvancedDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

void QGeneralAdvancedDialog::updateStartupSpecifyPositionState()
{
    const bool enabled = (ui->startupPositionComboBox->currentIndex() == STARTUP_POSITION_SPECIFY);
    ui->startupSpecifyPositionXLabel->setEnabled(enabled);
    ui->startupSpecifyPositionYLabel->setEnabled(enabled);
    ui->startupSpecifyPositionXSpinBox->setEnabled(enabled);
    ui->startupSpecifyPositionYSpinBox->setEnabled(enabled);
}
