#include "qkeymapper.h"
#include "qkeymapper_constants.h"
#include "qtablesetupdialog.h"
#include "ui_qtablesetupdialog.h"

QTableSetupDialog *QTableSetupDialog::m_instance = Q_NULLPTR;

QTableSetupDialog::QTableSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTableSetupDialog)
    , m_TabIndex(-1)
    , m_SettingSelectIndex(-1)
    , m_NotificationFontColorPicker(new ColorPickerWidget(this, "TabFontColor", COLORPICKER_BUTTON_WIDTH_TABFONTCOLOR))
{
    m_instance = this;
    ui->setupUi(this);

    initSelectImageFileDialog();

    QStyle* windowsStyle = QStyleFactory::create("windows");
    ui->tabCustomImageGroupBox->setStyle(windowsStyle);
    ui->customImageLabel->setStyle(windowsStyle);

    ui->tabNameLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->tabHotkeyLineEdit->setFocusPolicy(Qt::ClickFocus);

    ui->tabNameLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    ui->tabHotkeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));

    ui->customImagePaddingSpinBox->setRange(TAB_CUSTOMIMAGE_PADDING_MIN, TAB_CUSTOMIMAGE_PADDING_MAX);
    ui->customImagePaddingSpinBox->setValue(TAB_CUSTOMIMAGE_PADDING_DEFAULT);

    int x_offset = 0;
    int y_offset = 30;
    QRect exportTableButtonGeometry = ui->exportTableButton->geometry();
    int tabfont_color_x = exportTableButtonGeometry.x();
    int tabfont_color_y = exportTableButtonGeometry.y();
    tabfont_color_x += x_offset;
    tabfont_color_y += y_offset;
    // Set position for the tabfont color picker
    m_NotificationFontColorPicker->move(tabfont_color_x, tabfont_color_y);
    m_NotificationFontColorPicker->raise();

    QStringList showPositionList;
    showPositionList.append(tr("None"));
    showPositionList.append(tr("Left"));
    showPositionList.append(tr("Right"));
    ui->customImageShowPositionComboBox->addItems(showPositionList);
    ui->customImageShowPositionComboBox->setCurrentIndex(TAB_CUSTOMIMAGE_POSITION_DEFAULT);

    QStringList trayiconPixelList;
    trayiconPixelList.append(tr("Default"));
    trayiconPixelList.append("16x16");
    trayiconPixelList.append("24x24");
    trayiconPixelList.append("32x32");
    trayiconPixelList.append("48x48");
    trayiconPixelList.append("64x64");
    trayiconPixelList.append("96x96");
    trayiconPixelList.append("128x128");
    trayiconPixelList.append("256x256");
    ui->customImageTrayIconPixelComboBox->addItems(trayiconPixelList);
    ui->customImageTrayIconPixelComboBox->setCurrentIndex(TAB_CUSTOMIMAGE_TRAYICON_PIXEL_DEFAULT);

    QObject::connect(ui->tabNameLineEdit, &QLineEdit::returnPressed, this, &QTableSetupDialog::on_tabNameUpdateButton_clicked);
    QObject::connect(ui->tabHotkeyLineEdit, &QLineEdit::returnPressed, this, &QTableSetupDialog::on_tabHotkeyUpdateButton_clicked);
    QObject::connect(m_NotificationFontColorPicker, &ColorPickerWidget::colorChanged, this, &QTableSetupDialog::onTabFontColorChanged);
}

QTableSetupDialog::~QTableSetupDialog()
{
    delete ui;
}

void QTableSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);

    m_NotificationFontColorPicker->setButtonText(tr("TabFontColor"));
    m_NotificationFontColorPicker->setWindowTitle(tr("Select Notification Font Color"));
    setWindowTitle(tr(TABLESETUPDIALOG_WINDOWTITLE_STR));
    ui->tabNameLabel->setText(tr(TABNAMELABEL_STR));
    ui->tabHotkeyLabel->setText(tr(TABHOTKEYLABEL_STR));
    ui->tabNameUpdateButton->setText(tr(UPDATEBUTTON_STR));
    ui->tabHotkeyUpdateButton->setText(tr(UPDATEBUTTON_STR));
    ui->exportTableButton->setText(tr(EXPORTTABLEBUTTON_STR));
    ui->importTableButton->setText(tr(IMPORTTABLEBUTTON_STR));
    ui->removeTableButton->setText(tr(REMOVETABLEBUTTON_STR));

    ui->tabCustomImageGroupBox->setTitle(tr("Tab Custom Image"));
    ui->selectCustomImageButton->setText(tr("Select Custom Image"));
    ui->customImageShowPositionLabel->setText(tr("ShowPositoin"));
    ui->customImagePaddingLabel->setText(tr("Padding"));
    ui->customImageShowAsTrayIconCheckBox->setText(tr("Show as TrayIcon"));

    ui->customImageShowPositionComboBox->setItemText(TAB_CUSTOMIMAGE_SHOW_NONE,     tr("None"));
    ui->customImageShowPositionComboBox->setItemText(TAB_CUSTOMIMAGE_SHOW_LEFT,     tr("Left"));
    ui->customImageShowPositionComboBox->setItemText(TAB_CUSTOMIMAGE_SHOW_RIGHT,    tr("Right"));

    ui->customImageTrayIconPixelLabel->setText(tr("TrayIcon Pixel"));
    ui->customImageTrayIconPixelComboBox->setItemText(TAB_CUSTOMIMAGE_TRAYICON_PIXEL_DEFAULT, tr("Default"));
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

void QTableSetupDialog::setSettingSelectIndex(int index)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QTableSetupDialog::setSettingSelectIndex]" << "Setting Select Index =" << index;
#endif
    m_SettingSelectIndex = index;
}

void QTableSetupDialog::initSelectImageFileDialog()
{
    m_SelectImageFileDialog = new QFileDialog(this);
    m_SelectImageFileDialog->setFileMode(QFileDialog::ExistingFile);
}

void QTableSetupDialog::updateTrayIconPixelComboBoxIcons(const QIcon &icon)
{
    // 1. If the icon is null, clear all item icons
    if (icon.isNull()) {
        for (int i = 0; i < ui->customImageTrayIconPixelComboBox->count(); ++i) {
            ui->customImageTrayIconPixelComboBox->setItemIcon(i, QIcon());
        }
        return;
    }

    // 2. Get available icon sizes
    QList<QSize> availableSizes = icon.availableSizes();

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QTableSetupDialog::updateTrayIconPixelComboBoxIcons]" << "Icon availableSizes:" << availableSizes;
#endif

    // 3. Iterate through all ComboBox items (start from index 1, skip "Default" item)
    for (int i = 1; i < ui->customImageTrayIconPixelComboBox->count(); ++i) {
        QString itemText = ui->customImageTrayIconPixelComboBox->itemText(i);

        // Parse item text to get size information (e.g., "16x16" -> QSize(16, 16))
        QStringList sizeParts = itemText.split('x');
        if (sizeParts.size() == 2) {
            bool ok1, ok2;
            int width = sizeParts[0].toInt(&ok1);
            int height = sizeParts[1].toInt(&ok2);

            if (ok1 && ok2) {
                QSize itemSize(width, height);

                // Check if this size is in the available sizes list
                if (availableSizes.contains(itemSize)) {
                    // Extract icon of corresponding size and set it to the item
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                    QPixmap scaled_pixmap = icon.pixmap(itemSize, 1.0);
#else
                    qreal devicePixelRatio = qApp->devicePixelRatio();
                    QSize deviceIndependentSize = QSize(
                        qRound(itemSize.width() / devicePixelRatio),
                        qRound(itemSize.height() / devicePixelRatio)
                        );
                    QPixmap scaled_pixmap = icon.pixmap(deviceIndependentSize);
#endif
                    QIcon itemIcon(scaled_pixmap);
                    ui->customImageTrayIconPixelComboBox->setItemIcon(i, itemIcon);
                } else {
                    // If the size is not available, clear the item's icon
                    ui->customImageTrayIconPixelComboBox->setItemIcon(i, QKeyMapper::s_Icon_Blank);
                }
            }
        }
    }

    // "Default" item (index 0) uses the original icon
    ui->customImageTrayIconPixelComboBox->setItemIcon(0, QKeyMapper::s_Icon_Blank);
}

int QTableSetupDialog::getSettingSelectIndex()
{
    return m_SettingSelectIndex;
}

bool QTableSetupDialog::isSelectImageFileDialogVisible()
{
    if (m_SelectImageFileDialog && m_SelectImageFileDialog->isVisible()) {
        return true;
    }
    else {
        return false;
    }
}

bool QTableSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            if (QKeyMapper::isSelectColorDialogVisible()
                || isSelectImageFileDialogVisible()) {
            }
            else {
                close();
            }
        }
    }
    return QDialog::event(event);
}

void QTableSetupDialog::closeEvent(QCloseEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QTableSetupDialog::closeEvent]" << "Tab Index initialize to -1";
#endif
    m_TabIndex = -1;
    m_SettingSelectIndex = -1;

    QDialog::closeEvent(event);
}

void QTableSetupDialog::showEvent(QShowEvent *event)
{
    if (0 <= m_TabIndex && m_TabIndex < QKeyMapper::s_KeyMappingTabInfoList.size()) {
        QString TabName = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName;
        QString TabHotkey = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabHotkey;
        QColor TabFontColor = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabFontColor;
        QString TabCustomImage_Path = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_Path;
        int TabCustomImage_ShowPosition = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_ShowPosition;
        int TabCustomImage_Padding = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_Padding;
        bool TabCustomImage_ShowAsTrayIcon = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_ShowAsTrayIcon;
        QSize TabCustomImage_TrayIconPixel = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_TrayIconPixel;

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote()
            << "[QTableSetupDialog::showEvent] "
            << "TabIndex[" << m_TabIndex << "] -> "
            << "TabName: " << TabName << ", "
            << "Hotkey: " << TabHotkey << ", "
            << "FontColor: " << TabFontColor.name() << ", "
            << "ShowPosition: " << TabCustomImage_ShowPosition << ", "
            << "Padding: " << TabCustomImage_Padding << ", "
            << "ShowAsTrayIcon: " << (TabCustomImage_ShowAsTrayIcon ? "true" : "false") << ", "
            << "TrayIconPixel: " << TabCustomImage_TrayIconPixel;
        qDebug().nospace().noquote()
            << "[QTableSetupDialog::showEvent] CustomImagePath: " << TabCustomImage_Path;
#endif

        // Load TabNamc String
        ui->tabNameLineEdit->setText(TabName);

        // Load TabHotkey String
        ui->tabHotkeyLineEdit->setText(TabHotkey);

        // Load TabFontColor
        if (TabFontColor.isValid() != true) {
            if (GLOBALSETTING_INDEX == m_SettingSelectIndex) {
                TabFontColor = NOTIFICATION_COLOR_GLOBAL_DEFAULT;
            }
            else {
                TabFontColor = NOTIFICATION_COLOR_NORMAL_DEFAULT;
            }
        }
        m_NotificationFontColorPicker->setColor(TabFontColor);

        // Load Custom Image
        QIcon icon_loaded;
        if (!TabCustomImage_Path.isEmpty()) {
            icon_loaded = QIcon(TabCustomImage_Path);
            if (!icon_loaded.isNull()) {
                QPixmap scaled_pixmap = icon_loaded.pixmap(QSize(TAB_CUSTOMIMAGE_WIDTH_DEFAULT, TAB_CUSTOMIMAGE_HEIGHT_DEFAULT));
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[QTableSetupDialog::showEvent]" << " Scaled(" << QSize(TAB_CUSTOMIMAGE_WIDTH_DEFAULT, TAB_CUSTOMIMAGE_HEIGHT_DEFAULT) << ") pixmap size: " << scaled_pixmap.size();
#endif
                ui->customImageLabel->setPixmap(scaled_pixmap);
                ui->customImageLabel->setToolTip(TabCustomImage_Path);
            }
            else {
                ui->customImageLabel->clear();
                ui->customImageLabel->setToolTip("");
            }
        }
        else {
            ui->customImageLabel->clear();
            ui->customImageLabel->setToolTip("");
        }

        // Load Custom Image Show Position
        int showposition_index = TAB_CUSTOMIMAGE_POSITION_DEFAULT;
        if (TabCustomImage_ShowPosition == TAB_CUSTOMIMAGE_SHOW_LEFT) {
            showposition_index = TAB_CUSTOMIMAGE_SHOW_LEFT;
        }
        else if (TabCustomImage_ShowPosition == TAB_CUSTOMIMAGE_SHOW_RIGHT) {
            showposition_index = TAB_CUSTOMIMAGE_SHOW_RIGHT;
        }
        ui->customImageShowPositionComboBox->setCurrentIndex(showposition_index);

        // Load Custom Image Padding
        int padding_value = TAB_CUSTOMIMAGE_PADDING_DEFAULT;
        if (TAB_CUSTOMIMAGE_PADDING_MIN <= TabCustomImage_Padding && TabCustomImage_Padding <= TAB_CUSTOMIMAGE_PADDING_MAX) {
            padding_value = TabCustomImage_Padding;
        }
        ui->customImagePaddingSpinBox->setValue(padding_value);

        // Load Custom Image Show As Tray Icon
        ui->customImageShowAsTrayIconCheckBox->setChecked(TabCustomImage_ShowAsTrayIcon);

        // Load Custom Image Tray Icon Pixel
        int trayicon_index = TAB_CUSTOMIMAGE_TRAYICON_PIXEL_DEFAULT;
        QList<QSize> iconsize_list = ICON_SIZE_MAP.values();
        if (iconsize_list.contains(TabCustomImage_TrayIconPixel)) {
            trayicon_index = ICON_SIZE_MAP.key(TabCustomImage_TrayIconPixel);
        }
        ui->customImageTrayIconPixelComboBox->setCurrentIndex(trayicon_index);

        // Update TrayIconPixelComboBox Item icon display
        updateTrayIconPixelComboBoxIcons(icon_loaded);
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
        m_SettingSelectIndex = -1;
    }

    QDialog::keyPressEvent(event);
}

void QTableSetupDialog::onTabFontColorChanged(QColor &color)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (color != QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabFontColor) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QTableSetupDialog::onTabFontColorChanged]" << " TabIndex[" << m_TabIndex << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName << "] Tab Font Color -> " << color.name();
#endif
        QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].TabFontColor = color;
        QKeyMapper::getInstance()->updateKeyMappingTabWidgetTabDisplay(m_TabIndex);
    }
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

void QTableSetupDialog::on_selectCustomImageButton_clicked()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    int tabindex = m_TabIndex;

    if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
        QKeyMapper::clearTabCustomImage(tabindex);
        ui->customImageLabel->clear();
        ui->customImageLabel->setToolTip("");
        QKeyMapper::getInstance()->updateKeyMappingTabWidgetTabDisplay(tabindex);
        updateTrayIconPixelComboBoxIcons(QIcon());
        return;
    }

    QString TabName = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName;
    QString currentTabCustomImage_Path = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_Path;
    QString filter = tr("Image files") + "(*.ico;*.png)";
    QString caption_string;
    caption_string = tr("Select Custom Image") + (" : ") +TabName;

    m_SelectImageFileDialog->setNameFilter(filter);
    m_SelectImageFileDialog->setWindowTitle(caption_string);
    // Set the initial directory and selected file based on the current tab's custom image path
    if (currentTabCustomImage_Path.isEmpty()) {
        m_SelectImageFileDialog->setDirectory(QDir());
        m_SelectImageFileDialog->selectFile("");
    }
    else {
        m_SelectImageFileDialog->setDirectory(QFileInfo(currentTabCustomImage_Path).absoluteDir());
        m_SelectImageFileDialog->selectFile(QFileInfo(currentTabCustomImage_Path).absoluteFilePath());
    }

    QString customimage_path;
    if (m_SelectImageFileDialog->exec() == QDialog::Accepted) {
        QStringList selected_files = m_SelectImageFileDialog->selectedFiles();
        if (!selected_files.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[on_selectCustomImageButton_clicked]" << "selected_files from QFileDialog -> " << selected_files;
#endif
            customimage_path = selected_files.first();
        }

    }

    if (customimage_path.isEmpty()) {
        return;
    }

    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    tabindex = m_TabIndex;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[on_selectCustomImageButton_clicked]" << "customimage_path from QFileDialog -> TabIndex[" << tabindex << "] : " << customimage_path;
#endif

    QIcon icon_loaded = QKeyMapper::setTabCustomImage(tabindex, customimage_path);

    if (!icon_loaded.isNull()) {
        QPixmap scaled_pixmap = icon_loaded.pixmap(QSize(TAB_CUSTOMIMAGE_WIDTH_DEFAULT, TAB_CUSTOMIMAGE_HEIGHT_DEFAULT));
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[QTableSetupDialog::showEvent]" << " Scaled(" << QSize(TAB_CUSTOMIMAGE_WIDTH_DEFAULT, TAB_CUSTOMIMAGE_HEIGHT_DEFAULT) << ") pixmap size: " << scaled_pixmap.size();
#endif
        ui->customImageLabel->setPixmap(scaled_pixmap);
        ui->customImageLabel->setToolTip(customimage_path);

        QKeyMapper::getInstance()->updateKeyMappingTabWidgetTabDisplay(tabindex);
    }
    else {
        QString popupMessage;
        QString popupMessageColor;
        int popupMessageDisplayTime = 3000;
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("Unable to load the image!");
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
    }

    updateTrayIconPixelComboBoxIcons(icon_loaded);
}

void QTableSetupDialog::on_customImageShowPositionComboBox_currentIndexChanged(int index)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[CustomImageShowPosition] Custom Image show position changed ->" << index;
#endif

    int tabindex = m_TabIndex;
    QKeyMapper::s_KeyMappingTabInfoList[tabindex].TabCustomImage_ShowPosition = index;
}

void QTableSetupDialog::on_customImagePaddingSpinBox_valueChanged(int value)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[CustomImagePadding] Custom Image Padding value changed ->" << value;
#endif

    int tabindex = m_TabIndex;
    QKeyMapper::s_KeyMappingTabInfoList[tabindex].TabCustomImage_Padding = value;
}

void QTableSetupDialog::on_customImageShowAsTrayIconCheckBox_stateChanged(int state)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[CustomImageShowAsTrayIcon] Show as TrayIcon state changed ->" << (Qt::CheckState)state;
#endif

    int tabindex = m_TabIndex;
    if (Qt::Checked == state) {
        QKeyMapper::s_KeyMappingTabInfoList[tabindex].TabCustomImage_ShowAsTrayIcon = true;
    }
    else {
        QKeyMapper::s_KeyMappingTabInfoList[tabindex].TabCustomImage_ShowAsTrayIcon = false;
    }
}

void QTableSetupDialog::on_customImageTrayIconPixelComboBox_currentIndexChanged(int index)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (!ICON_SIZE_MAP.contains(index)) {
        return;
    }

    QSize trayicon_size = ICON_SIZE_MAP.value(index);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[CustomImageTrayIconPixel] Custom Image TrayIcon pixel changed ->" << trayicon_size;
#endif

    int tabindex = m_TabIndex;
    QKeyMapper::s_KeyMappingTabInfoList[tabindex].TabCustomImage_TrayIconPixel = trayicon_size;
}
