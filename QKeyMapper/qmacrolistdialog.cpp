#include "qmacrolistdialog.h"
#include "ui_qmacrolistdialog.h"

QMacroListDialog *QMacroListDialog::m_instance = Q_NULLPTR;

QMacroListDialog::QMacroListDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QMacroListDialog)
{
    m_instance = this;
    ui->setupUi(this);
}

QMacroListDialog::~QMacroListDialog()
{
    delete ui;
}

void QMacroListDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Mapping Macro List"));

    ui->macroNameLabel->setText(tr("Name"));
    ui->catetoryLabel->setText(tr("Category"));
    ui->macroContentLabel->setText(tr("Macro"));
    ui->addMacroButton->setText(tr("Add Macro"));
    ui->mapkeyLabel->setText(tr("MapKeys"));
    ui->categoryFilterLabel->setText(tr("Filter"));

    QTabWidget *tabWidget = ui->macroListTabWidget;
    tabWidget->setTabText(tabWidget->indexOf(ui->macrolist),            tr("Macro")          );
    tabWidget->setTabText(tabWidget->indexOf(ui->universalmacrolist),   tr("Universal Macro"));
}

void QMacroListDialog::refreshMacroListTabWidget()
{

}

void QMacroListDialog::showEvent(QShowEvent *event)
{
    refreshMacroListTabWidget();

    QDialog::showEvent(event);
}

void MacroListTabWidget::keyPressEvent(QKeyEvent *event)
{

    QTabWidget::keyPressEvent(event);
}

void MacroListDataTableWidget::setCategoryFilter(const QString &category)
{

}

void MacroListDataTableWidget::clearCategoryFilter()
{

}

QStringList MacroListDataTableWidget::getAvailableCategories() const
{
    QStringList categories;

    return categories;
}

void MacroListDataTableWidget::startDrag(Qt::DropActions supportedActions)
{

    QTableWidget::startDrag(supportedActions);
}

void MacroListDataTableWidget::dropEvent(QDropEvent *event)
{

}

void MacroListDataTableWidget::dragEnterEvent(QDragEnterEvent *event)
{

}

void MacroListDataTableWidget::dragMoveEvent(QDragMoveEvent *event)
{

}

void MacroListDataTableWidget::updateRowVisibility()
{

}
