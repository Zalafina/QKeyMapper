#include "qkeymapper.h"
#include "qkeymapper_constants.h"
#include "qtablesetupdialog.h"
#include "ui_qtablesetupdialog.h"

QTableSetupDialog *QTableSetupDialog::m_instance = Q_NULLPTR;

QTableSetupDialog::QTableSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTableSetupDialog)
    , m_TabIndex(-1)
{
    m_instance = this;
    ui->setupUi(this);

    ui->tabNameLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->tabHotkeyLineEdit->setFocusPolicy(Qt::ClickFocus);

    ui->tabNameLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->tabHotkeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));

    QObject::connect(ui->tabNameLineEdit, &QLineEdit::returnPressed, this, &QTableSetupDialog::on_tabNameUpdateButton_clicked);
    QObject::connect(ui->tabHotkeyLineEdit, &QLineEdit::returnPressed, this, &QTableSetupDialog::on_tabHotkeyUpdateButton_clicked);
}

QTableSetupDialog::~QTableSetupDialog()
{
    delete ui;
}

void QTableSetupDialog::setUILanguagee(int languageindex)
{
    if (LANGUAGE_ENGLISH == languageindex) {
        setWindowTitle(TABLESETUPDIALOG_WINDOWTITLE_ENGLISH);

        ui->tabNameLabel->setText(TABNAMELABEL_ENGLISH);
        ui->tabHotkeyLabel->setText(TABHOTKEYLABEL_ENGLISH);
        ui->tabNameUpdateButton->setText(UPDATEBUTTON_ENGLISH);
        ui->tabHotkeyUpdateButton->setText(UPDATEBUTTON_ENGLISH);
    }
    else {
        setWindowTitle(TABLESETUPDIALOG_WINDOWTITLE_CHINESE);

        ui->tabNameLabel->setText(TABNAMELABEL_CHINESE);
        ui->tabHotkeyLabel->setText(TABHOTKEYLABEL_CHINESE);
        ui->tabNameUpdateButton->setText(UPDATEBUTTON_CHINESE);
        ui->tabHotkeyUpdateButton->setText(UPDATEBUTTON_CHINESE);
    }
}

void QTableSetupDialog::resetFontSize()
{
    int scale = QKeyMapper::getInstance()->m_UI_Scale;
    QFont customFont;
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        customFont.setFamily(FONTNAME_ENGLISH);
        customFont.setPointSize(9);
    }
    else {
        customFont.setFamily(FONTNAME_CHINESE);
        customFont.setBold(true);

        if (UI_SCALE_4K_PERCENT_150 == scale) {
            customFont.setPointSize(11);
        }
        else {
            customFont.setPointSize(9);
        }
    }

    ui->tabNameLabel->setFont(customFont);
    ui->tabHotkeyLabel->setFont(customFont);
    ui->tabNameUpdateButton->setFont(customFont);
    ui->tabHotkeyUpdateButton->setFont(customFont);
}

void QTableSetupDialog::setTabIndex(int tabindex)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QTableSetupDialog::setTabIndex]" << "Tab Index =" << tabindex;
#endif

    m_TabIndex = tabindex;
}

bool QTableSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QWidget::event(event);
}

void QTableSetupDialog::closeEvent(QCloseEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QTableSetupDialog::closeEvent]" << "Tab Index initialize to -1";
#endif
    m_TabIndex = -1;

    QDialog::closeEvent(event);
}

void QTableSetupDialog::showEvent(QShowEvent *event)
{
    if (0 <= m_TabIndex && m_TabIndex < QKeyMapper::s_KeyMappingTabInfoList.size()) {
        QString TabName = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName;
        QString TabHotkey = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabHotkey;
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QTableSetupDialog::showEvent]" << "TabIndex[" << m_TabIndex << "] -> TabName(" << TabName << "), Hotkey(" << TabHotkey << ")";
#endif

        /* Load TabNamc String */
        ui->tabNameLineEdit->setText(TabName);

        /* Load TabHotkey String */
        ui->tabHotkeyLineEdit->setText(TabHotkey);
    }

    QDialog::showEvent(event);
}

void QTableSetupDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QTableSetupDialog::keyPressEvent]" << "ESC Key Pressed, Tab Index initialize to -1 and close TableSetupDialog.";
#endif
        m_TabIndex = -1;
    }

    QDialog::keyPressEvent(event);
}

void QTableSetupDialog::on_tabNameUpdateButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    QString tabNameString = ui->tabNameLineEdit->text();
    bool isduplicate = QKeyMapper::isTabTextDuplicate(tabNameString);

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;

    if (tabNameString.isEmpty()) {
        popupMessageColor = "#d63031";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = "TabName is empty!";
        }
        else {
            popupMessage = "未填写映射表名!";
        }
    }
    else if (isduplicate) {
        popupMessageColor = "#d63031";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = "TabName is duplicate: " + tabNameString;
        }
        else {
            popupMessage = "存在相同的映射表名: " + tabNameString;
        }
    }
    else {
        popupMessageColor = "#44bd32";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = "TabName update success: " + tabNameString;
        }
        else {
            popupMessage = "映射表名更新成功: " + tabNameString;
        }

        if (QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName != tabNameString) {
            QKeyMapper::getInstance()->updateKeyMappingTabWidgetTabName(m_TabIndex, tabNameString);
        }
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}


void QTableSetupDialog::on_tabHotkeyUpdateButton_clicked()
{

}
