#include "qkeymapper.h"
#include "qitemsetupdialog.h"
#include "ui_qitemsetupdialog.h"
#include "qkeymapper_constants.h"

QItemSetupDialog *QItemSetupDialog::m_instance = Q_NULLPTR;

QItemSetupDialog::QItemSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QItemSetupDialog)
    , m_ItemRow(-1)
    , m_OriginalKeyListComboBox(new KeyListComboBox(this))
    , m_MappingKeyListComboBox(new KeyListComboBox(this))
{
    m_instance = this;
    ui->setupUi(this);

    initKeyListComboBoxes();
}

QItemSetupDialog::~QItemSetupDialog()
{
    delete ui;
}

void QItemSetupDialog::setItemRow(int row)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::setItemRow]" << "Item Row =" << row;
#endif

    m_ItemRow = row;
}

QString QItemSetupDialog::getOriginalKeyText()
{
    return getInstance()->ui->originalKeyLineEdit->text();
}

void QItemSetupDialog::setOriginalKeyText(const QString &new_keytext)
{
    return getInstance()->ui->originalKeyLineEdit->setText(new_keytext);
}

QString QItemSetupDialog::getMappingKeyText()
{
    return getInstance()->ui->mappingKeyLineEdit->text();
}

void QItemSetupDialog::setMappingKeyText(const QString &new_keytext)
{
    return getInstance()->ui->mappingKeyLineEdit->setText(new_keytext);
}

void QItemSetupDialog::closeEvent(QCloseEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QItemSetupDialog::closeEvent]" << "Item Row initialize to -1";
#endif

    m_ItemRow = -1;

    QDialog::closeEvent(event);
}

void QItemSetupDialog::showEvent(QShowEvent *event)
{
    if (m_ItemRow >= 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QItemSetupDialog::showEvent]" << "Load Item Mapping Data ->";
#endif
    }

    QDialog::showEvent(event);
}

void QItemSetupDialog::initKeyListComboBoxes()
{
    KeyListComboBox *orikeyComboBox = QKeyMapper::getInstance()->m_orikeyComboBox;
    KeyListComboBox *mapkeyComboBox = QKeyMapper::getInstance()->m_mapkeyComboBox;

    for(int i = 0; i < orikeyComboBox->count(); i++) {
        QString text = orikeyComboBox->itemText(i);
        m_OriginalKeyListComboBox->addItem(text);
    }

    for(int i = 0; i < mapkeyComboBox->count(); i++) {
        QString text = mapkeyComboBox->itemText(i);
        m_MappingKeyListComboBox->addItem(text);
    }

    int left = ui->orikeyListLabel->x() + ui->orikeyListLabel->width() + 5;
    int top = ui->orikeyListLabel->y();
    m_OriginalKeyListComboBox->setObjectName(SETUPDIALOG_ORIKEY_COMBOBOX_NAME);
    m_OriginalKeyListComboBox->setGeometry(QRect(left, top, 142, 22));

    left = ui->mapkeyListLabel->x() + ui->mapkeyListLabel->width() + 5;
    top = ui->mapkeyListLabel->y();
    m_MappingKeyListComboBox->setObjectName(SETUPDIALOG_MAPKEY_COMBOBOX_NAME);
    m_MappingKeyListComboBox->setGeometry(QRect(left, top, 142, 22));
}
