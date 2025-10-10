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
    ui->ruleListWidget->setAlternatingRowColors(true);

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

}

void QIgnoreWindowInfoListDialog::showEvent(QShowEvent *event)
{
    updateRulesListWidget();
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

}


void QIgnoreWindowInfoListDialog::on_ruleNameLineEdit_textChanged(const QString &text)
{

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
