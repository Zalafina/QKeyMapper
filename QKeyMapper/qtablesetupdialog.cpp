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
        ui->exportTableButton->setText(EXPORTTABLEBUTTON_ENGLISH);
        ui->importTableButton->setText(IMPORTTABLEBUTTON_ENGLISH);
        ui->removeTableButton->setText(REMOVETABLEBUTTON_ENGLISH);
    }
    else {
        setWindowTitle(TABLESETUPDIALOG_WINDOWTITLE_CHINESE);

        ui->tabNameLabel->setText(TABNAMELABEL_CHINESE);
        ui->tabHotkeyLabel->setText(TABHOTKEYLABEL_CHINESE);
        ui->tabNameUpdateButton->setText(UPDATEBUTTON_CHINESE);
        ui->tabHotkeyUpdateButton->setText(UPDATEBUTTON_CHINESE);
        ui->exportTableButton->setText(EXPORTTABLEBUTTON_CHINESE);
        ui->importTableButton->setText(IMPORTTABLEBUTTON_CHINESE);
        ui->removeTableButton->setText(REMOVETABLEBUTTON_CHINESE);
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
        popupMessageColor = "#d63031";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = "TabName is empty!";
        }
        else {
            popupMessage = "未填写映射表名！";
        }
    }
    else if (tabNameString == QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_tabNameUpdateButton_clicked]" << "TabName was not modified, no action required.";
#endif
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
            popupMessageColor = "#44bd32";
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                popupMessage = "TabHotkey clear success";
            }
            else {
                popupMessage = "映射表快捷键清除成功";
            }

            QKeyMapper::getInstance()->updateKeyMappingTabInfoHotkey(m_TabIndex, ori_tabhotkeystring);
        }
    }
    else if (QKeyMapper::validateCombinationKey(tabhotkeystring))
    {
        popupMessageColor = "#44bd32";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = "TabHotkey update success: " + ori_tabhotkeystring;
        }
        else {
            popupMessage = "映射表快捷键更新成功: " + ori_tabhotkeystring;
        }

        QKeyMapper::getInstance()->updateKeyMappingTabInfoHotkey(m_TabIndex, ori_tabhotkeystring);
    }
    else
    {
        ui->tabHotkeyLineEdit->setText(QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabHotkey);

        popupMessageColor = "#d63031";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = "Invalid input format for TabHotkey!";
        }
        else {
            popupMessage = "映射表快捷键输入格式错误: " + ori_tabhotkeystring;
        }
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
    QString caption_string;
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        caption_string = "Export mapping data table : " +TabName;
    }
    else {
        caption_string = "导出映射表 : " +TabName;
    }

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
        popupMessageColor = "#44bd32";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = QString("Mapping data of table \"%1\" export successfully").arg(TabName);;
        }
        else {
            popupMessage = QString("映射表\"%1\"数据导出成功").arg(TabName);
        }
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
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        caption_string = "Import mapping data table : " +TabName;
    }
    else {
        caption_string = "导入映射表 : " +TabName;
    }

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
        popupMessageColor = "#44bd32";
        if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
            popupMessage = QString("Import mapping data to table \"%1\" successfully").arg(TabName);;
        }
        else {
            popupMessage = QString("映射表\"%1\"导入映射数据成功").arg(TabName);
        }
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
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        message = QString("Are you sure you want to remove the mapping table \"%1\"?").arg(TabName);
    }
    else {
        message = QString("请确认是否要删除映射表\"%1\"？").arg(TabName);
    }

    reply = QMessageBox::warning(parentWidget(), PROGRAM_NAME, message, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool remove_result = QKeyMapper::getInstance()->removeTabFromKeyMappingTabWidget(tabindex);

        if (remove_result) {
            QString popupMessage;
            QString popupMessageColor;
            int popupMessageDisplayTime = 3000;
            popupMessageColor = "#44bd32";
            if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
                popupMessage = QString("Mapping table \"%1\" removed successfully").arg(TabName);;
            }
            else {
                popupMessage = QString("映射表\"%1\"删除成功").arg(TabName);
            }
            emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        }
    }
}
