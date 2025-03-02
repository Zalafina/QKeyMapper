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

void QTableSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);

    setWindowTitle(tr(TABLESETUPDIALOG_WINDOWTITLE_STR));
    ui->tabNameLabel->setText(tr(TABNAMELABEL_STR));
    ui->tabHotkeyLabel->setText(tr(TABHOTKEYLABEL_STR));
    ui->tabNameUpdateButton->setText(tr(UPDATEBUTTON_STR));
    ui->tabHotkeyUpdateButton->setText(tr(UPDATEBUTTON_STR));
    ui->exportTableButton->setText(tr(EXPORTTABLEBUTTON_STR));
    ui->importTableButton->setText(tr(IMPORTTABLEBUTTON_STR));
    ui->removeTableButton->setText(tr(REMOVETABLEBUTTON_STR));
}

void QTableSetupDialog::resetFontSize()
{
    // int scale = QKeyMapper::getInstance()->m_UI_Scale;
    QFont customFont;
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        customFont.setFamily(FONTNAME_ENGLISH);
        customFont.setPointSize(9);
    }
    else if (LANGUAGE_JAPANESE == QKeyMapper::getLanguageIndex()) {
        customFont.setFamily(FONTNAME_ENGLISH);
        customFont.setPointSize(9);
    }
    else {
        customFont.setFamily(FONTNAME_ENGLISH);
        customFont.setPointSize(9);

        // customFont.setFamily(FONTNAME_CHINESE);
        // customFont.setBold(true);

        // if (UI_SCALE_4K_PERCENT_150 == scale) {
        //     customFont.setPointSize(11);
        // }
        // else {
        //     customFont.setPointSize(9);
        // }
    }

    ui->tabNameLabel->setFont(customFont);
    ui->tabHotkeyLabel->setFont(customFont);
    ui->tabNameUpdateButton->setFont(customFont);
    ui->tabHotkeyUpdateButton->setFont(customFont);
    ui->exportTableButton->setFont(customFont);
    ui->importTableButton->setFont(customFont);
    ui->removeTableButton->setFont(customFont);
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
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("TabName is empty!");
    }
    else if (tabNameString == QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_tabNameUpdateButton_clicked]" << "TabName was not modified, no action required.";
#endif
    }
    else if (isduplicate) {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("TabName is duplicate: ") + tabNameString;
    }
    else {
        popupMessageColor = SUCCESS_COLOR;
        popupMessage = tr("TabName update success: ") + tabNameString;

        if (QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName != tabNameString) {
            QKeyMapper::getInstance()->updateKeyMappingTabWidgetTabName(m_TabIndex, tabNameString);
        }
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}


void QTableSetupDialog::on_tabHotkeyUpdateButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    QString ori_tabhotkeystring = ui->tabHotkeyLineEdit->text();
    QString tabhotkeystring = ori_tabhotkeystring;
    if (tabhotkeystring.startsWith(PREFIX_PASSTHROUGH)) {
        tabhotkeystring.remove(0, 1);
    }

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = 3000;

    if (ori_tabhotkeystring.isEmpty()) {
        if (QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabHotkey.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[on_tabHotkeyUpdateButton_clicked]" << "TabHotkey is empty; no action required.";
#endif
        }
        else {
            popupMessageColor = SUCCESS_COLOR;
            popupMessage = tr("TabHotkey clear success");

            QKeyMapper::getInstance()->updateKeyMappingTabInfoHotkey(m_TabIndex, ori_tabhotkeystring);
        }
    }
    else if (QKeyMapper::validateCombinationKey(tabhotkeystring))
    {
        popupMessageColor = SUCCESS_COLOR;
        popupMessage = tr("TabHotkey update success: ") + ori_tabhotkeystring;

        QKeyMapper::getInstance()->updateKeyMappingTabInfoHotkey(m_TabIndex, ori_tabhotkeystring);
    }
    else
    {
        ui->tabHotkeyLineEdit->setText(QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabHotkey);

        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("Invalid input format for TabHotkey: %1").arg(ori_tabhotkeystring);
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}

void QTableSetupDialog::on_exportTableButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    int tabindex = m_TabIndex;
    QString TabName = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName;
    QString default_filename = "mapdatatable.ini";
    QString filter = "INI files (*.ini)";
    QString caption_string = tr("Export mapping data table : ") +TabName;

    QString export_filename = QFileDialog::getSaveFileName(parentWidget(),
                                                           caption_string,
                                                           default_filename,
                                                           filter);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[on_exportTableButton_clicked]" << "export_filename from QFileDialog -> TabIndex[" << tabindex << "] : " << export_filename;
#endif

    bool export_result = QKeyMapper::exportKeyMappingDataToFile(tabindex, export_filename);

    if (export_result) {
        //Show success popup message
        QString popupMessage;
        QString popupMessageColor;
        int popupMessageDisplayTime = 3000;
        popupMessageColor = SUCCESS_COLOR;
        popupMessage = tr("Mapping data of table \"%1\" export successfully").arg(TabName);;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
    }
}


void QTableSetupDialog::on_importTableButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    int tabindex = m_TabIndex;
    QString TabName = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName;
    QString filter = "INI files (*.ini)";
    QString caption_string;
    caption_string = tr("Import mapping data table : ") +TabName;

    QString import_filename = QFileDialog::getOpenFileName(parentWidget(),
                                                           caption_string,
                                                           NULL,
                                                           filter);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[on_importTableButton_clicked]" << "import_filename from QFileDialog -> TabIndex[" << tabindex << "] : " << import_filename;
#endif

    bool import_result = QKeyMapper::importKeyMappingDataFromFile(tabindex, import_filename);

    if (import_result) {
        QKeyMapper::getInstance()->refreshKeyMappingDataTableByTabIndex(tabindex);

        //Show success popup message
        QString popupMessage;
        QString popupMessageColor;
        int popupMessageDisplayTime = 3000;
        popupMessageColor = SUCCESS_COLOR;
        popupMessage = tr("Import mapping data to table \"%1\" successfully").arg(TabName);
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
    }
}

void QTableSetupDialog::on_removeTableButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    int tabindex = m_TabIndex;
    QString TabName = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName;
    QString message;
    QMessageBox::StandardButton reply;
    message = tr("Are you sure you want to remove the mapping table \"%1\"?").arg(TabName);
    reply = QMessageBox::warning(parentWidget(), PROGRAM_NAME, message, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        int remove_result = QKeyMapper::getInstance()->removeTabFromKeyMappingTabWidget(tabindex);

        QString popupMessage;
        QString popupMessageColor;
        int popupMessageDisplayTime = 3000;
        if (REMOVE_MAPPINGTAB_SUCCESS == remove_result) {
            popupMessageColor = SUCCESS_COLOR;
            popupMessage = tr("Mapping table \"%1\" removed successfully").arg(TabName);;
            emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        }
        else if (REMOVE_MAPPINGTAB_LASTONE == remove_result) {
            popupMessageColor = FAILURE_COLOR;
            popupMessage = tr("Cannot remove the last mapping table!");
            emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        }
    }
}
