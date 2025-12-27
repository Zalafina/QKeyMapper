#include "qkeymapper.h"
#include "qignorewindowinfolistdialog.h"
#include "ui_qignorewindowinfolistdialog.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QIgnoreWindowInfoListDialog *QIgnoreWindowInfoListDialog::m_instance = Q_NULLPTR;

QIgnoreWindowInfoListDialog::QIgnoreWindowInfoListDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QIgnoreWindowInfoListDialog)
{
    m_instance = this;
    ui->setupUi(this);

    QStringList windowinfoMatchList;
    windowinfoMatchList.append(tr("Ignore"));
    windowinfoMatchList.append(tr("Equals"));
    windowinfoMatchList.append(tr("Contains"));
    windowinfoMatchList.append(tr("StartsWith"));
    windowinfoMatchList.append(tr("EndsWith"));
    windowinfoMatchList.append(tr("RegexMatch"));
    ui->ruleProcessNameMatchTypeComboBox->addItems(windowinfoMatchList);
    ui->ruleWindowTitleMatchTypeComboBox->addItems(windowinfoMatchList);
    ui->ruleClassNameMatchTypeComboBox->addItems(windowinfoMatchList);
    ui->ruleProcessNameMatchTypeComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
    ui->ruleWindowTitleMatchTypeComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
    ui->ruleClassNameMatchTypeComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_IGNORE);

    ui->ruleListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // Keep uniform item height for consistent DPI scaling
    ui->ruleListWidget->setUniformItemSizes(true);

    // Alternate row colors for better readability
    // ui->ruleListWidget->setAlternatingRowColors(true);

    // Disable word wrapping to keep layout predictable
    ui->ruleListWidget->setWordWrap(false);
}

QIgnoreWindowInfoListDialog::~QIgnoreWindowInfoListDialog()
{
    delete ui;
}

void QIgnoreWindowInfoListDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Ignore Rules List"));

    ui->ruleNameLabel->setText(tr("Rule Name"));
    ui->ruleProcessNameLabel->setText(tr("Process"));
    ui->ruleWindowTitleLabel->setText(tr("Title"));
    ui->ruleClassNameLabel->setText(tr("Class"));
    ui->ruleDescriptionLabel->setText(tr("Description"));
    ui->ruleListLabel->setText(tr("Rules List"));
    ui->saveRuleButton->setText(tr("Add"));
    ui->clearRuleButton->setText(tr("Clear"));
    ui->ruleDisabledCheckBox->setText(tr("Disabled"));
    ui->deleteRuleButton->setText(tr("Delete"));

    ui->ruleProcessNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_IGNORE,        tr("Ignore"));
    ui->ruleProcessNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_EQUALS,        tr("Equals"));
    ui->ruleProcessNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_CONTAINS,      tr("Contains"));
    ui->ruleProcessNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_STARTSWITH,    tr("StartsWith"));
    ui->ruleProcessNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_ENDSWITH,      tr("EndsWith"));
    ui->ruleProcessNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_REGEXMATCH,    tr("RegexMatch"));
    ui->ruleWindowTitleMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_IGNORE,        tr("Ignore"));
    ui->ruleWindowTitleMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_EQUALS,        tr("Equals"));
    ui->ruleWindowTitleMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_CONTAINS,      tr("Contains"));
    ui->ruleWindowTitleMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_STARTSWITH,    tr("StartsWith"));
    ui->ruleWindowTitleMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_ENDSWITH,      tr("EndsWith"));
    ui->ruleWindowTitleMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_REGEXMATCH,    tr("RegexMatch"));
    ui->ruleClassNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_IGNORE,          tr("Ignore"));
    ui->ruleClassNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_EQUALS,          tr("Equals"));
    ui->ruleClassNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_CONTAINS,        tr("Contains"));
    ui->ruleClassNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_STARTSWITH,      tr("StartsWith"));
    ui->ruleClassNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_ENDSWITH,        tr("EndsWith"));
    ui->ruleClassNameMatchTypeComboBox->setItemText(WINDOWINFO_MATCH_INDEX_REGEXMATCH,      tr("RegexMatch"));
}

void QIgnoreWindowInfoListDialog::updateRulesListWidget()
{
    // Clear existing items
    ui->ruleListWidget->clear();

    // Populate list widget with rule names from the map
    for (const IgnoreWindowInfo &info : std::as_const(QKeyMapper::s_IgnoreWindowInfoMap)) {
        QListWidgetItem *item = new QListWidgetItem(info.ruleName);

        // Visual indication for disabled rules
        if (info.disabled) {
            QFont font = item->font();
            font.setItalic(true);
            item->setFont(font);
            item->setForeground(Qt::gray);
        }
        item->setSizeHint(QSize(0, IGNOREWINDOWINFOLIST_ITEM_HEIGHT));

        ui->ruleListWidget->addItem(item);
    }

    updateSaveRuleButtonText();
}

void QIgnoreWindowInfoListDialog::showEvent(QShowEvent *event)
{
    updateRulesListWidget();

    QDialog::showEvent(event);
}

#if 0
bool QIgnoreWindowInfoListDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}
#endif

void QIgnoreWindowInfoListDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

void QIgnoreWindowInfoListDialog::on_saveRuleButton_clicked()
{
    // Get and validate rule name
    QString ruleName = ui->ruleNameLineEdit->text().trimmed();

    if (ruleName.isEmpty()) {
        QString popupMessage = tr("Rule name cannot be empty.");
        QString popupMessageColor = FAILURE_COLOR;
        int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    // Create IgnoreWindowInfo structure from UI
    IgnoreWindowInfo info;
    info.ruleName = ruleName;
    info.processName = ui->ruleProcessNameLineEdit->text();
    info.windowTitle = ui->ruleWindowTitleLineEdit->text();
    info.className = ui->ruleClassNameLineEdit->text();
    info.description = ui->ruleDescriptionLineEdit->text();
    info.processNameMatchType = static_cast<QKeyMapperConstants::WindowInfoMatchType>(ui->ruleProcessNameMatchTypeComboBox->currentIndex());
    info.windowTitleMatchType = static_cast<QKeyMapperConstants::WindowInfoMatchType>(ui->ruleWindowTitleMatchTypeComboBox->currentIndex());
    info.classNameMatchType = static_cast<QKeyMapperConstants::WindowInfoMatchType>(ui->ruleClassNameMatchTypeComboBox->currentIndex());
    info.disabled = ui->ruleDisabledCheckBox->isChecked();

    // Check if rule already exists (update vs add)
    bool isUpdate = QKeyMapper::s_IgnoreWindowInfoMap.contains(ruleName);

    // Add or update rule in map
    QKeyMapper::s_IgnoreWindowInfoMap[ruleName] = info;

    // Refresh list widget
    updateRulesListWidget();

    // Show success message
    QString popupMessage;
    if (isUpdate) {
        popupMessage = tr("Rule \"%1\" updated successfully").arg(ruleName);
    } else {
        popupMessage = tr("Rule \"%1\" added successfully").arg(ruleName);
    }
    QString popupMessageColor = SUCCESS_COLOR;
    int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);

    // Clear input fields after successful save
    // initRuleWindowInfoArea();
}


void QIgnoreWindowInfoListDialog::on_clearRuleButton_clicked()
{
    QString ruleName = ui->ruleNameLineEdit->text();
    QString ruleProcessName = ui->ruleProcessNameLineEdit->text();
    QString ruleWindowTitle = ui->ruleWindowTitleLineEdit->text();
    QString ruleClassName = ui->ruleClassNameLineEdit->text();
    QString ruleDescription = ui->ruleDescriptionLineEdit->text();
    bool ruleDisabled = ui->ruleDisabledCheckBox->isChecked();

    if (!ruleName.isEmpty()
        || !ruleProcessName.isEmpty()
        || !ruleWindowTitle.isEmpty()
        || !ruleClassName.isEmpty()
        || !ruleDescription.isEmpty()
        || ruleDisabled) {
        QString message = tr("Are you sure you want to clear the rule information fields?");
        QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message,
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            // User cancelled, don't clear
            return;
        }

        initRuleWindowInfoArea();
    }
}


void QIgnoreWindowInfoListDialog::on_deleteRuleButton_clicked()
{
    // Get selected item
    QListWidgetItem *selectedItem = ui->ruleListWidget->currentItem();

    if (!selectedItem) {
        // No item selected, do nothing
        return;
    }

    QString ruleName = selectedItem->text();

    // Show confirmation dialog
    QString message = tr("Are you sure you want to delete rule \"%1\"?").arg(ruleName);
    QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message,
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        // User cancelled deletion
        return;
    }

    // Remove rule from map
    QKeyMapper::s_IgnoreWindowInfoMap.remove(ruleName);

    // Refresh list widget
    updateRulesListWidget();

    // Show success message
    QString popupMessage = tr("Rule \"%1\" deleted successfully").arg(ruleName);
    QString popupMessageColor = SUCCESS_COLOR;
    int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);

    // Clear input fields if deleted rule was being displayed
    if (ui->ruleNameLineEdit->text().trimmed() == ruleName) {
        initRuleWindowInfoArea();
    }
}


void QIgnoreWindowInfoListDialog::on_ruleNameLineEdit_textChanged(const QString &text)
{
    Q_UNUSED(text);

    updateSaveRuleButtonText();
}

void QIgnoreWindowInfoListDialog::on_ruleListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    QString ruleName = item->text();
    loadRuleToUI(ruleName);
}

void QIgnoreWindowInfoListDialog::loadRuleToUI(const QString &ruleName)
{
    // Find rule in map
    if (!QKeyMapper::s_IgnoreWindowInfoMap.contains(ruleName)) {
        return;
    }

    const IgnoreWindowInfo &info = QKeyMapper::s_IgnoreWindowInfoMap[ruleName];

    // Load rule data to UI controls
    ui->ruleNameLineEdit->setText(info.ruleName);
    ui->ruleProcessNameLineEdit->setText(info.processName);
    ui->ruleWindowTitleLineEdit->setText(info.windowTitle);
    ui->ruleClassNameLineEdit->setText(info.className);
    ui->ruleDescriptionLineEdit->setText(info.description);
    ui->ruleProcessNameMatchTypeComboBox->setCurrentIndex(static_cast<int>(info.processNameMatchType));
    ui->ruleWindowTitleMatchTypeComboBox->setCurrentIndex(static_cast<int>(info.windowTitleMatchType));
    ui->ruleClassNameMatchTypeComboBox->setCurrentIndex(static_cast<int>(info.classNameMatchType));
    ui->ruleDisabledCheckBox->setChecked(info.disabled);
}

void QIgnoreWindowInfoListDialog::updateSaveRuleButtonText()
{
    QString ruleName = ui->ruleNameLineEdit->text().trimmed();

    // Check if rule name already exists in the map
    if (QKeyMapper::s_IgnoreWindowInfoMap.keys().contains(ruleName) && !ruleName.isEmpty()) {
        // Rule exists, button should show "Update"
        ui->saveRuleButton->setText(tr("Update"));
    } else {
        // Rule doesn't exist, button should show "Add"
        ui->saveRuleButton->setText(tr("Add"));
    }
}

void QIgnoreWindowInfoListDialog::initRuleWindowInfoArea()
{
    ui->ruleProcessNameMatchTypeComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
    ui->ruleWindowTitleMatchTypeComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_DEFAULT);
    ui->ruleClassNameMatchTypeComboBox->setCurrentIndex(WINDOWINFO_MATCH_INDEX_IGNORE);

    ui->ruleNameLineEdit->clear();
    ui->ruleProcessNameLineEdit->clear();
    ui->ruleWindowTitleLineEdit->clear();
    ui->ruleClassNameLineEdit->clear();
    ui->ruleDescriptionLineEdit->clear();
    ui->ruleDisabledCheckBox->setChecked(false);
}
