#include "qkeymapper.h"
#include "qtablesetupdialog.h"
#include "ui_qtablesetupdialog.h"

using namespace QKeyMapperConstants;

QTableSetupDialog *QTableSetupDialog::m_instance = Q_NULLPTR;

QTableSetupDialog::QTableSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTableSetupDialog)
    , m_TabIndex(-1)
    , m_SettingSelectIndex(-1)
    , m_NotificationFontColorPicker(new ColorPickerWidget(this, "TabFontColor", COLORPICKER_BUTTON_WIDTH_TABFONTCOLOR))
    , m_NotificationBackgroundColorPicker(new ColorPickerWidget(this, "TabBGColor", COLORPICKER_BUTTON_WIDTH_TABBGCOLOR))
{
    m_instance = this;
    ui->setupUi(this);

    m_FloatingWindowSetupDialog = new QFloatingWindowSetupDialog(this);

    initSelectImageFileDialog();

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->tabCustomImageGroupBox->setStyle(windowsStyle);
        ui->customImageLabel->setStyle(windowsStyle);
    }

    ui->hideNotificationCheckBox->setTristate(true);

    ui->tabNameLineEdit->setFocusPolicy(Qt::ClickFocus);
    ui->tabHotkeyLineEdit->setFocusPolicy(Qt::ClickFocus);

    // ui->tabNameLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));
    // ui->tabHotkeyLineEdit->setFont(QFont(FONTNAME_ENGLISH, 9));

    ui->customImagePaddingSpinBox->setRange(TAB_CUSTOMIMAGE_PADDING_MIN, TAB_CUSTOMIMAGE_PADDING_MAX);
    ui->customImagePaddingSpinBox->setValue(TAB_CUSTOMIMAGE_PADDING_DEFAULT);

    int x_offset;
    int y_offset;
    QRect exportTableButtonGeometry = ui->exportTableButton->geometry();

    x_offset = -50;
    y_offset = 30;
    int tabfont_color_x = exportTableButtonGeometry.x();
    int tabfont_color_y = exportTableButtonGeometry.y();
    tabfont_color_x += x_offset;
    tabfont_color_y += y_offset;
    // Set position for the tabfont color picker
    m_NotificationFontColorPicker->move(tabfont_color_x, tabfont_color_y);
    m_NotificationFontColorPicker->raise();

    x_offset = 160;
    y_offset = 0;
    int tabbg_color_x = tabfont_color_x;
    int tabbg_color_y = tabfont_color_y;
    tabbg_color_x += x_offset;
    tabbg_color_y += y_offset;
    // Set position for the tabbackground color picker
    m_NotificationBackgroundColorPicker->move(tabbg_color_x, tabbg_color_y);
    m_NotificationBackgroundColorPicker->raise();
    m_NotificationBackgroundColorPicker->setShowAlphaChannel(true);

    QStringList showPositionList;
    showPositionList.append(tr("None"));
    showPositionList.append(tr("Left"));
    showPositionList.append(tr("Right"));
    ui->customImageShowPositionComboBox->addItems(showPositionList);
    ui->customImageShowPositionComboBox->setCurrentIndex(TAB_CUSTOMIMAGE_POSITION_DEFAULT);

    clearTrayIconPixelComboBox();

    QObject::connect(ui->tabNameLineEdit, &QLineEdit::returnPressed, this, &QTableSetupDialog::on_tabNameUpdateButton_clicked);
    QObject::connect(ui->tabHotkeyLineEdit, &QLineEdit::returnPressed, this, &QTableSetupDialog::on_tabHotkeyUpdateButton_clicked);
    QObject::connect(m_NotificationFontColorPicker, &ColorPickerWidget::colorChanged, this, &QTableSetupDialog::onTabFontColorChanged);
    QObject::connect(m_NotificationBackgroundColorPicker, &ColorPickerWidget::colorChanged, this, &QTableSetupDialog::onTabBackgroundColorChanged);
}

QTableSetupDialog::~QTableSetupDialog()
{
    delete ui;
}

void QTableSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);

    m_NotificationFontColorPicker->setButtonText(tr("TabFontColor"));
    m_NotificationFontColorPicker->setWindowTitle(tr("Select Tab Notification Font Color"));
    m_NotificationBackgroundColorPicker->setButtonText(tr("TabBGColor"));
    m_NotificationBackgroundColorPicker->setWindowTitle(tr("Select Tab Notification Background Color"));

    setWindowTitle(tr(TABLESETUPDIALOG_WINDOWTITLE_STR));
    ui->tabNameLabel->setText(tr(TABNAMELABEL_STR));
    ui->tabHotkeyLabel->setText(tr(TABHOTKEYLABEL_STR));
    ui->tabNameUpdateButton->setText(tr(UPDATEBUTTON_STR));
    ui->tabHotkeyUpdateButton->setText(tr(UPDATEBUTTON_STR));
    ui->exportTableButton->setText(tr(EXPORTTABLEBUTTON_STR));
    ui->importTableButton->setText(tr(IMPORTTABLEBUTTON_STR));
    ui->removeTableButton->setText(tr(REMOVETABLEBUTTON_STR));
    ui->hideNotificationCheckBox->setText(tr("Hide Notification"));

    ui->tabCustomImageGroupBox->setTitle(tr("Tab Custom Image"));
    ui->selectCustomImageButton->setText(tr("Select Custom Image"));
    ui->customImageShowPositionLabel->setText(tr("ShowPositoin"));
    ui->customImagePaddingLabel->setText(tr("Padding"));
    ui->customImageShowAsTrayIconCheckBox->setText(tr("Show as TrayIcon"));
    ui->customImageShowAsFloatingWindowCheckBox->setText(tr("Show as FloatingWindow"));
    ui->floatingWindowSetupButton->setText(tr("FloatingWindow Setup"));

    ui->customImageShowPositionComboBox->setItemText(TAB_CUSTOMIMAGE_SHOW_NONE,     tr("None"));
    ui->customImageShowPositionComboBox->setItemText(TAB_CUSTOMIMAGE_SHOW_LEFT,     tr("Left"));
    ui->customImageShowPositionComboBox->setItemText(TAB_CUSTOMIMAGE_SHOW_RIGHT,    tr("Right"));

    ui->customImageTrayIconPixelLabel->setText(tr("TrayIcon Pixel"));
    ui->customImageTrayIconPixelComboBox->setItemText(TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT, tr("Default"));

    if (m_FloatingWindowSetupDialog != Q_NULLPTR) {
        m_FloatingWindowSetupDialog->setUILanguage(languageindex);
    }
}

void QTableSetupDialog::resetFontSize()
{
    QFont customFont(FONTNAME_ENGLISH, 9);

    ui->tabNameLabel->setFont(customFont);
    ui->tabHotkeyLabel->setFont(customFont);
    ui->tabNameLineEdit->setFont(customFont);
    ui->tabHotkeyLineEdit->setFont(customFont);
    ui->tabNameUpdateButton->setFont(customFont);
    ui->tabHotkeyUpdateButton->setFont(customFont);
    ui->exportTableButton->setFont(customFont);
    ui->importTableButton->setFont(customFont);
    ui->removeTableButton->setFont(customFont);
    ui->hideNotificationCheckBox->setFont(customFont);

    if (m_FloatingWindowSetupDialog != Q_NULLPTR) {
        m_FloatingWindowSetupDialog->resetFontSize();
    }
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

void QTableSetupDialog::clearTrayIconPixelComboBox()
{
    ui->customImageTrayIconPixelComboBox->blockSignals(true);
    ui->customImageTrayIconPixelComboBox->clear();
    ui->customImageTrayIconPixelComboBox->addItem(QKeyMapper::s_Icon_Blank, tr("Default"));
    ui->customImageTrayIconPixelComboBox->setCurrentIndex(TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT);
    ui->customImageTrayIconPixelComboBox->blockSignals(false);
}

QSize QTableSetupDialog::parseComboBoxTextToSize(const QString &text)
{
    // Parse text like "16×16" or "32×32" to QSize
    // Return QSize() (invalid) if parsing fails or values are not positive
    QStringList sizeParts = text.split(ICON_SIZE_SEPARATORT);
    if (sizeParts.size() != 2) {
        return QSize(); // Invalid format
    }

    bool ok1, ok2;
    int width = sizeParts[0].toInt(&ok1);
    int height = sizeParts[1].toInt(&ok2);

    if (ok1 && ok2 && width > 0 && height > 0) {
        return QSize(width, height);
    }

    return QSize(); // Invalid size
}

void QTableSetupDialog::updateTrayIconPixelSizeWithCurrentText()
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    QString currentText = ui->customImageTrayIconPixelComboBox->currentText();
    QSize trayicon_size = parseComboBoxTextToSize(currentText);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[updateTrayIconPixelSizeWithCurrentText] Custom Image TrayIcon pixel currentText:" << currentText << ", converted Size: " << trayicon_size;
#endif

    if (trayicon_size.isEmpty()) {
        if (ui->customImageTrayIconPixelComboBox->currentIndex() != TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT) {
            ui->customImageTrayIconPixelComboBox->blockSignals(true);
            ui->customImageTrayIconPixelComboBox->setCurrentIndex(TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT);
            ui->customImageTrayIconPixelComboBox->blockSignals(false);
        }
    }

    int tabindex = m_TabIndex;
    QKeyMapper::s_KeyMappingTabInfoList[tabindex].TabCustomImage_TrayIconPixel = trayicon_size;
}

void QTableSetupDialog::updateTrayIconPixelComboBox(const QIcon &icon, const QSize &targetSize)
{
    // 1. If the icon is null, clear all item icons
    if (icon.isNull()) {
        clearTrayIconPixelComboBox();
        return;
    }

    // 2. Get available icon sizes & sort in descending order
    QList<QSize> availableSizes = icon.availableSizes();
    if (availableSizes.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[QTableSetupDialog::updateTrayIconPixelComboBox]" << "Icon.availableSizes() is empty. Showing default icon only.";
#endif
        clearTrayIconPixelComboBox();
        ui->customImageTrayIconPixelComboBox->setItemIcon(TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT, icon);
        return;
    }

    std::sort(availableSizes.begin(), availableSizes.end(), [](const QSize &a, const QSize &b) {
        if (a.width() != b.width())
            return a.width() > b.width();   // Primary: sort by width in descending order
        return a.height() > b.height();     // Secondary: sort by height in descending order
    });

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[QTableSetupDialog::updateTrayIconPixelComboBox]" << "Icon availableSizes sorted:" << availableSizes;
#endif

    // 3. Save current selection before clearing if it's not the default
    QSize savedCurrentSize;
    if (ui->customImageTrayIconPixelComboBox->currentIndex() != TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT) {
        QString currentText = ui->customImageTrayIconPixelComboBox->currentText();
        savedCurrentSize = parseComboBoxTextToSize(currentText);
    }

    // 4. Clear TrayIconPixelComboBox & set "Default" item (index 0) uses the original icon
    clearTrayIconPixelComboBox();
    ui->customImageTrayIconPixelComboBox->setItemIcon(TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT, icon);

    // 5. Add available sizes as new items with corresponding icons
    for (const QSize &size : std::as_const(availableSizes)) {
        QString itemText = QString("%1%2%3").arg(size.width()).arg(ICON_SIZE_SEPARATORT).arg(size.height());

        // Extract icon of corresponding size and create item
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        QPixmap scaled_pixmap = icon.pixmap(size, 1.0);
#else
        qreal devicePixelRatio = qApp->devicePixelRatio();
        QSize deviceIndependentSize = QSize(
            qRound(size.width() / devicePixelRatio),
            qRound(size.height() / devicePixelRatio)
        );
        QPixmap scaled_pixmap = icon.pixmap(deviceIndependentSize);
#endif

        QIcon itemIcon(scaled_pixmap);
        ui->customImageTrayIconPixelComboBox->addItem(itemIcon, itemText);

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "[QTableSetupDialog::updateTrayIconPixelComboBox] Added item: " << itemText << " at index " << (ui->customImageTrayIconPixelComboBox->count() - 1);
#endif
    }

    // 6. Try to set index based on targetSize first
    int targetIndex = TAB_CUSTOMIMAGE_TRAYICON_PIXEL_INDEX_DEFAULT;
    bool indexSet = false;

    if (!targetSize.isEmpty()) {
        // Search for matching targetSize in available sizes
        for (int i = 0; i < availableSizes.size(); ++i) {
            if (availableSizes[i] == targetSize) {
                targetIndex = i + 1; // +1 because index 0 is "Default"
                indexSet = true;
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[QTableSetupDialog::updateTrayIconPixelComboBox] Found targetSize match at index " << targetIndex << " for size " << targetSize;
#endif
                break;
            }
        }
    }

    // 7. If targetSize didn't work, try saved current size
    if (!indexSet && !savedCurrentSize.isEmpty()) {
        for (int i = 0; i < availableSizes.size(); ++i) {
            if (availableSizes[i] == savedCurrentSize) {
                targetIndex = i + 1; // +1 because index 0 is "Default"
                indexSet = true;
#ifdef DEBUG_LOGOUT_ON
                qDebug().nospace() << "[QTableSetupDialog::updateTrayIconPixelComboBox] Restored saved size at index " << targetIndex << " for size " << savedCurrentSize;
#endif
                break;
            }
        }
    }

    Q_UNUSED(indexSet);

    // 8. Set the final index (default if nothing matched)
    ui->customImageTrayIconPixelComboBox->blockSignals(true);
    ui->customImageTrayIconPixelComboBox->setCurrentIndex(targetIndex);
    ui->customImageTrayIconPixelComboBox->blockSignals(false);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[QTableSetupDialog::updateTrayIconPixelComboBox] Final index set to " << targetIndex << " (total items: " << ui->customImageTrayIconPixelComboBox->count() << ")";
#endif
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

bool QTableSetupDialog::isFloatingWindowSetupDialogVisible()
{
    if (m_FloatingWindowSetupDialog && m_FloatingWindowSetupDialog->isVisible()) {
        return true;
    }
    else {
        return false;
    }
}

#ifndef CLOSE_SETUPDIALOG_ONDATACHANGED
bool QTableSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            if (QKeyMapper::isSelectColorDialogVisible()
                || isSelectImageFileDialogVisible()
                || isFloatingWindowSetupDialogVisible()) {
            }
            else {
                close();
            }
        }
    }
    return QDialog::event(event);
}
#endif

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
        QColor TabBackgroundColor = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabBackgroundColor;
        Qt::CheckState TabHideNotification = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabHideNotification;
        QString TabCustomImage_Path = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_Path;
        int TabCustomImage_ShowPosition = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_ShowPosition;
        int TabCustomImage_Padding = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_Padding;
        bool TabCustomImage_ShowAsTrayIcon = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_ShowAsTrayIcon;
        bool TabCustomImage_ShowAsFloatingWindow = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_ShowAsFloatingWindow;
        QSize TabCustomImage_TrayIconPixel = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_TrayIconPixel;

#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote()
            << "[QTableSetupDialog::showEvent] "
            << "TabIndex[" << m_TabIndex << "] -> "
            << "TabName: " << TabName << ", "
            << "Hotkey: " << TabHotkey << ", "
            << "FontColor: " << TabFontColor.name() << ", "
            << "BackgroundColor: (" << TabBackgroundColor.name(QColor::HexArgb) << ", Alpha: " << TabBackgroundColor.alpha() << "), "
            << "HideNotification: " << TabHideNotification << ","
            << "ShowPosition: " << TabCustomImage_ShowPosition << ", "
            << "Padding: " << TabCustomImage_Padding << ", "
            << "ShowAsTrayIcon: " << (TabCustomImage_ShowAsTrayIcon ? "true" : "false") << ", "
            << "ShowAsFloatingWindow: " << (TabCustomImage_ShowAsFloatingWindow ? "true" : "false") << ", "
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

        // Load TabBackgroundColor
        if (TabBackgroundColor.isValid() != true) {
            TabBackgroundColor = NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
        }
        m_NotificationBackgroundColorPicker->setColor(TabBackgroundColor);

        // Load TabHideNotification
        ui->hideNotificationCheckBox->setCheckState(TabHideNotification);

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

        // Update TrayIconPixelComboBox Items & Index
        updateTrayIconPixelComboBox(icon_loaded, TabCustomImage_TrayIconPixel);
        updateTrayIconPixelSizeWithCurrentText();

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

        // Load Custom Image Show As Floating Window
        ui->customImageShowAsFloatingWindowCheckBox->setChecked(TabCustomImage_ShowAsFloatingWindow);
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

void QTableSetupDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
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
        // QKeyMapper::getInstance()->updateKeyMappingTabWidgetTabDisplay(m_TabIndex);
    }
}

void QTableSetupDialog::onTabBackgroundColorChanged(QColor &color)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    if (color != QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabBackgroundColor) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote()
            << "[QTableSetupDialog::onTabBackgroundColorChanged]" << " TabIndex[" << m_TabIndex << "]["<< QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName << "] Tab Background Color -> " << color.name(QColor::HexArgb)
            << ", Alpha: " << color.alpha();
#endif
        QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].TabBackgroundColor = color;
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

    static QRegularExpression whitespace_reg(R"(\s+)");
    QString ori_tabhotkeystring = ui->tabHotkeyLineEdit->text().simplified();
    ori_tabhotkeystring.remove(whitespace_reg);
    QString tabhotkeystring = ori_tabhotkeystring;

    // Extract the hotkey using REGEX_PATTERN_TABHOTKEY
    static QRegularExpression tabhotkey_regex(REGEX_PATTERN_TABHOTKEY);
    QRegularExpressionMatch tabhotkey_match = tabhotkey_regex.match(tabhotkeystring);
    if (tabhotkey_match.hasMatch()) {
        tabhotkeystring = tabhotkey_match.captured(3); // Extract the actual hotkey part
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
    else if (ori_tabhotkeystring == QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabHotkey) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[on_tabHotkeyUpdateButton_clicked]" << "TabHotkey was not modified, no action required.";
#endif
    }
    else
    {
        ValidationResult validationResult = QKeyMapper::validateCombinationKey(tabhotkeystring);
        if (validationResult.isValid)
        {
            popupMessageColor = SUCCESS_COLOR;
            popupMessage = tr("TabHotkey update success: ") + ori_tabhotkeystring;

            QKeyMapper::getInstance()->updateKeyMappingTabInfoHotkey(m_TabIndex, ori_tabhotkeystring);
        }
        else
        {
            popupMessageColor = FAILURE_COLOR;
            popupMessage = tr("Invalid TabHotkey: %1").arg(validationResult.errorMessage);
        }
    }
    if (!popupMessage.isEmpty()) {
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
    }

    ui->tabHotkeyLineEdit->setText(QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabHotkey);
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

    int auto_disabled = 0;
    bool import_result = QKeyMapper::importKeyMappingDataFromFile(tabindex, import_filename, &auto_disabled);

    if (import_result) {
        QKeyMapper::getInstance()->refreshKeyMappingDataTableByTabIndex(tabindex);

        //Show success popup message
        QString popupMessage;
        QString popupMessageColor;
        int popupMessageDisplayTime = 3000;
        if (auto_disabled > 0) {
            popupMessageColor = WARNING_COLOR;
            popupMessage = tr("Import mapping data to table \"%1\" successfully. %2 mapping(s) were disabled due to a conflict.")
                              .arg(TabName)
                              .arg(auto_disabled);
        }
        else {
            popupMessageColor = SUCCESS_COLOR;
            popupMessage = tr("Import mapping data to table \"%1\" successfully").arg(TabName);
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
        updateTrayIconPixelComboBox(QIcon());
        updateTrayIconPixelSizeWithCurrentText();
        return;
    }

    QString TabName = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabName;
    QString currentTabCustomImage_Path = QKeyMapper::s_KeyMappingTabInfoList.at(m_TabIndex).TabCustomImage_Path;
    QString filter = tr("Image files") + "(*.ico;*.png;*.svg)";
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

    updateTrayIconPixelComboBox(icon_loaded);
    updateTrayIconPixelSizeWithCurrentText();
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
    Q_UNUSED(index);
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

    updateTrayIconPixelSizeWithCurrentText();
}

void QTableSetupDialog::on_customImageShowAsFloatingWindowCheckBox_stateChanged(int state)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[CustomImageShowAsFloatingWindow] Show as FloatingWindow state changed ->" << (Qt::CheckState)state;
#endif

    int tabindex = m_TabIndex;
    if (Qt::Checked == state) {
        QKeyMapper::s_KeyMappingTabInfoList[tabindex].TabCustomImage_ShowAsFloatingWindow = true;
    }
    else {
        QKeyMapper::s_KeyMappingTabInfoList[tabindex].TabCustomImage_ShowAsFloatingWindow = false;
    }
}

void QTableSetupDialog::on_floatingWindowSetupButton_clicked()
{
    if (Q_NULLPTR == m_FloatingWindowSetupDialog) {
        return;
    }
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[on_floatingWindowSetupButton_clicked] Show Floating Window Setup Dialog";
#endif

    if (!m_FloatingWindowSetupDialog->isVisible()) {
        m_FloatingWindowSetupDialog->setTabIndex(m_TabIndex);
        m_FloatingWindowSetupDialog->show();
    }
}

void QTableSetupDialog::on_hideNotificationCheckBox_stateChanged(int state)
{
    if (m_TabIndex < 0 || m_TabIndex >= QKeyMapper::s_KeyMappingTabInfoList.size()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[HideNotification] Hide Notification state changed ->" << static_cast<Qt::CheckState>(state);
#endif

    QKeyMapper::s_KeyMappingTabInfoList[m_TabIndex].TabHideNotification = static_cast<Qt::CheckState>(state);
}
