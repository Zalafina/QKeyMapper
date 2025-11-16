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

    ui->addMacroButton->setText(tr("Add Macro"));
    ui->mapkeyLabel->setText(tr("MapKey"));
    ui->categoryLabel->setText(tr("Category"));
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
