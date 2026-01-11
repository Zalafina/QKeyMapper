#include "qmacrolistdialog.h"
#include "ui_qmacrolistdialog.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

namespace {

class MacroCategoryFilterPanelWidget final : public QWidget
{
public:
    using QWidget::QWidget;

    QSize sizeHint() const override
    {
        const QSize s = size();
        return (s.isValid() && s.width() > 0 && s.height() > 0) ? s : QWidget::sizeHint();
    }

    QSize minimumSizeHint() const override
    {
        return sizeHint();
    }
};

static void refreshMenuWidgetActionGeometry(QMenu *menu, QWidgetAction *action, QWidget *panel)
{
    if (!menu || !action || !panel) {
        return;
    }

    // Important: Do NOT call setDefaultWidget(nullptr) here.
    // QWidgetAction may take ownership of defaultWidget; clearing it can delete the panel,
    // making the caller's `panel` pointer dangling and causing a crash.
    if (action->defaultWidget() != panel) {
        action->setDefaultWidget(panel);
    }

    // Force QMenu to recalculate action geometry/sizeHint.
    menu->removeAction(action);
    menu->addAction(action);

    menu->updateGeometry();
    menu->ensurePolished();
    menu->adjustSize();
}

} // namespace

QMacroListDialog *QMacroListDialog::m_instance = Q_NULLPTR;
OrderedMap<QString, MappingMacroData> QMacroListDialog::s_CopiedMacroData;

QMacroListDialog::QMacroListDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QMacroListDialog)
{
    m_instance = this;
    ui->setupUi(this);

    initKeyListComboBoxes();

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->mapList_SelectKeyboardButton->setStyle(windowsStyle);
        ui->mapList_SelectMouseButton->setStyle(windowsStyle);
        ui->mapList_SelectGamepadButton->setStyle(windowsStyle);
        ui->mapList_SelectFunctionButton->setStyle(windowsStyle);
    }
    ui->mapList_SelectKeyboardButton->setIcon(QIcon(":/keyboard.svg"));
    ui->mapList_SelectMouseButton->setIcon(QIcon(":/mouse.svg"));
    ui->mapList_SelectGamepadButton->setIcon(QIcon(":/gamepad.svg"));
    ui->mapList_SelectFunctionButton->setIcon(QIcon(":/function.svg"));
    ui->mapList_SelectKeyboardButton->setChecked(true);
    ui->mapList_SelectMouseButton->setChecked(true);
    ui->mapList_SelectGamepadButton->setChecked(true);
    ui->mapList_SelectFunctionButton->setChecked(true);

    QFont customFont(FONTNAME_ENGLISH, 9);
    ui->macroListTabWidget->setFont(customFont);
    ui->macroNameLabel->setFont(customFont);
    ui->catetoryLabel->setFont(customFont);
    ui->macroContentLabel->setFont(customFont);
    ui->macroNoteLabel->setFont(customFont);
    ui->mapkeyLabel->setFont(customFont);
    ui->categoryFilterLabel->setFont(customFont);
    ui->MacroList_MappingKeyListComboBox->setFont(customFont);
    ui->macroNameLineEdit->setFont(customFont);
    ui->macroContentLineEdit->setFont(customFont);
    ui->macroNoteLineEdit->setFont(customFont);
    ui->categoryLineEdit->setFont(customFont);
    ui->clearButton->setFont(customFont);
    ui->deleteMacroButton->setFont(customFont);
    ui->macroListBackupButton->setFont(customFont);

    int scale = QKeyMapper::getInstance()->m_UI_Scale;
    if (UI_SCALE_4K_PERCENT_150 == scale) {
        customFont.setPointSize(14);
    }
    else {
        customFont.setPointSize(12);
    }
    ui->addMacroButton->setFont(customFont);

    initMacroListTabWidget();

    ui->macroContentLineEdit->setMaxLength(MAPPINGKEY_LINE_EDIT_MAX_LENGTH);

    QObject::connect(ui->macroListTabWidget, &QTabWidget::currentChanged, this, &QMacroListDialog::macroListTabWidgetCurrentChanged);

    initMacroCategoryFilterToolButton();

    // Connect drag and drop move signal
    QObject::connect(this, &QMacroListDialog::macroListTableDragDropMove_Signal,
                     this, &QMacroListDialog::macroListTableDragDropMove);

    QObject::connect(ui->macroNameLineEdit, &QLineEdit::returnPressed, this, &QMacroListDialog::addMacroToList);
    QObject::connect(ui->macroContentLineEdit, &QLineEdit::returnPressed, this, &QMacroListDialog::addMacroToList);
    QObject::connect(ui->categoryLineEdit, &QLineEdit::returnPressed, this, &QMacroListDialog::addMacroToList);
    QObject::connect(ui->macroNoteLineEdit, &QLineEdit::returnPressed, this, &QMacroListDialog::addMacroToList);

    initMacroListBackupActionPopup();

    if (QItemSetupDialog::getInstance() != Q_NULLPTR) {
        QItemSetupDialog::getInstance()->syncConnectMappingKeySelectButtons();
    }
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
    ui->macroNoteLabel->setText(tr("Note"));
    ui->clearButton->setText(tr("Clear Editing"));
    ui->deleteMacroButton->setText(tr("Delete"));
    ui->addMacroButton->setText(tr("Add Macro"));
    ui->mapkeyLabel->setText(tr("MapKeys"));
    ui->categoryFilterLabel->setText(tr("Filter"));
    ui->macroListBackupButton->setText(tr("MacroList Backup"));

    QTabWidget *tabWidget = ui->macroListTabWidget;
    tabWidget->setTabText(tabWidget->indexOf(ui->macrolist),            tr("Macro")          );
    tabWidget->setTabText(tabWidget->indexOf(ui->universalmacrolist),   tr("Universal Macro"));

    ui->macrolistTable->setHorizontalHeaderLabels(QStringList()             << tr("Name")
                                                                            << tr("Macro")
                                                                            << tr("Category")
                                                                            << tr("Note"));
    ui->universalmacrolistTable->setHorizontalHeaderLabels(QStringList()    << tr("Name")
                                                                            << tr("Macro")
                                                                            << tr("Category")
                                                                            << tr("Note"));

    // Update MacroList backup action popup button texts
    if (m_MacroListBackupActionPopup != Q_NULLPTR) {
        QStringList macroListBackupButtonTextList;
        macroListBackupButtonTextList.append(tr("MacroList Export"));
        macroListBackupButtonTextList.append(tr("MacroList Import"));
        m_MacroListBackupActionPopup->setButtonTexts(macroListBackupButtonTextList);
    }

    resizeMacroListTabWidgetColumnWidth();
}

void QMacroListDialog::refreshMacroListTabWidget(MacroListDataTableWidget *macroDataTable, const OrderedMap<QString, MappingMacroData> &mappingMacroDataList)
{
    macroDataTable->clearContents();
    macroDataTable->setRowCount(0);

    if (false == mappingMacroDataList.isEmpty()){
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshMacroListTabWidget]" << "mappingMacroDataList Start >>>";
#endif
        QSignalBlocker blocker(macroDataTable);
        int rowindex = 0;
        macroDataTable->setRowCount(mappingMacroDataList.size());

        // Iterate through the OrderedMap using iterator for better performance
        for (auto it = mappingMacroDataList.begin(); it != mappingMacroDataList.end(); ++it)
        {
            const QString &macroName = it.key();
            const MappingMacroData &macroData = it.value();

            /* MACRO_NAME_COLUMN */
            QTableWidgetItem *name_TableItem = new QTableWidgetItem(macroName);
            name_TableItem->setToolTip(macroName);
            macroDataTable->setItem(rowindex, MACRO_NAME_COLUMN, name_TableItem);

            /* MACRO_CONTENT_COLUMN */
            QTableWidgetItem *content_TableItem = new QTableWidgetItem(macroData.MappingMacro);
            content_TableItem->setToolTip(macroData.MappingMacro);
            macroDataTable->setItem(rowindex, MACRO_CONTENT_COLUMN, content_TableItem);

            /* MACRO_CATEGORY_COLUMN */
            QTableWidgetItem *category_TableItem = new QTableWidgetItem(macroData.Category);
            category_TableItem->setToolTip(macroData.Category);
            macroDataTable->setItem(rowindex, MACRO_CATEGORY_COLUMN, category_TableItem);

            /* MACRO_NOTE_COLUMN */
            QTableWidgetItem *note_TableItem = new QTableWidgetItem(macroData.Note);
            note_TableItem->setToolTip(macroData.Note);
            macroDataTable->setItem(rowindex, MACRO_NOTE_COLUMN, note_TableItem);

            rowindex += 1;

#ifdef DEBUG_LOGOUT_ON
            qDebug().nospace() << "[refreshMacroListTabWidget] " << macroName << " -> " << macroData.MappingMacro << ", Category -> " << macroData.Category << ", Note -> " << macroData.Note;
#endif
        }

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshMacroListTabWidget]" << "mappingMacroDataList End <<<";
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[refreshMacroListTabWidget]" << "Empty mappingMacroDataList";
#endif
    }

    resizeMacroListTableColumnWidth(macroDataTable);

    updateMacroCategoryFilterComboBox();
}

void QMacroListDialog::refreshAllMacroListTabWidget()
{
    refreshMacroListTabWidget(ui->macrolistTable, QKeyMapper::s_MappingMacroList);
    refreshMacroListTabWidget(ui->universalmacrolistTable, QKeyMapper::s_UniversalMappingMacroList);
}

void QMacroListDialog::updateMappingKeyListComboBox()
{
    KeyListComboBox *mapkeyComboBox = QKeyMapper::getInstance()->m_mapkeyComboBox;

    ui->MacroList_MappingKeyListComboBox->clear();

    const QIcon &common_icon = QKeyMapper::s_Icon_Blank;
    ui->MacroList_MappingKeyListComboBox->addItem(QString());
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, SEPARATOR_WAITTIME);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, SEPARATOR_NEXTARROW);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_DOWN);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_UP);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_BOTH);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, PREFIX_SEND_EXCLUSION);
    ui->MacroList_MappingKeyListComboBox->addItem(common_icon, REPEAT_STR);
    for(int i = 1; i < mapkeyComboBox->count(); i++) {
        QIcon icon = mapkeyComboBox->itemIcon(i);
        QString text = mapkeyComboBox->itemText(i);
        ui->MacroList_MappingKeyListComboBox->addItem(icon, text);
    }
}

QPushButton *QMacroListDialog::getMapListSelectKeyboardButton() const
{
    return ui->mapList_SelectKeyboardButton;
}

QPushButton *QMacroListDialog::getMapListSelectMouseButton() const
{
    return ui->mapList_SelectMouseButton;
}

QPushButton *QMacroListDialog::getMapListSelectGamepadButton() const
{
    return ui->mapList_SelectGamepadButton;
}

QPushButton *QMacroListDialog::getMapListSelectFunctionButton() const
{
    return ui->mapList_SelectFunctionButton;
}

QString QMacroListDialog::getEditingMacroText()
{
    return getInstance()->ui->macroContentLineEdit->text();
}

int QMacroListDialog::getEditingMacroCursorPosition()
{
    return getInstance()->ui->macroContentLineEdit->cursorPosition();
}

void QMacroListDialog::setEditingMacroText(const QString &new_macrotext)
{
    return getInstance()->ui->macroContentLineEdit->setText(new_macrotext);
}

QString QMacroListDialog::getCurrentMapKeyListText()
{
    return getInstance()->ui->MacroList_MappingKeyListComboBox->currentText();
}

bool QMacroListDialog::isMacroDataTableFiltered()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable) {
        return false;
    }
    return !macroDataTable->m_CategoryFilters.isEmpty();
}

void QMacroListDialog::onMacroCategoryFilterChanged(int index)
{
    Q_UNUSED(index);
    // Deprecated: old ComboBox-based filter.
}

void QMacroListDialog::updateMacroCategoryFilterComboBox()
{
    // Repurposed: refresh toolbutton summary.
    updateMacroCategoryFilterToolButtonSummary();
}

void QMacroListDialog::initMacroCategoryFilterToolButton(void)
{
    if (!ui->categoryFilterToolButton) {
        return;
    }

    // Avoid QToolButton default menu popup positioning/sizing constraints.
    ui->categoryFilterToolButton->setPopupMode(QToolButton::DelayedPopup);

    if (!m_CategoryFilterMenu) {
        m_CategoryFilterMenu = new QMenu(ui->categoryFilterToolButton);

        m_CategoryFilterWidgetAction = new QWidgetAction(m_CategoryFilterMenu);
        m_CategoryFilterPanel = new MacroCategoryFilterPanelWidget(m_CategoryFilterMenu);
        QVBoxLayout *panelLayout = new QVBoxLayout(m_CategoryFilterPanel);
        panelLayout->setContentsMargins(8, 8, 8, 8);
        panelLayout->setSpacing(6);

        m_CategoryFilterAllCheckBox = new QCheckBox(tr("All"), m_CategoryFilterPanel);
        m_CategoryFilterAllCheckBox->setTristate(true);
        panelLayout->addWidget(m_CategoryFilterAllCheckBox);

        m_CategoryFilterScrollArea = new QScrollArea(m_CategoryFilterPanel);
        m_CategoryFilterScrollArea->setWidgetResizable(true);
        m_CategoryFilterScrollArea->setFrameShape(QFrame::NoFrame);
        panelLayout->addWidget(m_CategoryFilterScrollArea);

        m_CategoryFilterListContainer = new QWidget(m_CategoryFilterScrollArea);
        m_CategoryFilterListLayout = new QVBoxLayout(m_CategoryFilterListContainer);
        m_CategoryFilterListLayout->setContentsMargins(0, 0, 0, 0);
        m_CategoryFilterListLayout->setSpacing(2);
        m_CategoryFilterScrollArea->setWidget(m_CategoryFilterListContainer);

        m_CategoryFilterPanel->setMinimumWidth(CATEGORY_FILTER_MIN_WIDTH_MACROLIST);
        m_CategoryFilterPanel->setMaximumHeight(CATEGORY_FILTER_MAX_HEIGHT_MACROLIST);

        m_CategoryFilterWidgetAction->setDefaultWidget(m_CategoryFilterPanel);
        m_CategoryFilterMenu->addAction(m_CategoryFilterWidgetAction);

        // Deterministic popup: rebuild just before popup and clamp to screen.
        QObject::connect(ui->categoryFilterToolButton, &QToolButton::clicked, this, [this]() {
            if (!m_CategoryFilterMenu || !ui->categoryFilterToolButton || !m_CategoryFilterPanel) {
                return;
            }

            // Clear any previous fixed sizing so rebuild can compute a fresh fixed size.
            const int maxHeight = CATEGORY_FILTER_MAX_HEIGHT_MACROLIST;
            m_CategoryFilterPanel->setMinimumSize(0, 0);
            m_CategoryFilterPanel->setMaximumSize(QWIDGETSIZE_MAX, maxHeight);
            rebuildMacroCategoryFilterMenu();

            // Force menu/action to discard cached geometry and follow rebuilt panel size.
            refreshMenuWidgetActionGeometry(m_CategoryFilterMenu, m_CategoryFilterWidgetAction, m_CategoryFilterPanel);

            int minPanelH = 0;
            if (m_CategoryFilterAllCheckBox) {
                const QVBoxLayout *panelLayout = qobject_cast<const QVBoxLayout *>(m_CategoryFilterPanel->layout());
                const QMargins margins = panelLayout ? panelLayout->contentsMargins() : QMargins();
                const int spacing = panelLayout ? panelLayout->spacing() : 0;

                minPanelH = margins.top() + margins.bottom() + m_CategoryFilterAllCheckBox->sizeHint().height();
                if (!m_CategoryFilterCheckBoxes.isEmpty()) {
                    QCheckBox *anyCb = Q_NULLPTR;
                    for (auto it = m_CategoryFilterCheckBoxes.constBegin(); it != m_CategoryFilterCheckBoxes.constEnd(); ++it) {
                        if (it.value()) {
                            anyCb = it.value();
                            break;
                        }
                    }
                    const int oneRowH = anyCb ? anyCb->sizeHint().height() : 0;
                    const int scrollFrameH = m_CategoryFilterScrollArea ? (m_CategoryFilterScrollArea->frameWidth() * 2) : 0;
                    const int scrollMinH = oneRowH + scrollFrameH;
                    minPanelH += spacing + scrollMinH;
                }
            }

            m_CategoryFilterMenu->ensurePolished();
            m_CategoryFilterMenu->adjustSize();

            const QPoint anchor = ui->categoryFilterToolButton->mapToGlobal(QPoint(ui->categoryFilterToolButton->width(), 0));
            QScreen *screen = QGuiApplication::screenAt(anchor);
            if (!screen) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                screen = ui->categoryFilterToolButton->screen();
#else
                QWidget *topLevel = ui->categoryFilterToolButton->window();
                screen = (topLevel && topLevel->windowHandle())
                    ? topLevel->windowHandle()->screen()
                    : QGuiApplication::primaryScreen();
#endif
            }
            const QRect avail = screen ? screen->availableGeometry() : QRect();

            QSize popupSize = m_CategoryFilterMenu->sizeHint();
            QPoint pos = anchor; // prefer right, top-aligned

            if (screen && !avail.isEmpty()) {
                const int kMargin = 8;
                QRect availInner = avail.adjusted(kMargin, kMargin, -kMargin, -kMargin);
                if (availInner.isEmpty()) {
                    availInner = avail;
                }

                const int rightCandidate = anchor.x();
                const int leftCandidate = ui->categoryFilterToolButton->mapToGlobal(QPoint(0, 0)).x() - popupSize.width();

                // If right side doesn't fit, use left.
                if (rightCandidate + popupSize.width() > availInner.right() + 1) {
                    pos.setX(leftCandidate);
                }

                // If after switching sides it's still off-screen due to popup bigger than available, shrink panel to fit.
                if (popupSize.width() > availInner.width() || popupSize.height() > availInner.height()) {
                    const int basePanelW = qMax(m_CategoryFilterPanel->width(), m_CategoryFilterPanel->sizeHint().width());
                    const int basePanelH = qMax(m_CategoryFilterPanel->height(), m_CategoryFilterPanel->sizeHint().height());

                    int newPanelW = basePanelW;
                    int newPanelH = basePanelH;

                    if (popupSize.width() > availInner.width()) {
                        newPanelW = basePanelW - (popupSize.width() - availInner.width());
                        const int minW = qMin(CATEGORY_FILTER_MIN_WIDTH_MACROLIST, availInner.width());
                        newPanelW = qMax(minW, newPanelW);
                        newPanelW = qMin(newPanelW, availInner.width());
                    }
                    if (popupSize.height() > availInner.height()) {
                        newPanelH = basePanelH - (popupSize.height() - availInner.height());
                        const int effectiveMinH = qMin(minPanelH, availInner.height());
                        newPanelH = qMax(effectiveMinH, newPanelH);
                        newPanelH = qMin(newPanelH, availInner.height());
                    }

                    m_CategoryFilterPanel->setFixedSize(newPanelW, newPanelH);
                    m_CategoryFilterPanel->updateGeometry();

                    refreshMenuWidgetActionGeometry(m_CategoryFilterMenu, m_CategoryFilterWidgetAction, m_CategoryFilterPanel);
                    popupSize = m_CategoryFilterMenu->sizeHint();

                    // Re-evaluate side with updated size.
                    const int leftCandidate2 = ui->categoryFilterToolButton->mapToGlobal(QPoint(0, 0)).x() - popupSize.width();
                    pos.setX(rightCandidate);
                    if (rightCandidate + popupSize.width() > availInner.right() + 1) {
                        pos.setX(leftCandidate2);
                    }
                }

                // Clamp within available geometry.
                pos.setX(qBound(availInner.left(), pos.x(), availInner.right() - popupSize.width() + 1));
                pos.setY(qBound(availInner.top(), pos.y(), availInner.bottom() - popupSize.height() + 1));
            }

            m_CategoryFilterMenu->popup(pos);
        });

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        QObject::connect(m_CategoryFilterAllCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
            if (m_CategoryFilterGuard) {
                return;
            }

            Qt::CheckState newState = state;
            if (newState == Qt::PartiallyChecked) {
                m_CategoryFilterGuard = true;
                m_CategoryFilterAllCheckBox->setCheckState(Qt::Checked);
                m_CategoryFilterGuard = false;
                newState = Qt::Checked;
            }

            m_CategoryFilterGuard = true;
            const bool checkAll = (newState == Qt::Checked);
            for (QCheckBox *cb : std::as_const(m_CategoryFilterCheckBoxes)) {
                cb->setChecked(checkAll);
            }
            m_CategoryFilterGuard = false;
            applyMacroCategoryFilterFromUI();
        });
#else
        QObject::connect(m_CategoryFilterAllCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
            if (m_CategoryFilterGuard) {
                return;
            }

            Qt::CheckState newState = static_cast<Qt::CheckState>(state);
            if (newState == Qt::PartiallyChecked) {
                m_CategoryFilterGuard = true;
                m_CategoryFilterAllCheckBox->setCheckState(Qt::Checked);
                m_CategoryFilterGuard = false;
                newState = Qt::Checked;
            }

            m_CategoryFilterGuard = true;
            const bool checkAll = (newState == Qt::Checked);
            for (QCheckBox *cb : std::as_const(m_CategoryFilterCheckBoxes)) {
                cb->setChecked(checkAll);
            }
            m_CategoryFilterGuard = false;
            applyMacroCategoryFilterFromUI();
        });
#endif
    }

    ui->categoryFilterToolButton->setText(tr("All"));
    ui->categoryFilterToolButton->setToolTip(tr("All"));
}

void QMacroListDialog::rebuildMacroCategoryFilterMenu(void)
{
    static const QString kNoneToken = QStringLiteral("__QKM_INTERNAL_NONE__");
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable || !m_CategoryFilterListLayout || !m_CategoryFilterAllCheckBox) {
        return;
    }

    QStringList categories = macroDataTable->getAvailableCategories();
    QStringList regularCategories;
    bool hasBlankOption = false;
    for (const QString &category : std::as_const(categories)) {
        if (category == tr("Blank")) {
            hasBlankOption = true;
        }
        else {
            regularCategories.append(category);
        }
    }
    regularCategories.sort();
    m_CategoryFilterDisplayOrder = regularCategories;
    if (hasBlankOption) {
        m_CategoryFilterDisplayOrder.append(QString());
    }

    while (QLayoutItem *child = m_CategoryFilterListLayout->takeAt(0)) {
        if (QWidget *w = child->widget()) {
            w->deleteLater();
        }
        delete child;
    }
    m_CategoryFilterCheckBoxes.clear();

    for (const QString &categoryValue : std::as_const(m_CategoryFilterDisplayOrder)) {
        const bool isBlank = categoryValue.isEmpty();
        QCheckBox *cb = new QCheckBox(isBlank ? tr("Blank") : categoryValue, m_CategoryFilterListContainer);
        cb->setTristate(false);
        cb->setProperty("categoryValue", categoryValue);
        QString cb_text = cb->text();
        cb_text.replace("&", "&&");
        cb->setText(cb_text);
        m_CategoryFilterListLayout->addWidget(cb);
        m_CategoryFilterCheckBoxes.insert(categoryValue, cb);

        QObject::connect(cb, &QCheckBox::toggled, this, [this](bool) {
            if (m_CategoryFilterGuard) {
                return;
            }
            applyMacroCategoryFilterFromUI();
        });
    }
    m_CategoryFilterListLayout->addStretch(1);

    QSet<QString> filters = macroDataTable->m_CategoryFilters;
    if (!filters.contains(kNoneToken)) {
        QSet<QString> allowed;
        for (const QString &v : std::as_const(m_CategoryFilterDisplayOrder)) {
            allowed.insert(v);
        }
        filters = filters.intersect(allowed);
    }

    m_CategoryFilterGuard = true;
    if (filters.isEmpty()) {
        for (QCheckBox *cb : std::as_const(m_CategoryFilterCheckBoxes)) {
            cb->setChecked(true);
        }
    }
    else if (filters.contains(kNoneToken)) {
        for (QCheckBox *cb : std::as_const(m_CategoryFilterCheckBoxes)) {
            cb->setChecked(false);
        }
    }
    else {
        for (auto it = m_CategoryFilterCheckBoxes.constBegin(); it != m_CategoryFilterCheckBoxes.constEnd(); ++it) {
            it.value()->setChecked(filters.contains(it.key()));
        }
    }
    m_CategoryFilterGuard = false;

    if (filters != macroDataTable->m_CategoryFilters) {
        macroDataTable->setCategoryFilters(filters);
    }

    updateMacroAllCheckStateFromItems();
    updateMacroCategoryFilterToolButtonSummary();

    // Dynamically size the panel to content (width fits longest checkbox text; height fits items up to max).
    if (m_CategoryFilterPanel && m_CategoryFilterScrollArea && m_CategoryFilterAllCheckBox) {
        const QVBoxLayout *panelLayout = qobject_cast<QVBoxLayout *>(m_CategoryFilterPanel->layout());
        const QMargins panelMargins = panelLayout ? panelLayout->contentsMargins() : QMargins();
        const int panelSpacing = panelLayout ? panelLayout->spacing() : 0;

        const QVBoxLayout *listLayout = m_CategoryFilterListLayout;
        const QMargins listMargins = listLayout ? listLayout->contentsMargins() : QMargins();
        const int listSpacing = listLayout ? listLayout->spacing() : 0;

        int maxCheckWidth = m_CategoryFilterAllCheckBox->sizeHint().width();
        int itemsHeight = 0;
        const int itemCount = m_CategoryFilterCheckBoxes.size();
        for (QCheckBox *cb : std::as_const(m_CategoryFilterCheckBoxes)) {
            const QSize sh = cb->sizeHint();
            maxCheckWidth = qMax(maxCheckWidth, sh.width());
            itemsHeight += sh.height();
        }
        if (itemCount > 1) {
            itemsHeight += listSpacing * (itemCount - 1);
        }
        itemsHeight += listMargins.top() + listMargins.bottom();

        const int allHeight = m_CategoryFilterAllCheckBox->sizeHint().height();
        const int otherHeight = panelMargins.top() + panelMargins.bottom() + allHeight + panelSpacing + 16;

        const int maxHeight = CATEGORY_FILTER_MAX_HEIGHT_MACROLIST;
        const int desiredHeightUncapped = otherHeight + itemsHeight;
        const int desiredHeight = qMin(desiredHeightUncapped, maxHeight);

        bool needsVScroll = (desiredHeightUncapped > maxHeight);
        int idealWidth = panelMargins.left() + panelMargins.right() + maxCheckWidth;

        if (needsVScroll) {
            const int sbExtent = m_CategoryFilterPanel->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_CategoryFilterPanel);
            idealWidth += sbExtent;
        }

        int desiredWidth = qMax(idealWidth, CATEGORY_FILTER_MIN_WIDTH_MACROLIST);
        desiredWidth = qMin(desiredWidth, CATEGORY_FILTER_MAX_WIDTH_MACROLIST);

        // Ensure horizontal scrolling can happen when width is capped.
        if (m_CategoryFilterListContainer) {
            m_CategoryFilterListContainer->setMinimumWidth(maxCheckWidth);
        }

        m_CategoryFilterScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_CategoryFilterScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_CategoryFilterPanel->setFixedSize(desiredWidth, desiredHeight);
        m_CategoryFilterPanel->updateGeometry();

        refreshMenuWidgetActionGeometry(m_CategoryFilterMenu, m_CategoryFilterWidgetAction, m_CategoryFilterPanel);
    }
}

void QMacroListDialog::applyMacroCategoryFilterFromUI(void)
{
    static const QString kNoneToken = QStringLiteral("__QKM_INTERNAL_NONE__");
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable) {
        return;
    }

    int total = 0;
    int checkedCount = 0;
    QSet<QString> selected;
    for (auto it = m_CategoryFilterCheckBoxes.constBegin(); it != m_CategoryFilterCheckBoxes.constEnd(); ++it) {
        total += 1;
        if (it.value()->isChecked()) {
            checkedCount += 1;
            selected.insert(it.key());
        }
    }

    if (total > 0 && checkedCount == 0) {
        selected.clear();
        selected.insert(kNoneToken);
    }
    else if (total > 0 && checkedCount == total) {
        selected.clear();
    }

    macroDataTable->setCategoryFilters(selected);
    updateMacroAllCheckStateFromItems();
    updateMacroCategoryFilterToolButtonSummary();
}

void QMacroListDialog::updateMacroAllCheckStateFromItems(void)
{
    if (!m_CategoryFilterAllCheckBox) {
        return;
    }

    int total = 0;
    int checkedCount = 0;
    for (QCheckBox *cb : std::as_const(m_CategoryFilterCheckBoxes)) {
        total += 1;
        if (cb->isChecked()) {
            checkedCount += 1;
        }
    }

    m_CategoryFilterGuard = true;
    if (total == 0) {
        m_CategoryFilterAllCheckBox->setEnabled(false);
        m_CategoryFilterAllCheckBox->setCheckState(Qt::Unchecked);
    }
    else {
        m_CategoryFilterAllCheckBox->setEnabled(true);
        if (checkedCount == 0) {
            m_CategoryFilterAllCheckBox->setCheckState(Qt::Unchecked);
        }
        else if (checkedCount == total) {
            m_CategoryFilterAllCheckBox->setCheckState(Qt::Checked);
        }
        else {
            m_CategoryFilterAllCheckBox->setCheckState(Qt::PartiallyChecked);
        }
    }
    m_CategoryFilterGuard = false;
}

void QMacroListDialog::updateMacroCategoryFilterToolButtonSummary(void)
{
    static const QString kNoneToken = QStringLiteral("__QKM_INTERNAL_NONE__");
    if (!ui->categoryFilterToolButton) {
        return;
    }
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable) {
        return;
    }

    if (m_CategoryFilterAllCheckBox) {
        m_CategoryFilterAllCheckBox->setText(tr("All"));
    }

    if (m_CategoryFilterDisplayOrder.isEmpty()) {
        QStringList categories = macroDataTable->getAvailableCategories();
        QStringList regularCategories;
        bool hasBlankOption = false;
        for (const QString &category : std::as_const(categories)) {
            if (category == tr("Blank")) {
                hasBlankOption = true;
            }
            else {
                regularCategories.append(category);
            }
        }
        regularCategories.sort();
        m_CategoryFilterDisplayOrder = regularCategories;
        if (hasBlankOption) {
            m_CategoryFilterDisplayOrder.append(QString());
        }
    }

    const QSet<QString> filters = macroDataTable->m_CategoryFilters;
    QString buttonText;
    QString tooltip;

    if (filters.contains(kNoneToken)) {
        buttonText = tr("None");
        tooltip = buttonText;
    }
    else if (filters.isEmpty()) {
        buttonText = tr("All");
        tooltip = buttonText;
    }
    else {
        QStringList selectedInOrder;
        for (const QString &v : std::as_const(m_CategoryFilterDisplayOrder)) {
            if (filters.contains(v)) {
                selectedInOrder.append(v.isEmpty() ? tr("Blank") : v);
            }
        }
        tooltip = selectedInOrder.join("\n");
        if (selectedInOrder.size() <= 2) {
            buttonText = selectedInOrder.join(", ");
        }
        else {
            buttonText = tr("%1, %2 (+%3)")
                             .arg(selectedInOrder.at(0), selectedInOrder.at(1))
                             .arg(selectedInOrder.size() - 2);
        }
        if (buttonText.isEmpty()) {
            buttonText = tr("None");
        }
        else {
            buttonText.replace("&", "&&");
        }
    }

    ui->categoryFilterToolButton->setText(buttonText);
    ui->categoryFilterToolButton->setToolTip(tooltip);
}

void QMacroListDialog::showEvent(QShowEvent *event)
{
    refreshMacroListTabWidget(ui->macrolistTable, QKeyMapper::s_MappingMacroList);
    refreshMacroListTabWidget(ui->universalmacrolistTable, QKeyMapper::s_UniversalMappingMacroList);

    QDialog::showEvent(event);
}

void QMacroListDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

void QMacroListDialog::resizeEvent(QResizeEvent *event)
{
    // Call base class implementation first
    QDialog::resizeEvent(event);

    // Adjust table column widths to fit new window size
    resizeMacroListTabWidgetColumnWidth();
}

void QMacroListDialog::on_addMacroButton_clicked()
{
    addMacroToList();
}

void QMacroListDialog::on_clearButton_clicked()
{
    QString macroName = ui->macroNameLineEdit->text();
    QString macroContent = ui->macroContentLineEdit->text();
    QString category = ui->categoryLineEdit->text();
    QString macroNote = ui->macroNoteLineEdit->text();

    // Check if any field has content
    if (!macroName.isEmpty() || !macroContent.isEmpty() || !category.isEmpty() || !macroNote.isEmpty()) {
        QString message = tr("Are you sure you want to clear the macro information fields?");
        QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message,
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            // User cancelled, don't clear
            return;
        }

        // Clear all input fields
        ui->macroNameLineEdit->clear();
        ui->macroContentLineEdit->clear();
        ui->macroNoteLineEdit->clear();
        ui->categoryLineEdit->clear();
    }
}

void QMacroListDialog::on_deleteMacroButton_clicked()
{
    deleteMacroSelectedItems();
}

void QMacroListDialog::on_macroListBackupButton_clicked()
{
    // Calculate the starting position (to the right of the backup button)
    QPoint globalPos = ui->macroListBackupButton->mapToGlobal(QPoint(ui->macroListBackupButton->width(), 0));
    int popupWidth = 140;
    int popupHeight = 80;

    // Use opacity animation to show the popup
    QRect finalRect(globalPos.x(), globalPos.y(), popupWidth, popupHeight);
    m_MacroListBackupActionPopup->setGeometry(finalRect);
    m_MacroListBackupActionPopup->setWindowOpacity(0.0); // Start transparent
    m_MacroListBackupActionPopup->show();

    // Fade-in animation
    QPropertyAnimation *opacityAnim = new QPropertyAnimation(m_MacroListBackupActionPopup, "windowOpacity");
    opacityAnim->setDuration(500);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutQuart);
    opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void QMacroListDialog::initMacroListBackupActionPopup()
{
    QStringList macroListBackupActionList;
    macroListBackupActionList.append(MACROLIST_BACKUP_ACTION_EXPORT);
    macroListBackupActionList.append(MACROLIST_BACKUP_ACTION_IMPORT);

    m_MacroListBackupActionPopup = new ActionPopup(macroListBackupActionList, this);

    QStringList macroListBackupButtonTextList;
    macroListBackupButtonTextList.append(tr("MacroList Export"));
    macroListBackupButtonTextList.append(tr("MacroList Import"));
    m_MacroListBackupActionPopup->setButtonTexts(macroListBackupButtonTextList);

    QObject::connect(m_MacroListBackupActionPopup, &ActionPopup::actionTriggered,
                     this, &QMacroListDialog::macroListBackupActionTriggered);
}

void QMacroListDialog::macroListBackupActionTriggered(const QString &actionName)
{
    if (actionName == MACROLIST_BACKUP_ACTION_EXPORT) {
        exportMacroListToFile();
    }
    else if (actionName == MACROLIST_BACKUP_ACTION_IMPORT) {
        importMacroListFromFile();
    }
}

void QMacroListDialog::exportMacroListToFile()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    // Check if any rows are selected
    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
        // No selection, show warning message
        QString popupMessage = tr("Please select the macro items to export first.");
        QString popupMessageColor = FAILURE_COLOR;
        int popupMessageDisplayTime = 3000;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    // Get the selection range (ContiguousSelection mode ensures only one range)
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();
    int selectedCount = bottomRow - topRow + 1;

    // Collect selected macro data
    OrderedMap<QString, MappingMacroData> exportMacroList;
    for (int row = topRow; row <= bottomRow; ++row) {
        QTableWidgetItem *nameItem = macroDataTable->item(row, MACRO_NAME_COLUMN);
        if (nameItem) {
            QString macroName = nameItem->text();
            if (macroDataList->contains(macroName)) {
                exportMacroList[macroName] = macroDataList->value(macroName);
            }
        }
    }

    if (exportMacroList.isEmpty()) {
        return;
    }

    // Get current tab name for dialog title
    QString tabName;
    if (ui->macroListTabWidget->currentWidget() == ui->macrolist) {
        tabName = tr("Macro");
    }
    else {
        tabName = tr("Universal Macro");
    }

    QString default_filename = MACROLIST_EXPORT_DEFAULT_FILENAME;
    QString filter = "INI files (*.ini)";
    QString caption_string = tr("Export macro list : ") + tabName;

    QString export_filename = QFileDialog::getSaveFileName(this,
                                                           caption_string,
                                                           default_filename,
                                                           filter);
    if (export_filename.isEmpty()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[exportMacroListToFile] export_filename: " << export_filename << ", count: " << selectedCount;
#endif

    // Save to INI file
    QSettings exportFile(export_filename, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    exportFile.setIniCodec("UTF-8");
#endif

    // Convert OrderedMap to QVariantList for INI storage
    QVariantList macroList;
    for (auto it = exportMacroList.begin(); it != exportMacroList.end(); ++it) {
        QVariantMap macroMap;
        macroMap[MACROLIST_FIELD_MACRONAME] = it.key();
        macroMap[MACROLIST_FIELD_MACROCONTENT] = it.value().MappingMacro;
        macroMap[MACROLIST_FIELD_MACROCATEGORY] = it.value().Category;
        macroMap[MACROLIST_FIELD_MACRONOTE] = it.value().Note;
        macroList.append(macroMap);
    }

    exportFile.setValue(MACROLIST_EXPORT, macroList);
    exportFile.sync();

    // Show success popup message
    QString popupMessage = tr("Successfully exported %1 macro(s).").arg(selectedCount);
    QString popupMessageColor = SUCCESS_COLOR;
    int popupMessageDisplayTime = 3000;
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}

void QMacroListDialog::importMacroListFromFile()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    // Get current tab name for dialog title
    QString tabName;
    if (ui->macroListTabWidget->currentWidget() == ui->macrolist) {
        tabName = tr("Macro");
    }
    else {
        tabName = tr("Universal Macro");
    }

    QString filter = "INI files (*.ini)";
    QString caption_string = tr("Import macro list : ") + tabName;

    QString import_filename = QFileDialog::getOpenFileName(this,
                                                           caption_string,
                                                           QString(),
                                                           filter);
    if (import_filename.isEmpty()) {
        return;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[importMacroListFromFile] import_filename: " << import_filename;
#endif

    // Load from INI file
    QSettings importFile(import_filename, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    importFile.setIniCodec("UTF-8");
#endif

    if (!importFile.contains(MACROLIST_EXPORT)) {
        // No MacroList_Export key found in file
        QString popupMessage = tr("No macro data found in the selected file.");
        QString popupMessageColor = FAILURE_COLOR;
        int popupMessageDisplayTime = 3000;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    // Load macro list from INI file
    QVariant macroListVar = importFile.value(MACROLIST_EXPORT);
    if (!macroListVar.isValid() || !macroListVar.canConvert<QVariantList>()) {
        QString popupMessage = tr("Invalid macro data format in the selected file.");
        QString popupMessageColor = FAILURE_COLOR;
        int popupMessageDisplayTime = 3000;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    // Parse imported macro data
    OrderedMap<QString, MappingMacroData> importMacroList;
    QVariantList macroList = macroListVar.toList();
    for (const QVariant &macroVar : std::as_const(macroList)) {
        if (!macroVar.canConvert<QVariantMap>()) {
            continue;
        }

        QVariantMap macroMap = macroVar.toMap();

        QString macroName = macroMap.value(MACROLIST_FIELD_MACRONAME).toString();
        QString macroContent = macroMap.value(MACROLIST_FIELD_MACROCONTENT).toString();
        QString category = macroMap.value(MACROLIST_FIELD_MACROCATEGORY).toString();
        QString note = macroMap.value(MACROLIST_FIELD_MACRONOTE).toString();

        if (!macroName.isEmpty()) {
            importMacroList[macroName] = MappingMacroData{ macroContent, category, note };
        }
    }

    if (importMacroList.isEmpty()) {
        QString popupMessage = tr("No valid macro data found in the selected file.");
        QString popupMessageColor = FAILURE_COLOR;
        int popupMessageDisplayTime = 3000;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    // Check for duplicate macro names
    QStringList duplicateNames;
    for (auto it = importMacroList.begin(); it != importMacroList.end(); ++it) {
        if (macroDataList->contains(it.key())) {
            duplicateNames.append(it.key());
        }
    }

    bool overwriteDuplicates = false;
    if (!duplicateNames.isEmpty()) {
        // Ask user whether to overwrite duplicates
        // Show only the first duplicate name and total count to keep the message concise
        QString message;
        if (duplicateNames.size() == 1) {
            message = tr("Macro name \"%1\" already exists.\n\nDo you want to overwrite it?")
                      .arg(duplicateNames.first());
        }
        else {
            message = tr("Macro name \"%1\" and %2 other(s) already exist.\n\nDo you want to overwrite them?")
                      .arg(duplicateNames.first())
                      .arg(duplicateNames.size() - 1);
        }
        QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message,
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No);
        overwriteDuplicates = (reply == QMessageBox::Yes);
    }

    // Record the starting row for new items (for selection after import)
    // int originalRowCount = macroDataList->size();
    int importedCount = 0;

    // Import macro data
    for (auto it = importMacroList.begin(); it != importMacroList.end(); ++it) {
        const QString &macroName = it.key();
        const MappingMacroData &macroData = it.value();

        if (macroDataList->contains(macroName)) {
            if (overwriteDuplicates) {
                // Use insert() to move existing key to end and update value
                macroDataList->insert(macroName, macroData);
                ++importedCount;
            }
            // If not overwriting, skip this macro
        }
        else {
            // New macro, insert at end
            macroDataList->insert(macroName, macroData);
            ++importedCount;
        }
    }

    if (importedCount == 0) {
        QString popupMessage = tr("No new macro data was imported.");
        QString popupMessageColor = FAILURE_COLOR;
        int popupMessageDisplayTime = 3000;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    // Refresh the table display
    if (ui->macroListTabWidget->currentWidget() == ui->macrolist) {
        refreshMacroListTabWidget(ui->macrolistTable, QKeyMapper::s_MappingMacroList);
    }
    else {
        refreshMacroListTabWidget(ui->universalmacrolistTable, QKeyMapper::s_UniversalMappingMacroList);
    }

    // Calculate the new row range for imported items
    int newRowCount = macroDataList->size();
    int importStartRow = newRowCount - importedCount;
    int importEndRow = newRowCount - 1;

    // Select the imported rows to highlight them
    if (importStartRow >= 0 && importStartRow < macroDataTable->rowCount()) {
        macroDataTable->clearSelection();
        QTableWidgetSelectionRange newSelection(importStartRow, 0, importEndRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
        macroDataTable->setRangeSelected(newSelection, true);

        // Update current cell to the start of imported rows for Ctrl/Shift+Click consistency
        macroDataTable->setCurrentCell(importStartRow, 0, QItemSelectionModel::NoUpdate);

        // Scroll to make the imported items visible
        macroDataTable->scrollToItem(macroDataTable->item(importStartRow, 0));
    }

    // Show success popup message
    QString popupMessage = tr("Successfully imported %1 macro(s).").arg(importedCount);
    QString popupMessageColor = SUCCESS_COLOR;
    int popupMessageDisplayTime = 3000;
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[importMacroListFromFile] Imported " << importedCount << " macros, selection range: " << importStartRow << " - " << importEndRow;
#endif
}

void QMacroListDialog::addMacroToList()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();

    if (macroDataTable == Q_NULLPTR) {
        return;
    }

    OrderedMap<QString, MappingMacroData>& CurrentMacroList = (macroDataTable == ui->macrolistTable) ? QKeyMapper::s_MappingMacroList : QKeyMapper::s_UniversalMappingMacroList;

#ifdef DEBUG_LOGOUT_ON
    const char* currentTabStr = (macroDataTable == ui->macrolistTable) ? "Macro List" : "Universal Macro List";
    qDebug() << "[QMacroListDialog::addMacroToList] Add macro to" << currentTabStr;
#endif

    static QRegularExpression simplified_regex(R"([\r\n]+)");

    QString macroname_str = ui->macroNameLineEdit->text();
    macroname_str.replace(simplified_regex, " ");
    macroname_str = macroname_str.trimmed();

    QString macro_str = ui->macroContentLineEdit->text();
    macro_str.replace(simplified_regex, " ");
    macro_str = macro_str.trimmed();

    QString category_str = ui->categoryLineEdit->text();
    category_str.replace(simplified_regex, " ");
    category_str = category_str.trimmed();

    QString macronote_str = ui->macroNoteLineEdit->text();
    macronote_str.replace(simplified_regex, " ");
    macronote_str = macronote_str.trimmed();

    if (macro_str.isEmpty()) {
        return;
    }

    QString popupMessage;
    QString popupMessageColor;
    int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;

    if (macroname_str.isEmpty()) {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("Macro name cannot be empty.");
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    // Validate macro name does not contain ')' character
    // This ensures the macro can be referenced correctly in Macro(name) syntax
    if (macroname_str.contains(')')) {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("Macro name cannot contain ')' character.");
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    ValidationResult result = QKeyMapper::validateMappingKeyString(macro_str);

    if (!result.isValid) {
        popupMessageColor = FAILURE_COLOR;
        popupMessage = tr("Macro") + " -> " + result.errorMessage;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
        return;
    }

    // Check if macro name already exists
    bool isUpdate = false;
    if (CurrentMacroList.contains(macroname_str)) {
        QString dialogTitle = (macroDataTable == ui->macrolistTable) ? tr("Macro List") : tr("Universal Macro List");
        QString messageText = tr("Macro name already exists. Replace existing macro?");
        
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            dialogTitle,
            messageText,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        
        if (reply != QMessageBox::Yes) {
            return;
        }
        isUpdate = true;
    }

    // Insert or update macro in the list
    CurrentMacroList[macroname_str] = MappingMacroData{ macro_str, category_str, macronote_str};

    // Show success message
    popupMessageColor = SUCCESS_COLOR;
    if (macroDataTable == ui->macrolistTable) {
        popupMessage = tr("Macro List") + " -> ";
    }
    else {
        popupMessage = tr("Universal Macro List") + " -> ";
    }
    if (isUpdate) {
        popupMessage = popupMessage + tr("Macro \"%1\" updated successfully").arg(macroname_str);
    }
    else {
        popupMessage = popupMessage + tr("Macro \"%1\" added successfully").arg(macroname_str);
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);

    // Refresh the macro list display
    refreshMacroListTabWidget(macroDataTable, CurrentMacroList);
}

void QMacroListDialog::initMacroListTabWidget()
{
    QTabWidget *tabWidget = ui->macroListTabWidget;
    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        tabWidget->setStyle(windowsStyle);
    }
    tabWidget->setFocusPolicy(Qt::StrongFocus);
    QTabBar *bar = tabWidget->tabBar();
    for (QObject *child : bar->children()) {
        if (QToolButton *btn = qobject_cast<QToolButton *>(child)) {
            btn->setFocusPolicy(Qt::NoFocus);
        }
    }
    tabWidget->setFont(QFont(FONTNAME_ENGLISH, 9));
    tabWidget->setCurrentIndex(tabWidget->indexOf(ui->macrolist));

    initMacroListTable(ui->macrolistTable);
    initMacroListTable(ui->universalmacrolistTable);
}

void QMacroListDialog::initMacroListTable(MacroListDataTableWidget *macroDataTable)
{
    macroDataTable->setFocusPolicy(Qt::ClickFocus);
    macroDataTable->setColumnCount(MACROLISTDATA_TABLE_COLUMN_COUNT);

    macroDataTable->horizontalHeader()->setStretchLastSection(true);
    macroDataTable->horizontalHeader()->setHighlightSections(false);

    // macroDataTable->verticalHeader()->setVisible(false);
    macroDataTable->verticalHeader()->setDefaultSectionSize(25);
    macroDataTable->verticalHeader()->setStyleSheet("QHeaderView::section { color: #1A9EDB; padding-left: 2px; padding-right: 1px;}");
    macroDataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    macroDataTable->setSelectionMode(QAbstractItemView::ContiguousSelection);
    // Enable double-click editing for category column
    macroDataTable->setEditTriggers(QAbstractItemView::DoubleClicked);

    /* Support Drag&Drop for macroDataTable Table */
    macroDataTable->setDragEnabled(true);
    macroDataTable->setDragDropMode(QAbstractItemView::InternalMove);
    macroDataTable->setDefaultDropAction(Qt::MoveAction);

    macroDataTable->setHorizontalHeaderLabels(QStringList() << tr("Name")
                                                            << tr("Macro")
                                                            << tr("Category")
                                                            << tr("Note"));

    QFont customFont(FONTNAME_ENGLISH, 9);
    macroDataTable->setFont(customFont);
    macroDataTable->horizontalHeader()->setFont(customFont);
    if (QStyle *fusionStyle = QKeyMapperStyle::fusionStyle()) {
        macroDataTable->setStyle(fusionStyle);
    }

    resizeMacroListTableColumnWidth(macroDataTable);

    // Connect signals for this table
    updateMacroDataTableConnection(macroDataTable);
}

void QMacroListDialog::updateMacroDataTableConnection(MacroListDataTableWidget *macroDataTable)
{
    if (macroDataTable != Q_NULLPTR) {
        QObject::connect(macroDataTable, &QTableWidget::cellChanged,
                         this, &QMacroListDialog::macroTableCellChanged, Qt::UniqueConnection);
        QObject::connect(macroDataTable, &QTableWidget::itemSelectionChanged,
                         this, &QMacroListDialog::macroTableItemSelectionChanged, Qt::UniqueConnection);
        QObject::connect(macroDataTable, &QTableWidget::itemDoubleClicked,
                         this, &QMacroListDialog::macroTableItemDoubleClicked, Qt::UniqueConnection);
#ifdef DEBUG_LOGOUT_ON
        QObject::connect(macroDataTable, &QTableWidget::currentCellChanged,
                         this, &QMacroListDialog::macroTableCurrentCellChanged, Qt::UniqueConnection);
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qWarning() << "[updateMacroDataTableConnection]" << "Invalid macroDataTable pointer!";
#endif
    }
}

void QMacroListDialog::initKeyListComboBoxes()
{
    updateMappingKeyListComboBox();
}

void QMacroListDialog::resizeMacroListTabWidgetColumnWidth()
{
    resizeMacroListTableColumnWidth(ui->macrolistTable);
    resizeMacroListTableColumnWidth(ui->universalmacrolistTable);
}

void QMacroListDialog::resizeMacroListTableColumnWidth(MacroListDataTableWidget *macroDataTable)
{
    // When verticalHeader is visible (row numbers), it consumes horizontal space
    // from the table viewport. Subtract it from our column width budget to keep
    // the same no-horizontal-scroll behavior as when verticalHeader was hidden.
    int verticalHeaderWidth = 0;
    if (macroDataTable->verticalHeader()) {
        verticalHeaderWidth = qMax(macroDataTable->verticalHeader()->width(),
                                  macroDataTable->verticalHeader()->sizeHint().width());
    }

    int referenceWidth = ui->macroListTabWidget->width();

    macroDataTable->resizeColumnToContents(MACRO_NAME_COLUMN);

    int macro_name_width_min = referenceWidth/7 - 15;
    int macro_name_width_max = referenceWidth / 2;
    int macro_name_width = macroDataTable->columnWidth(MACRO_NAME_COLUMN);

    macroDataTable->horizontalHeader()->setStretchLastSection(false);

    // Calculate Category column width
    int macro_category_width_min = referenceWidth / 20 + 5;
    int macro_category_width_max = referenceWidth / 5;
    macroDataTable->resizeColumnToContents(MACRO_CATEGORY_COLUMN);
    int macro_category_width = macroDataTable->columnWidth(MACRO_CATEGORY_COLUMN);
    if (macro_category_width < macro_category_width_min) {
        macro_category_width = macro_category_width_min;
    }
    if (macro_category_width > macro_category_width_max) {
        macro_category_width = macro_category_width_max;
    }

    // Calculate Note column width
    int macro_note_width_min = referenceWidth / 20 + 5;
    int macro_note_width_max = referenceWidth / 5;
    macroDataTable->resizeColumnToContents(MACRO_NOTE_COLUMN);
    int macro_note_width = macroDataTable->columnWidth(MACRO_NOTE_COLUMN);
    if (macro_note_width < macro_note_width_min) {
        macro_note_width = macro_note_width_min;
    }
    if (macro_note_width > macro_note_width_max) {
        macro_note_width = macro_note_width_max;
    }

    macroDataTable->horizontalHeader()->setStretchLastSection(true);

    if (macro_name_width < macro_name_width_min) {
        macro_name_width = macro_name_width_min;
    }
    else if (macro_name_width > macro_name_width_max) {
        macro_name_width = macro_name_width_max;
    }

    int macro_content_width_min = referenceWidth/5 - 15;
    int macro_content_width = referenceWidth - verticalHeaderWidth - macro_name_width - macro_category_width - macro_note_width - 24;
    if (macro_content_width < macro_content_width_min) {
        macro_content_width = macro_content_width_min;
    }

    macroDataTable->setColumnWidth(MACRO_NAME_COLUMN, macro_name_width);
    macroDataTable->setColumnWidth(MACRO_CONTENT_COLUMN, macro_content_width);
    macroDataTable->setColumnWidth(MACRO_CATEGORY_COLUMN, macro_category_width);
    macroDataTable->setColumnWidth(MACRO_NOTE_COLUMN, macro_note_width);
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[resizeMacroListTableColumnWidth]" << "macroDataTable->rowCount" << macroDataTable->rowCount();
    qDebug() << "[resizeMacroListTableColumnWidth]" << "referenceWidth =" << referenceWidth << ", verticalHeaderWidth =" << verticalHeaderWidth << ", macro_name_width =" << macro_name_width << ", macro_content_width =" << macro_content_width << ", macro_category_width =" << macro_category_width << ", macro_note_width =" << macro_note_width;
#endif
}

void QMacroListDialog::updateMacroListTableItem(MacroListDataTableWidget *macroDataTable, OrderedMap<QString, MappingMacroData> *macroDataList, int row, int column)
{
    // Validate input parameters
    if (!macroDataTable || !macroDataList) {
        return;
    }

    if (row < 0 || row >= macroDataList->size() || row >= macroDataTable->rowCount()) {
        return;
    }

    if (column < MACRO_NAME_COLUMN || column > MACRO_NOTE_COLUMN) {
        return;
    }

    // Block signals to prevent triggering cellChanged during update
    QSignalBlocker blocker(macroDataTable);

    // Get the macro name and data at the specified row
    QList<QString> macroNameList = macroDataList->keys();
    QString macroName = macroNameList.at(row);
    const MappingMacroData &macroData = macroDataList->value(macroName);

    // Update the specific column
    switch (column) {
        case MACRO_NAME_COLUMN: {
            QTableWidgetItem *nameItem = macroDataTable->item(row, MACRO_NAME_COLUMN);
            if (nameItem) {
                // Reuse existing item
                nameItem->setText(macroName);
                nameItem->setToolTip(macroName);
            }
            else {
                // Create new item (should not happen in normal operation)
                nameItem = new QTableWidgetItem(macroName);
                nameItem->setToolTip(macroName);
                macroDataTable->setItem(row, MACRO_NAME_COLUMN, nameItem);
            }
            break;
        }
        case MACRO_CONTENT_COLUMN: {
            QTableWidgetItem *contentItem = macroDataTable->item(row, MACRO_CONTENT_COLUMN);
            if (contentItem) {
                // Reuse existing item
                contentItem->setText(macroData.MappingMacro);
                contentItem->setToolTip(macroData.MappingMacro);
            }
            else {
                // Create new item (should not happen in normal operation)
                contentItem = new QTableWidgetItem(macroData.MappingMacro);
                contentItem->setToolTip(macroData.MappingMacro);
                macroDataTable->setItem(row, MACRO_CONTENT_COLUMN, contentItem);
            }
            break;
        }
        case MACRO_CATEGORY_COLUMN: {
            QTableWidgetItem *categoryItem = macroDataTable->item(row, MACRO_CATEGORY_COLUMN);
            if (categoryItem) {
                // Reuse existing item
                categoryItem->setText(macroData.Category);
                categoryItem->setToolTip(macroData.Category);
            }
            else {
                // Create new item (should not happen in normal operation)
                categoryItem = new QTableWidgetItem(macroData.Category);
                categoryItem->setToolTip(macroData.Category);
                macroDataTable->setItem(row, MACRO_CATEGORY_COLUMN, categoryItem);
            }
            break;
        }
        case MACRO_NOTE_COLUMN: {
            QTableWidgetItem *noteItem = macroDataTable->item(row, MACRO_NOTE_COLUMN);
            if (noteItem) {
                // Reuse existing item
                noteItem->setText(macroData.Note);
                noteItem->setToolTip(macroData.Note);
            }
            else {
                // Create new item (should not happen in normal operation)
                noteItem = new QTableWidgetItem(macroData.Note);
                noteItem->setToolTip(macroData.Note);
                macroDataTable->setItem(row, MACRO_NOTE_COLUMN, noteItem);
            }
            break;
        }
        default:
            break;
    }

    resizeMacroListTableColumnWidth(macroDataTable);
}

void MacroListTabWidget::keyPressEvent(QKeyEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[MacroListTabWidget::keyPressEvent]" << "Key:" << (Qt::Key)event->key() << "Modifiers:" << event->modifiers();
#endif

    QMacroListDialog *macroListDialog = QMacroListDialog::getInstance();
    if (macroListDialog && QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus) {
        if (event->key() == Qt::Key_Up) {
            if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
                // Move selected items to top when Ctrl+Shift is pressed
                macroListDialog->selectedMacroItemsMoveToTop();
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                // Move selected items up when Ctrl is pressed
                macroListDialog->selectedMacroItemsMoveUp();
            }
            else if (event->modifiers() & Qt::ShiftModifier) {
                // Extend or shrink selection upward when Shift is pressed
                macroListDialog->highlightSelectExtendUp();
            }
            else {
                // TableWidget highlight select up
                macroListDialog->highlightSelectUp();
            }
            return;
        }
        else if (event->key() == Qt::Key_Down) {
            if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
                // Move selected items to bottom when Ctrl+Shift is pressed
                macroListDialog->selectedMacroItemsMoveToBottom();
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                // Move selected items down when Ctrl is pressed
                macroListDialog->selectedMacroItemsMoveDown();
            }
            else if (event->modifiers() & Qt::ShiftModifier) {
                // Extend or shrink selection downward when Shift is pressed
                macroListDialog->highlightSelectExtendDown();
            }
            else {
                // TableWidget highlight select down
                macroListDialog->highlightSelectDown();
            }
            return;
        }
        else if (event->key() == Qt::Key_Delete) {
            // Delete selected items
            macroListDialog->deleteMacroSelectedItems();
            return;
        }
        else if (event->key() == Qt::Key_Home) {
            if (event->modifiers() & Qt::ControlModifier) {
                // Move selected items to top when Ctrl+Home is pressed
                macroListDialog->selectedMacroItemsMoveToTop();
            }
            else {
                // Select the first row
                macroListDialog->highlightSelectFirst();
            }
            return;
        }
        else if (event->key() == Qt::Key_End) {
            if (event->modifiers() & Qt::ControlModifier) {
                // Move selected items to bottom when Ctrl+End is pressed
                macroListDialog->selectedMacroItemsMoveToBottom();
            }
            else {
                // Select the last row
                macroListDialog->highlightSelectLast();
            }
            return;
        }
        else if ((event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Space)
            && (event->modifiers() == Qt::NoModifier)) {
            // Clear current selection
            macroListDialog->clearHighlightSelection();
            return;
        }
        else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            && (event->modifiers() == Qt::NoModifier)) {
            // Load selected macro data to editing fields when Enter/Return is pressed
            macroListDialog->highlightSelectLoadData();
            return;
        }
        else if (event->key() == Qt::Key_C && (event->modifiers() & Qt::ControlModifier)) {
            // Copy selected macro data to clipboard
            int copied_count = macroListDialog->copySelectedMacroDataToCopiedList();
            if (copied_count > 0) {
                QString message = tr("%1 selected macro(s) copied.").arg(copied_count);
                QKeyMapper::getInstance()->showInformationPopup(message);
                return;
            }
        }
        else if (event->key() == Qt::Key_V && (event->modifiers() & Qt::ControlModifier)) {
            // Paste macro data from clipboard
            int inserted_count = macroListDialog->insertMacroDataFromCopiedList();
            int copied_count = QMacroListDialog::s_CopiedMacroData.size();
            if (inserted_count == 0 && copied_count > 0) {
                QString message = tr("%1 copied macro(s) could not be inserted!").arg(copied_count);
                QKeyMapper::getInstance()->showFailurePopup(message);
            }
            else if (inserted_count > 0) {
                QString message = tr("Inserted %1 macro(s) into current macro list.").arg(inserted_count);
                QKeyMapper::getInstance()->showInformationPopup(message);
            }
            return;
        }
    }

    QTabWidget::keyPressEvent(event);
}

#if 0
void MacroListDataTableWidget::setCategoryFilter(const QString &category)
{
    m_CategoryFilters.clear();
    if (!category.isEmpty()) {
        if (category == tr("Blank")) {
            m_CategoryFilters.insert(QString());
        }
        else {
            m_CategoryFilters.insert(category);
        }
    }
    updateRowVisibility();
}
#endif

void MacroListDataTableWidget::setCategoryFilters(const QSet<QString> &categories)
{
    m_CategoryFilters = categories;
    updateRowVisibility();
}

void MacroListDataTableWidget::clearCategoryFilter()
{
    m_CategoryFilters.clear();
    updateRowVisibility();
}

void MacroListDataTableWidget::clearCategoryFilters()
{
    clearCategoryFilter();
}

QStringList MacroListDataTableWidget::getAvailableCategories() const
{
    QStringList categories;

    // Category column is always visible in macro list dialog
    bool hasNonEmptyCategories = false;
    bool hasEmptyCategories = false;

    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *categoryItem = item(row, MACRO_CATEGORY_COLUMN);
        QString category;

        if (categoryItem) {
            category = categoryItem->text().trimmed();
        }

        if (category.isEmpty()) {
            hasEmptyCategories = true;
        } else {
            hasNonEmptyCategories = true;
            if (!categories.contains(category)) {
#ifdef DEBUG_LOGOUT_ON
                // Avoid confusion with the built-in "All" option
                // Users can still create an "All" category, but we'll warn about it
                if (category == tr("All")) {
                    qDebug() << "[getAvailableCategories]" << "Warning: Found user-created category named 'All', which may cause confusion with the built-in 'All' option";
                }
#endif
                categories.append(category);
            }
        }
    }

    // Add "(Blanks)" option only if there are both non-empty and empty categories
    // If all categories are empty, showing "(Blanks)" would be the same as "All"
    if (hasEmptyCategories && hasNonEmptyCategories) {
        categories.append(tr("Blank"));
    }

    return categories;
}

void MacroListDataTableWidget::keyPressEvent(QKeyEvent *event)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[MacroListDataTableWidget::keyPressEvent]" << "Key:" << (Qt::Key)event->key() << "Modifiers:" << event->modifiers();
#endif

    QMacroListDialog *macroListDialog = QMacroListDialog::getInstance();
    if (macroListDialog && QKeyMapper::KEYMAP_IDLE == QKeyMapper::getInstance()->m_KeyMapStatus) {
        if (event->key() == Qt::Key_Up) {
            if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
                // Move selected items to top when Ctrl+Shift is pressed
                macroListDialog->selectedMacroItemsMoveToTop();
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                // Move selected items up when Ctrl is pressed
                macroListDialog->selectedMacroItemsMoveUp();
            }
            else if (event->modifiers() & Qt::ShiftModifier) {
                // Extend or shrink selection upward when Shift is pressed
                macroListDialog->highlightSelectExtendUp();
            }
            else {
                // TableWidget highlight select up
                macroListDialog->highlightSelectUp();
            }
            return;
        }
        else if (event->key() == Qt::Key_Down) {
            if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
                // Move selected items to bottom when Ctrl+Shift is pressed
                macroListDialog->selectedMacroItemsMoveToBottom();
            }
            else if (event->modifiers() & Qt::ControlModifier) {
                // Move selected items down when Ctrl is pressed
                macroListDialog->selectedMacroItemsMoveDown();
            }
            else if (event->modifiers() & Qt::ShiftModifier) {
                // Extend or shrink selection downward when Shift is pressed
                macroListDialog->highlightSelectExtendDown();
            }
            else {
                // TableWidget highlight select down
                macroListDialog->highlightSelectDown();
            }
            return;
        }
        else if (event->key() == Qt::Key_Delete) {
            // Delete selected items
            macroListDialog->deleteMacroSelectedItems();
            return;
        }
        else if (event->key() == Qt::Key_Home) {
            if (event->modifiers() & Qt::ControlModifier) {
                // Move selected items to top when Ctrl+Home is pressed
                macroListDialog->selectedMacroItemsMoveToTop();
            }
            else {
                // Select the first row
                macroListDialog->highlightSelectFirst();
            }
            return;
        }
        else if (event->key() == Qt::Key_End) {
            if (event->modifiers() & Qt::ControlModifier) {
                // Move selected items to bottom when Ctrl+End is pressed
                macroListDialog->selectedMacroItemsMoveToBottom();
            }
            else {
                // Select the last row
                macroListDialog->highlightSelectLast();
            }
            return;
        }
        else if ((event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Space)
                 && (event->modifiers() == Qt::NoModifier)) {
            // Clear current selection
            macroListDialog->clearHighlightSelection();
            return;
        }
        else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
                 && (event->modifiers() == Qt::NoModifier)) {
            // Load selected macro data to editing fields when Enter/Return is pressed
            macroListDialog->highlightSelectLoadData();
            return;
        }
        else if (event->key() == Qt::Key_C && (event->modifiers() & Qt::ControlModifier)) {
            // Copy selected macro data to clipboard
            int copied_count = macroListDialog->copySelectedMacroDataToCopiedList();
            if (copied_count > 0) {
                QString message = tr("%1 selected macro(s) copied.").arg(copied_count);
                QKeyMapper::getInstance()->showInformationPopup(message);
                return;
            }
        }
        else if (event->key() == Qt::Key_V && (event->modifiers() & Qt::ControlModifier)) {
            // Paste macro data from clipboard
            int inserted_count = macroListDialog->insertMacroDataFromCopiedList();
            int copied_count = QMacroListDialog::s_CopiedMacroData.size();
            if (inserted_count == 0 && copied_count > 0) {
                QString message = tr("%1 copied macro(s) could not be inserted!").arg(copied_count);
                QKeyMapper::getInstance()->showFailurePopup(message);
            }
            else if (inserted_count > 0) {
                QString message = tr("Inserted %1 macro(s) into current macro list.").arg(inserted_count);
                QKeyMapper::getInstance()->showInformationPopup(message);
            }
            return;
        }
    }

    QTableWidget::keyPressEvent(event);
}

void MacroListDataTableWidget::startDrag(Qt::DropActions supportedActions)
{
    QList<QTableWidgetSelectionRange> selectedRanges = this->selectedRanges();
    if (!selectedRanges.isEmpty()) {
        QTableWidgetSelectionRange range = selectedRanges.first();
        m_DraggedTopRow = range.topRow();
        m_DraggedBottomRow = range.bottomRow();
    }
    QTableWidget::startDrag(supportedActions);
}

void MacroListDataTableWidget::dropEvent(QDropEvent *event)
{
    if (event->dropAction() == Qt::MoveAction) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        int droppedRow = rowAt(event->position().toPoint().y());
#else
        int droppedRow = rowAt(event->pos().y());
#endif

        if (droppedRow < 0) {
            droppedRow = rowCount() - 1;
        }

        // Emit signal to handle macro list reordering
        emit QMacroListDialog::getInstance()->macroListTableDragDropMove_Signal(m_DraggedTopRow, m_DraggedBottomRow, droppedRow);
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MacroListDataTableWidget::dropEvent]" << "Drag from" << m_DraggedTopRow << "to" << m_DraggedBottomRow << "dropped at" << droppedRow;
#endif
    }
}

void MacroListDataTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (QMacroListDialog::getInstance()->isMacroDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the macro table is filtered!");
        QKeyMapper::getInstance()->showWarningPopup(message);
        event->ignore();
    } else {
        QTableWidget::dragEnterEvent(event);
    }
}

void MacroListDataTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (QMacroListDialog::getInstance()->isMacroDataTableFiltered()) {
        event->ignore();
    } else {
        QTableWidget::dragMoveEvent(event);
    }
}

void MacroListDataTableWidget::updateRowVisibility()
{
    if (m_CategoryFilters.isEmpty()) {
        // Show all rows when no filter is active
        for (int row = 0; row < rowCount(); ++row) {
            setRowHidden(row, false);
        }
        return;
    }

    // Filter rows based on category
    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *categoryItem = item(row, MACRO_CATEGORY_COLUMN);
        QString itemCategory;
        if (categoryItem) {
            itemCategory = categoryItem->text().trimmed();
        }
        if (itemCategory.isEmpty()) {
            const bool shouldShow = m_CategoryFilters.contains(QString());
            setRowHidden(row, !shouldShow);
            continue;
        }

        const bool shouldShow = m_CategoryFilters.contains(itemCategory);

        setRowHidden(row, !shouldShow);
    }
}

// Helper method to get the current macro data table based on active tab
MacroListDataTableWidget* QMacroListDialog::getCurrentMacroDataTable()
{
    if (ui->macroListTabWidget->currentWidget() == ui->macrolist) {
        return ui->macrolistTable;
    }
    else if (ui->macroListTabWidget->currentWidget() == ui->universalmacrolist) {
        return ui->universalmacrolistTable;
    }
    return Q_NULLPTR;
}

// Helper method to get the current macro data list based on active tab
OrderedMap<QString, MappingMacroData>* QMacroListDialog::getCurrentMacroDataList()
{
    if (ui->macroListTabWidget->currentWidget() == ui->macrolist) {
        return &QKeyMapper::s_MappingMacroList;
    }
    else if (ui->macroListTabWidget->currentWidget() == ui->universalmacrolist) {
        return &QKeyMapper::s_UniversalMappingMacroList;
    }
    return Q_NULLPTR;
}

void QMacroListDialog::macroTableItemDoubleClicked(QTableWidgetItem *item)
{
    if (item == Q_NULLPTR) {
        return;
    }

    int rowindex = item->row();
    int columnindex = item->column();
    Q_UNUSED(columnindex);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[macroTableItemDoubleClicked]" << "Row" << rowindex << "Column" << columnindex << "DoubleClicked";
#endif

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable) {
        return;
    }

    Qt::MouseButtons buttons = QApplication::mouseButtons();
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

    int editmode = QKeyMapper::getEditModeIndex();
    bool load_data = true;
    if (editmode == EDITMODE_LEFT_DOUBLECLICK) {
        if (buttons & Qt::LeftButton && !(buttons & Qt::RightButton)) {
            if (modifiers & Qt::AltModifier) {
                load_data = true;
            }
            else {
                load_data = false;
            }
        }
    }
    else {
        if (buttons & Qt::RightButton && !(buttons & Qt::LeftButton)) {
            load_data = false;
        }
        else if (modifiers & Qt::AltModifier) {
            load_data = false;
        }
    }

    if (load_data) {
        // Temporarily disable edit triggers
        macroDataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // For Name and Macro columns, load data to LineEdit controls
        QTableWidgetItem *nameItem = macroDataTable->item(rowindex, MACRO_NAME_COLUMN);
        QTableWidgetItem *contentItem = macroDataTable->item(rowindex, MACRO_CONTENT_COLUMN);
        QTableWidgetItem *categoryItem = macroDataTable->item(rowindex, MACRO_CATEGORY_COLUMN);
        QTableWidgetItem *noteItem = macroDataTable->item(rowindex, MACRO_NOTE_COLUMN);

        if (nameItem) {
            ui->macroNameLineEdit->setText(nameItem->text());
        }
        if (contentItem) {
            ui->macroContentLineEdit->setText(contentItem->text());
        }
        if (categoryItem) {
            ui->categoryLineEdit->setText(categoryItem->text());
        }
        if (noteItem) {
            ui->macroNoteLineEdit->setText(noteItem->text());
        }

        // Restore edit triggers after event processing
        QTimer::singleShot(0, this, [=]() {
            macroDataTable->setEditTriggers(QAbstractItemView::DoubleClicked);
        });

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[macroTableItemDoubleClicked]" << "Loaded macro data to LineEdit controls";
#endif
    }
    else {
        macroDataTable->editItem(item);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[macroTableItemDoubleClicked]" << "Entering edit mode";
#endif
    }
}

void QMacroListDialog::macroListTabWidgetCurrentChanged(int index)
{
    Q_UNUSED(index);
    updateMacroCategoryFilterComboBox();
}

void QMacroListDialog::macroTableCellChanged(int row, int column)
{
    // Only handle valid column changes
    if (column < MACRO_NAME_COLUMN || column > MACRO_NOTE_COLUMN) {
        return;
    }

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    int row_count = macroDataList->size();

    if (row >= row_count || row < 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug("\033[1;31m[%s]: row(%d) out of range, row_count(%d), col(%d)\033[0m", __func__, row, row_count, column);
#endif
        return;
    }

    QTableWidgetItem *nameItem = macroDataTable->item(row, MACRO_NAME_COLUMN);
    if (!nameItem) {
        return;
    }

    // Block signals to prevent triggering cellChanged during update
    QSignalBlocker blocker(macroDataTable);

    QList<QString> macroNameList = macroDataList->keys();
    QString macroName = macroNameList.at(row);

    static QRegularExpression simplified_regex(R"([\r\n]+)");
    if (column == MACRO_NAME_COLUMN) {
        QString newMacroName = nameItem->text();
        if (newMacroName != macroName) {
            newMacroName.replace(simplified_regex, " ");
            newMacroName = newMacroName.trimmed();
            if (newMacroName != macroName) {
                QString popupMessage;
                QString popupMessageColor;
                int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;

                if (newMacroName.isEmpty()) {
                    macroDataTable->item(row, column)->setText(macroName);

                    popupMessageColor = FAILURE_COLOR;
                    popupMessage = tr("Macro name cannot be empty.");
                    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
                    return;
                }
                else if (newMacroName.contains(')')) {
                    macroDataTable->item(row, column)->setText(macroName);

                    popupMessageColor = FAILURE_COLOR;
                    popupMessage = tr("Macro name cannot contain ')' character.");
                    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
                    return;
                }
                else if (macroNameList.contains(newMacroName)) {
                    macroDataTable->item(row, column)->setText(macroName);

                    popupMessageColor = FAILURE_COLOR;
                    popupMessage = tr("Macro name \"%1\" already exists.").arg(newMacroName);
                    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
                    return;
                }
                else {
                    // Replace the key in OrderedMap while preserving value and order
                    // Using the efficient replaceKey method (O(1) average complexity)
                    bool success = macroDataList->replaceKey(macroName, newMacroName);

                    if (!success) {
                        // This should not happen if validation above is correct
                        macroDataTable->item(row, column)->setText(macroName);
#ifdef DEBUG_LOGOUT_ON
                        qDebug() << "[macroTableCellChanged]" << "replaceKey failed for:" << macroName << "->" << newMacroName;
#endif
                        return;
                    }

#ifdef DEBUG_LOGOUT_ON
                    qDebug() << "[macroTableCellChanged]" << "Updated macro name from:" << macroName << "to:" << newMacroName << "at row:" << row;
#endif

                    // Update MacroList Table display for the macro name column
                    updateMacroListTableItem(macroDataTable, macroDataList, row, MACRO_NAME_COLUMN);

                    // Show success message
                    popupMessageColor = SUCCESS_COLOR;
                    popupMessage = tr("Macro name updated from \"%1\" to \"%2\"").arg(macroName, newMacroName);
                    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
                }
            }
            else {
                macroDataTable->item(row, column)->setText(macroName);

#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[QMacroListDialog::%1] row(%2) MacroName unchanged: \"%3\"").arg(__func__).arg(row).arg(newMacroName);
                qDebug().noquote().nospace() << debugmessage;
#endif
            }
        }
    }
    else if (column == MACRO_CONTENT_COLUMN) {
        QTableWidgetItem *macroContentItem = macroDataTable->item(row, MACRO_CONTENT_COLUMN);
        if (!macroContentItem) {
            return;
        }
        QString newMacroContent = macroContentItem->text();
        QString macroContent = macroDataList->value(macroName).MappingMacro;
        if (newMacroContent != macroContent) {
            ValidationResult result = QKeyMapper::validateMappingKeyString(newMacroContent);
            if (!result.isValid) {
                QString popupMessage;
                QString popupMessageColor;
                int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;

                macroDataTable->item(row, column)->setText(macroContent);

                popupMessageColor = FAILURE_COLOR;
                popupMessage = tr("Macro") + " -> " + result.errorMessage;
                emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
                return;
            }
            else {
                // Update the category in the data list
                (*macroDataList)[macroName].MappingMacro = newMacroContent;

                // Update MacroList Table display for the macro content column
                updateMacroListTableItem(macroDataTable, macroDataList, row, MACRO_CONTENT_COLUMN);

#ifdef DEBUG_LOGOUT_ON
                QString debugmessage = QString("[QMacroListDialog::%1] row(%2) MacroContent changed : \"%3\"").arg(__func__).arg(row).arg(newMacroContent);
                qDebug().noquote().nospace() << debugmessage;
#endif
            }

        }
    }
    else if (column == MACRO_CATEGORY_COLUMN) {
        QTableWidgetItem *categoryItem = macroDataTable->item(row, MACRO_CATEGORY_COLUMN);
        if (!categoryItem) {
            return;
        }
        QString newCategory = categoryItem->text();
        newCategory.replace(simplified_regex, " ");
        newCategory = newCategory.trimmed();

        // Update the category in the data list
        (*macroDataList)[macroName].Category = newCategory;

        // Update MacroList Table display for the macro category column
        updateMacroListTableItem(macroDataTable, macroDataList, row, MACRO_CATEGORY_COLUMN);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[macroTableCellChanged]" << "Updated category for macro:" << macroName << "to:" << newCategory;
#endif
        // Update the category filter ComboBox
        updateMacroCategoryFilterComboBox();
    }
    else if (column == MACRO_NOTE_COLUMN) {
        QTableWidgetItem *noteItem = macroDataTable->item(row, MACRO_NOTE_COLUMN);
        if (!noteItem) {
            return;
        }
        QString newNote = noteItem->text();
        newNote.replace(simplified_regex, " ");
        newNote = newNote.trimmed();

        // Update the note in the data list
        (*macroDataList)[macroName].Note = newNote;

        // Update MacroList Table display for the macro note column
        updateMacroListTableItem(macroDataTable, macroDataList, row, MACRO_NOTE_COLUMN);

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[macroTableCellChanged]" << "Updated note for macro:" << macroName << "to:" << newNote;
#endif
    }
}

void QMacroListDialog::macroTableItemSelectionChanged()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable) {
        return;
    }

    // Check if selection is empty
    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
        // Get current cell
        int currentRow = macroDataTable->currentRow();
        int currentColumn = macroDataTable->currentColumn();

        // If current cell is valid (not -1, -1), clear it to avoid unexpected Ctrl/Shift+Click selection behavior
        if (currentRow != -1 || currentColumn != -1) {
            macroDataTable->setCurrentCell(-1, -1);
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[macroTableItemSelectionChanged] Selection is empty, cleared CurrentCell from (" << currentRow << "," << currentColumn << ") to (-1, -1)";
#endif
        }
    }
}

#ifdef DEBUG_LOGOUT_ON
void QMacroListDialog::macroTableCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    qDebug() << "[QMacroListDialog::macroTableCurrentCellChanged]" << "Current Cell: (" << currentRow << "," << currentColumn << "), Previous Cell: (" << previousRow << "," << previousColumn << ")";
}
#endif

void QMacroListDialog::macroListTableDragDropMove(int top_row, int bottom_row, int dragged_to)
{
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[macroListTableDragDropMove] DragDrop : Rows" << top_row << ":" << bottom_row << "->" << dragged_to;
#endif

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    int mappingdata_size = macroDataList->size();
    if (top_row >= 0 && bottom_row < mappingdata_size && dragged_to >= 0 && dragged_to < mappingdata_size
        && (dragged_to > bottom_row || dragged_to < top_row)) {

        int draged_row_count = bottom_row - top_row + 1;
        bool isDraggedToBottom = (dragged_to > bottom_row);

        // Get all keys in order
        QList<QString> keysList = macroDataList->keys();

        // Extract keys to be moved
        QList<QString> movedKeys;
        for (int i = top_row; i <= bottom_row; ++i) {
            movedKeys.append(keysList.at(i));
        }

        // Remove moved keys from list
        for (int i = bottom_row; i >= top_row; --i) {
            keysList.removeAt(i);
        }

        // Calculate insert position
        int insertPos;
        if (isDraggedToBottom) {
            insertPos = dragged_to - draged_row_count + 1;
        } else {
            insertPos = dragged_to;
        }

        // Insert moved keys at new position
        for (int i = 0; i < movedKeys.size(); ++i) {
            keysList.insert(insertPos + i, movedKeys.at(i));
        }

        // Rebuild the OrderedMap with new order
        OrderedMap<QString, MappingMacroData> newMacroList;
        for (const QString &key : std::as_const(keysList)) {
            newMacroList[key] = macroDataList->value(key);
        }
        *macroDataList = newMacroList;

#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[macroListTableDragDropMove] : refreshMacroListTabWidget()";
#endif
        refreshMacroListTabWidget(macroDataTable, *macroDataList);

        // Reselect the moved rows
        QTableWidgetSelectionRange newSelection;
        if (isDraggedToBottom) {
            newSelection = QTableWidgetSelectionRange(dragged_to - draged_row_count + 1, 0, dragged_to, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
        } else {
            newSelection = QTableWidgetSelectionRange(dragged_to, 0, dragged_to + draged_row_count - 1, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
        }
        macroDataTable->clearSelection();
        macroDataTable->setRangeSelected(newSelection, true);

        // Update current cell based on drag direction for Ctrl/Shift+Click consistency
        if (isDraggedToBottom) {
            macroDataTable->setCurrentCell(newSelection.bottomRow(), 0, QItemSelectionModel::NoUpdate);
        } else {
            macroDataTable->setCurrentCell(newSelection.topRow(), 0, QItemSelectionModel::NoUpdate);
        }

#ifdef DEBUG_LOGOUT_ON
        if (macroDataTable->rowCount() != macroDataList->size()) {
            qDebug("macroListTableDragDropMove : MacroData sync error!!! DataTableSize(%d), DataListSize(%d)", macroDataTable->rowCount(), macroDataList->size());
        }
#endif
    }
}

void QMacroListDialog::highlightSelectUp()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable || macroDataTable->rowCount() <= 0) {
        return;
    }

    bool isFiltered = isMacroDataTableFiltered();

    // Get current selection ranges
    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectUp] No selected item, selecting last visible row";
#endif
        // Select the last visible row when no selection exists
        highlightSelectLast();
        return;
    }

    // Find the topmost visible selected row
    int topVisibleSelectedRow = -1;
    for (const QTableWidgetSelectionRange &range : std::as_const(selectedRanges)) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            if (!macroDataTable->isRowHidden(row)) {
                if (topVisibleSelectedRow == -1 || row < topVisibleSelectedRow) {
                    topVisibleSelectedRow = row;
                }
                break;
            }
        }
    }

    if (topVisibleSelectedRow == -1) {
        // No visible selected row found
        highlightSelectLast();
        return;
    }

    // Find the previous visible row
    int newSelectedRow = -1;
    for (int row = topVisibleSelectedRow - 1; row >= 0; --row) {
        if (!macroDataTable->isRowHidden(row)) {
            newSelectedRow = row;
            break;
        }
    }

    if (newSelectedRow == -1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectUp] Already at the top visible row, cannot select up";
#endif
        return;
    }

    // Clear current selection
    macroDataTable->clearSelection();

    // Select the row above
    QTableWidgetSelectionRange newSelection(newSelectedRow, 0, newSelectedRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to match the new selection for Ctrl/Shift+Click consistency
    macroDataTable->setCurrentCell(newSelectedRow, 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the selected row visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newSelectedRow, 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[highlightSelectUp] Selected row" << newSelectedRow << "(filtered:" << isFiltered << ")";
#endif
    Q_UNUSED(isFiltered);
}

void QMacroListDialog::highlightSelectDown()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable || macroDataTable->rowCount() <= 0) {
        return;
    }

    bool isFiltered = isMacroDataTableFiltered();

    // Get current selection ranges
    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectDown] No selected item, selecting first visible row";
#endif
        // Select the first visible row when no selection exists
        highlightSelectFirst();
        return;
    }

    // Find the bottommost visible selected row
    int bottomVisibleSelectedRow = -1;
    for (const QTableWidgetSelectionRange &range : std::as_const(selectedRanges)) {
        for (int row = range.bottomRow(); row >= range.topRow(); --row) {
            if (!macroDataTable->isRowHidden(row)) {
                if (bottomVisibleSelectedRow == -1 || row > bottomVisibleSelectedRow) {
                    bottomVisibleSelectedRow = row;
                }
                break;
            }
        }
    }

    if (bottomVisibleSelectedRow == -1) {
        // No visible selected row found
        highlightSelectFirst();
        return;
    }

    // Find the next visible row
    int newSelectedRow = -1;
    int rowCount = macroDataTable->rowCount();
    for (int row = bottomVisibleSelectedRow + 1; row < rowCount; ++row) {
        if (!macroDataTable->isRowHidden(row)) {
            newSelectedRow = row;
            break;
        }
    }

    if (newSelectedRow == -1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectDown] Already at the bottom visible row, cannot select down";
#endif
        return;
    }

    // Clear current selection
    macroDataTable->clearSelection();

    // Select the row below
    QTableWidgetSelectionRange newSelection(newSelectedRow, 0, newSelectedRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to match the new selection for Ctrl/Shift+Click consistency
    macroDataTable->setCurrentCell(newSelectedRow, 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the selected row visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newSelectedRow, 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[highlightSelectDown] Selected row" << newSelectedRow << "(filtered:" << isFiltered << ")";
#endif
    Q_UNUSED(isFiltered);
}

void QMacroListDialog::highlightSelectExtendUp()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable || macroDataTable->rowCount() <= 0) {
        return;
    }

    bool isFiltered = isMacroDataTableFiltered();

    // Get current selection ranges
    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectExtendUp] No selection, delegate to highlightSelectUp";
#endif
        // No selection exists, behave like normal Up key
        highlightSelectUp();
        return;
    }

    // Get selection boundaries
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();
    int currentRow = macroDataTable->currentRow();

    // Find the topmost and bottommost visible rows in current selection
    int topVisibleRow = -1;
    int bottomVisibleRow = -1;
    for (int row = topRow; row <= bottomRow; ++row) {
        if (!macroDataTable->isRowHidden(row)) {
            if (topVisibleRow == -1) {
                topVisibleRow = row;
            }
            bottomVisibleRow = row;
        }
    }

    // If no visible row in selection, delegate to highlightSelectUp
    if (topVisibleRow == -1) {
        highlightSelectUp();
        return;
    }

    // Determine the anchor point (the row that stays fixed during Shift selection)
    // In Windows standard behavior:
    // - If currentRow is at or near the top of selection, we extend upward from top
    // - If currentRow is at or near the bottom of selection, we shrink from bottom
    bool currentAtTop = (currentRow <= topVisibleRow);
    bool currentAtBottom = (currentRow >= bottomVisibleRow);

    int newTopRow = topRow;
    int newBottomRow = bottomRow;
    int newCurrentRow = currentRow;

    if (currentAtTop || (!currentAtTop && !currentAtBottom)) {
        // Current is at top or in middle - extend selection upward to next visible row
        int nextVisibleRowUp = -1;
        for (int row = topRow - 1; row >= 0; --row) {
            if (!macroDataTable->isRowHidden(row)) {
                nextVisibleRowUp = row;
                break;
            }
        }

        if (nextVisibleRowUp == -1) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[highlightSelectExtendUp] Already at top visible boundary, cannot extend up";
#endif
            return;
        }
        // Extend selection to include all rows up to (and including) the next visible row
        newTopRow = nextVisibleRowUp;
        newCurrentRow = nextVisibleRowUp;
    }
    else if (currentAtBottom) {
        // Current is at bottom - shrink selection from bottom to previous visible row
        if (topVisibleRow >= bottomVisibleRow) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[highlightSelectExtendUp] Selection has only one visible row, cannot shrink";
#endif
            return;
        }
        // Find the second-to-last visible row
        int prevVisibleRow = -1;
        for (int row = bottomRow - 1; row >= topRow; --row) {
            if (!macroDataTable->isRowHidden(row)) {
                prevVisibleRow = row;
                break;
            }
        }
        if (prevVisibleRow == -1 || prevVisibleRow < topRow) {
            return;
        }
        newBottomRow = prevVisibleRow;
        newCurrentRow = prevVisibleRow;
    }

    // Apply new selection
    macroDataTable->clearSelection();
    QTableWidgetSelectionRange newSelection(newTopRow, 0, newBottomRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to the edge that moved
    macroDataTable->setCurrentCell(newCurrentRow, 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the current row visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newCurrentRow, 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[highlightSelectExtendUp] Selection range:" << newTopRow << "-" << newBottomRow << ", currentRow:" << newCurrentRow << "(filtered:" << isFiltered << ")";
#endif
    Q_UNUSED(isFiltered);
}

void QMacroListDialog::highlightSelectExtendDown()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable || macroDataTable->rowCount() <= 0) {
        return;
    }

    bool isFiltered = isMacroDataTableFiltered();
    int rowCount = macroDataTable->rowCount();

    // Get current selection ranges
    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectExtendDown] No selection, delegate to highlightSelectDown";
#endif
        // No selection exists, behave like normal Down key
        highlightSelectDown();
        return;
    }

    // Get selection boundaries
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();
    int currentRow = macroDataTable->currentRow();

    // Find the topmost and bottommost visible rows in current selection
    int topVisibleRow = -1;
    int bottomVisibleRow = -1;
    for (int row = topRow; row <= bottomRow; ++row) {
        if (!macroDataTable->isRowHidden(row)) {
            if (topVisibleRow == -1) {
                topVisibleRow = row;
            }
            bottomVisibleRow = row;
        }
    }

    // If no visible row in selection, delegate to highlightSelectDown
    if (bottomVisibleRow == -1) {
        highlightSelectDown();
        return;
    }

    // Determine the anchor point (the row that stays fixed during Shift selection)
    // In Windows standard behavior:
    // - If currentRow is at or near the bottom of selection, we extend downward
    // - If currentRow is at or near the top of selection, we shrink from top
    bool currentAtTop = (currentRow <= topVisibleRow);
    bool currentAtBottom = (currentRow >= bottomVisibleRow);

    int newTopRow = topRow;
    int newBottomRow = bottomRow;
    int newCurrentRow = currentRow;

    if (currentAtBottom || (!currentAtTop && !currentAtBottom)) {
        // Current is at bottom or in middle - extend selection downward to next visible row
        int nextVisibleRowDown = -1;
        for (int row = bottomRow + 1; row < rowCount; ++row) {
            if (!macroDataTable->isRowHidden(row)) {
                nextVisibleRowDown = row;
                break;
            }
        }

        if (nextVisibleRowDown == -1) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[highlightSelectExtendDown] Already at bottom visible boundary, cannot extend down";
#endif
            return;
        }
        // Extend selection to include all rows down to (and including) the next visible row
        newBottomRow = nextVisibleRowDown;
        newCurrentRow = nextVisibleRowDown;
    }
    else if (currentAtTop) {
        // Current is at top - shrink selection from top to next visible row
        if (topVisibleRow >= bottomVisibleRow) {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "[highlightSelectExtendDown] Selection has only one visible row, cannot shrink";
#endif
            return;
        }
        // Find the second visible row from top
        int nextVisibleRow = -1;
        for (int row = topRow + 1; row <= bottomRow; ++row) {
            if (!macroDataTable->isRowHidden(row)) {
                nextVisibleRow = row;
                break;
            }
        }
        if (nextVisibleRow == -1 || nextVisibleRow > bottomRow) {
            return;
        }
        newTopRow = nextVisibleRow;
        newCurrentRow = nextVisibleRow;
    }

    // Apply new selection
    macroDataTable->clearSelection();
    QTableWidgetSelectionRange newSelection(newTopRow, 0, newBottomRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to the edge that moved
    macroDataTable->setCurrentCell(newCurrentRow, 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the current row visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newCurrentRow, 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[highlightSelectExtendDown] Selection range:" << newTopRow << "-" << newBottomRow << ", currentRow:" << newCurrentRow << "(filtered:" << isFiltered << ")";
#endif
    Q_UNUSED(isFiltered);
}

void QMacroListDialog::highlightSelectFirst()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable || macroDataTable->rowCount() <= 0) {
        return;
    }

    bool isFiltered = isMacroDataTableFiltered();

    // Clear current selection
    macroDataTable->clearSelection();

    // Find the first visible row
    int newSelectedRow = -1;
    int rowCount = macroDataTable->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        if (!macroDataTable->isRowHidden(row)) {
            newSelectedRow = row;
            break;
        }
    }

    if (newSelectedRow == -1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectFirst] No visible row found";
#endif
        return;
    }

    // Select the first visible row
    QTableWidgetSelectionRange newSelection(newSelectedRow, 0, newSelectedRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to match the new selection for Ctrl/Shift+Click consistency
    macroDataTable->setCurrentCell(newSelectedRow, 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the selected row visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newSelectedRow, 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[highlightSelectFirst] Selected first visible row" << newSelectedRow << "(filtered:" << isFiltered << ")";
#endif
    Q_UNUSED(isFiltered);
}

void QMacroListDialog::highlightSelectLast()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable || macroDataTable->rowCount() <= 0) {
        return;
    }

    bool isFiltered = isMacroDataTableFiltered();

    // Clear current selection
    macroDataTable->clearSelection();

    // Find the last visible row
    int newSelectedRow = -1;
    for (int row = macroDataTable->rowCount() - 1; row >= 0; --row) {
        if (!macroDataTable->isRowHidden(row)) {
            newSelectedRow = row;
            break;
        }
    }

    if (newSelectedRow == -1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectLast] No visible row found";
#endif
        return;
    }

    // Select the last visible row
    QTableWidgetSelectionRange newSelection(newSelectedRow, 0, newSelectedRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to match the new selection for Ctrl/Shift+Click consistency
    macroDataTable->setCurrentCell(newSelectedRow, 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the selected row visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newSelectedRow, 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[highlightSelectLast] Selected last visible row" << newSelectedRow << "(filtered:" << isFiltered << ")";
#endif
    Q_UNUSED(isFiltered);
}

void QMacroListDialog::clearHighlightSelection()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable) {
        return;
    }

    macroDataTable->clearSelection();

    // Clear current cell to avoid unexpected Ctrl/Shift+Click selection behavior
    macroDataTable->setCurrentCell(-1, -1);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[clearHighlightSelection] Cleared selection";
#endif
}

void QMacroListDialog::highlightSelectLoadData()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    if (!macroDataTable) {
        return;
    }

    // Get current selection ranges
    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();

    // Check if no selection
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectLoadData] No selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

    // Check if exactly one row is selected
    if (topRow != bottomRow) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[highlightSelectLoadData] Multiple rows selected, topRow:" << topRow << ", bottomRow:" << bottomRow;
#endif
        return;
    }

    // Load macro data from the selected row to LineEdit controls
    int rowindex = topRow;

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[highlightSelectLoadData] Loading macro data from row:" << rowindex;
#endif

    // Get macro data from the selected row
    QTableWidgetItem *nameItem = macroDataTable->item(rowindex, MACRO_NAME_COLUMN);
    QTableWidgetItem *contentItem = macroDataTable->item(rowindex, MACRO_CONTENT_COLUMN);
    QTableWidgetItem *categoryItem = macroDataTable->item(rowindex, MACRO_CATEGORY_COLUMN);
    QTableWidgetItem *noteItem = macroDataTable->item(rowindex, MACRO_NOTE_COLUMN);

    if (nameItem) {
        ui->macroNameLineEdit->setText(nameItem->text());
    }
    if (contentItem) {
        ui->macroContentLineEdit->setText(contentItem->text());
    }
    if (categoryItem) {
        ui->categoryLineEdit->setText(categoryItem->text());
    }
    if (noteItem) {
        ui->macroNoteLineEdit->setText(noteItem->text());
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[highlightSelectLoadData] Loaded macro data to LineEdit controls";
#endif
}

int QMacroListDialog::copySelectedMacroDataToCopiedList()
{
    int copied_count = -1;

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return copied_count;
    }

    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[copySelectedMacroDataToCopiedList] There is no selected item";
#endif
        return copied_count;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int top_row = range.topRow();
    int bottom_row = range.bottomRow();

    // Clear and populate the copied macro data (only visible rows)
    s_CopiedMacroData.clear();
    QList<QString> macroNameList = macroDataList->keys();
    for (int row = top_row; row <= bottom_row; ++row) {
        // Skip hidden rows (filtered out rows)
        if (macroDataTable->isRowHidden(row)) {
            continue;
        }
        if (row < macroNameList.size()) {
            QString macroName = macroNameList.at(row);
            s_CopiedMacroData[macroName] = macroDataList->value(macroName);
        }
    }
    copied_count = s_CopiedMacroData.size();

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace() << "[copySelectedMacroDataToCopiedList] Ctrl+C pressed, copy(" << copied_count << ") visible selected macro data to s_CopiedMacroData";
#endif

    return copied_count;
}

int QMacroListDialog::insertMacroDataFromCopiedList()
{
    int inserted_count = 0;

    if (s_CopiedMacroData.isEmpty()) {
        return inserted_count;
    }

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return inserted_count;
    }

    // Build list of macro data to insert, handling duplicate names
    OrderedMap<QString, MappingMacroData> insertMacroDataList;
    for (auto it = s_CopiedMacroData.begin(); it != s_CopiedMacroData.end(); ++it) {
        QString macroName = it.key();
        MappingMacroData macroData = it.value();

        // Check if macro name already exists
        if (macroDataList->contains(macroName)) {
            // Generate a unique name using "_copy" suffix
            QString baseName = macroName + tr("_copy");
            QString newName = baseName;

            // If the base name with "_copy" also exists, try adding numbers
            if (macroDataList->contains(newName) || insertMacroDataList.contains(newName)) {
                bool uniqueNameFound = false;
                for (int i = 1; i <= 999; ++i) {
                    QString tempName = QString("%1%2").arg(baseName).arg(i, 3, 10, QChar('0'));
                    if (!macroDataList->contains(tempName) && !insertMacroDataList.contains(tempName)) {
#ifdef DEBUG_LOGOUT_ON
                        qDebug().nospace() << "[insertMacroDataFromCopiedList] MacroName:" << macroName << " already exists, generated unique name: " << tempName;
#endif
                        newName = tempName;
                        uniqueNameFound = true;
                        break;
                    }
                }
                // If no unique name found after 999 attempts, skip this macro
                if (!uniqueNameFound) {
#ifdef DEBUG_LOGOUT_ON
                    qDebug().nospace() << "[insertMacroDataFromCopiedList] Cannot find unique name for MacroName:" << macroName << ", skipping";
#endif
                    continue;
                }
            }
#ifdef DEBUG_LOGOUT_ON
            else {
                qDebug().nospace() << "[insertMacroDataFromCopiedList] MacroName:" << macroName << " already exists, using new name: " << newName;
            }
#endif
            macroName = newName;
        }

        // Also check if the new name already exists in our insert list
        if (insertMacroDataList.contains(macroName)) {
            QString baseName = macroName + tr("_copy");
            QString newName = baseName;
            if (insertMacroDataList.contains(newName)) {
                bool uniqueNameFound = false;
                for (int i = 1; i <= 999; ++i) {
                    QString tempName = QString("%1(%2)").arg(baseName).arg(i, 3, 10, QChar('0'));
                    if (!macroDataList->contains(tempName) && !insertMacroDataList.contains(tempName)) {
                        newName = tempName;
                        uniqueNameFound = true;
                        break;
                    }
                }
                if (!uniqueNameFound) {
                    continue;
                }
            }
            macroName = newName;
        }

        insertMacroDataList[macroName] = macroData;
    }

    inserted_count = insertMacroDataList.size();
    if (insertMacroDataList.isEmpty()) {
        return inserted_count;
    }

    bool insertToEnd = false;
    int insertRow = -1;

    // If there is a highlighted selection, insert at the selected row index (Excel-like).
    // If there is no selection, keep the existing behavior and append to the end.
    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[insertMacroDataFromCopiedList] There is no selected item, insert to the end.";
#endif
        insertToEnd = true;
    }
    else {
        QTableWidgetSelectionRange range = selectedRanges.first();
        insertRow = range.topRow();
    }

    if (insertToEnd) {
        // Append to the end of the current list (existing behavior)
        for (auto it = insertMacroDataList.begin(); it != insertMacroDataList.end(); ++it) {
            (*macroDataList)[it.key()] = it.value();
        }
    }
    else {
        // Insert at the selected row index (Excel-like) while preserving order.
        // This uses OrderedMap::insertAt to avoid rebuilding the container.
        macroDataList->insertAt(insertRow, insertMacroDataList);
    }

    // Refresh table display
    refreshMacroListTabWidget(macroDataTable, *macroDataList);

    // Reselect inserted rows
    if (inserted_count > 0) {
        int startRow = insertToEnd ? (macroDataTable->rowCount() - inserted_count) : insertRow;
        int endRow = startRow + inserted_count - 1;
        QTableWidgetSelectionRange newSelection = QTableWidgetSelectionRange(startRow, 0, endRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
        macroDataTable->clearSelection();
        macroDataTable->setRangeSelected(newSelection, true);

        // Update current cell to the start of inserted rows for Ctrl/Shift+Click consistency
        macroDataTable->setCurrentCell(startRow, 0, QItemSelectionModel::NoUpdate);

        // Scroll to make the inserted items visible
        QTableWidgetItem *itemToScrollTo = macroDataTable->item(startRow, 0);
        if (itemToScrollTo) {
            macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
        }
    }

#ifdef DEBUG_LOGOUT_ON
    if (insertToEnd) {
        qDebug().nospace() << "[insertMacroDataFromCopiedList] Ctrl+V pressed, appended (" << inserted_count << ") macros to the end of current macro list";
    }
    else {
        qDebug().nospace() << "[insertMacroDataFromCopiedList] Ctrl+V pressed, inserted (" << inserted_count << ") macros at row(" << insertRow << ") of current macro list";
    }
#endif

    return inserted_count;
}

void QMacroListDialog::selectedMacroItemsMoveUp()
{
    if (isMacroDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the macro table is filtered!");
        QKeyMapper::getInstance()->showWarningPopup(message);
        return;
    }

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveUpMacroItem] There is no selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

    if (topRow <= 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveUpMacroItem] Cannot move up, already at the top";
#endif
        return;
    }

    // Get all keys in order
    QList<QString> keysList = macroDataList->keys();

    // Move the selected rows up by one
    for (int row = topRow; row <= bottomRow; ++row) {
        keysList.move(row, row - 1);
    }

    // Rebuild the OrderedMap with new order
    OrderedMap<QString, MappingMacroData> newMacroList;
    for (const QString &key : std::as_const(keysList)) {
        newMacroList[key] = macroDataList->value(key);
    }
    *macroDataList = newMacroList;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[selectedMacroItemsMoveUp] MoveUp: topRow(" << topRow << "), bottomRow(" << bottomRow << ")";
#endif

#ifdef DEBUG_LOGOUT_ON
    qDebug() << __func__ << ": refreshMacroListTabWidget()";
#endif
    refreshMacroListTabWidget(macroDataTable, *macroDataList);

    // Reselect the moved rows
    QTableWidgetSelectionRange newSelection(topRow - 1, 0, bottomRow - 1, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->clearSelection();
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to the top of the new selection for Ctrl/Shift+Click consistency
    macroDataTable->setCurrentCell(newSelection.topRow(), 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the selected items visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newSelection.topRow(), 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    if (macroDataTable->rowCount() != macroDataList->size()) {
        qDebug("MoveUp:MacroData sync error!!! DataTableSize(%d), DataListSize(%d)", macroDataTable->rowCount(), macroDataList->size());
    }
#endif
}

void QMacroListDialog::selectedMacroItemsMoveToTop()
{
    if (isMacroDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the macro table is filtered!");
        QKeyMapper::getInstance()->showWarningPopup(message);
        return;
    }

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveToTop] There is no selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

    if (topRow <= 0) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveToTop] Already at the top";
#endif
        return;
    }

    // Get all keys in order
    QList<QString> keysList = macroDataList->keys();

    // Move the selected rows to the top (preserve order)
    QList<QString> movedKeys;
    // Save selected keys in order
    for (int row = topRow; row <= bottomRow; ++row) {
        movedKeys.append(keysList.at(row));
    }
    // Remove selected keys from bottom to top
    for (int row = bottomRow; row >= topRow; --row) {
        keysList.removeAt(row);
    }
    // Insert keys at the top in original order
    for (int i = movedKeys.size() - 1; i >= 0; --i) {
        keysList.insert(0, movedKeys.at(i));
    }

    // Rebuild the OrderedMap with new order
    OrderedMap<QString, MappingMacroData> newMacroList;
    for (const QString &key : std::as_const(keysList)) {
        newMacroList[key] = macroDataList->value(key);
    }
    *macroDataList = newMacroList;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[selectedMacroItemsMoveToTop] MoveTop: topRow(" << topRow << "), bottomRow(" << bottomRow << ")";
#endif

#ifdef DEBUG_LOGOUT_ON
    qDebug() << __func__ << ": refreshMacroListTabWidget()";
#endif
    refreshMacroListTabWidget(macroDataTable, *macroDataList);

    // Reselect the moved rows at the top
    QTableWidgetSelectionRange newSelection(0, 0, bottomRow - topRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->clearSelection();
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to the top of the new selection for Ctrl/Shift+Click consistency
    macroDataTable->setCurrentCell(newSelection.topRow(), 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the selected items visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newSelection.topRow(), 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    if (macroDataTable->rowCount() != macroDataList->size()) {
        qDebug("MoveToTop:MacroData sync error!!! DataTableSize(%d), DataListSize(%d)", macroDataTable->rowCount(), macroDataList->size());
    }
#endif
}

void QMacroListDialog::selectedMacroItemsMoveDown()
{
    if (isMacroDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the macro table is filtered!");
        QKeyMapper::getInstance()->showWarningPopup(message);
        return;
    }

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveDownMacroItem] There is no selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

    if (bottomRow >= macroDataTable->rowCount() - 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveDownMacroItem] Cannot move down, already at the bottom";
#endif
        return;
    }

    // Get all keys in order
    QList<QString> keysList = macroDataList->keys();

    // Move the selected rows down by one
    for (int row = bottomRow; row >= topRow; --row) {
        keysList.move(row, row + 1);
    }

    // Rebuild the OrderedMap with new order
    OrderedMap<QString, MappingMacroData> newMacroList;
    for (const QString &key : std::as_const(keysList)) {
        newMacroList[key] = macroDataList->value(key);
    }
    *macroDataList = newMacroList;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[selectedMacroItemsMoveDown] MoveDown: topRow(" << topRow << "), bottomRow(" << bottomRow << ")";
#endif

#ifdef DEBUG_LOGOUT_ON
    qDebug() << __func__ << ": refreshMacroListTabWidget()";
#endif
    refreshMacroListTabWidget(macroDataTable, *macroDataList);

    // Reselect the moved rows
    QTableWidgetSelectionRange newSelection(topRow + 1, 0, bottomRow + 1, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->clearSelection();
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to the bottom of the new selection for Ctrl/Shift+Click consistency
    macroDataTable->setCurrentCell(newSelection.bottomRow(), 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the selected items visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newSelection.bottomRow(), 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    if (macroDataTable->rowCount() != macroDataList->size()) {
        qDebug("MoveDown:MacroData sync error!!! DataTableSize(%d), DataListSize(%d)", macroDataTable->rowCount(), macroDataList->size());
    }
#endif
}

void QMacroListDialog::selectedMacroItemsMoveToBottom()
{
    if (isMacroDataTableFiltered()) {
        QString message;
        message = tr("Cannot move items while the macro table is filtered!");
        QKeyMapper::getInstance()->showWarningPopup(message);
        return;
    }

    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveToBottom] There is no selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

    if (bottomRow >= macroDataTable->rowCount() - 1) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[MoveToBottom] Already at the bottom";
#endif
        return;
    }

    // Get all keys in order
    QList<QString> keysList = macroDataList->keys();

    // Move the selected rows to the bottom (preserve order)
    QList<QString> movedKeys;
    // Save selected keys in order
    for (int row = topRow; row <= bottomRow; ++row) {
        movedKeys.append(keysList.at(row));
    }
    // Remove selected keys from bottom to top
    for (int row = bottomRow; row >= topRow; --row) {
        keysList.removeAt(row);
    }
    // Append keys to the bottom in original order
    for (int i = 0; i < movedKeys.size(); ++i) {
        keysList.append(movedKeys.at(i));
    }

    // Rebuild the OrderedMap with new order
    OrderedMap<QString, MappingMacroData> newMacroList;
    for (const QString &key : std::as_const(keysList)) {
        newMacroList[key] = macroDataList->value(key);
    }
    *macroDataList = newMacroList;

#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[selectedMacroItemsMoveToBottom] MoveBottom: topRow(" << topRow << "), bottomRow(" << bottomRow << ")";
#endif

#ifdef DEBUG_LOGOUT_ON
    qDebug() << __func__ << ": refreshMacroListTabWidget()";
#endif
    refreshMacroListTabWidget(macroDataTable, *macroDataList);

    // Reselect the moved rows at the bottom
    QTableWidgetSelectionRange newSelection(macroDataTable->rowCount() - (bottomRow - topRow + 1), 0, macroDataTable->rowCount() - 1, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
    macroDataTable->clearSelection();
    macroDataTable->setRangeSelected(newSelection, true);

    // Update current cell to the bottom of the new selection for Ctrl/Shift+Click consistency
    macroDataTable->setCurrentCell(newSelection.bottomRow(), 0, QItemSelectionModel::NoUpdate);

    // Scroll to make the selected items visible
    QTableWidgetItem *itemToScrollTo = macroDataTable->item(newSelection.bottomRow(), 0);
    if (itemToScrollTo) {
        macroDataTable->scrollToItem(itemToScrollTo, QAbstractItemView::EnsureVisible);
    }

#ifdef DEBUG_LOGOUT_ON
    if (macroDataTable->rowCount() != macroDataList->size()) {
        qDebug("MoveToBottom:MacroData sync error!!! DataTableSize(%d), DataListSize(%d)", macroDataTable->rowCount(), macroDataList->size());
    }
#endif
}

void QMacroListDialog::deleteMacroSelectedItems()
{
    MacroListDataTableWidget *macroDataTable = getCurrentMacroDataTable();
    OrderedMap<QString, MappingMacroData> *macroDataList = getCurrentMacroDataList();

    if (!macroDataTable || !macroDataList) {
        return;
    }

    QList<QTableWidgetSelectionRange> selectedRanges = macroDataTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[DeleteMacroItem] There is no selected item";
#endif
        return;
    }

    // Get the first selected range
    QTableWidgetSelectionRange range = selectedRanges.first();
    int topRow = range.topRow();
    int bottomRow = range.bottomRow();

#ifdef DEBUG_LOGOUT_ON
    qDebug("DeleteMacro: topRow(%d), bottomRow(%d)", topRow, bottomRow);
#endif

    // Collect macro names to delete
    QList<QString> macroNamesToDelete;
    for (int row = topRow; row <= bottomRow; ++row) {
        QTableWidgetItem *nameItem = macroDataTable->item(row, MACRO_NAME_COLUMN);
        if (nameItem) {
            macroNamesToDelete.append(nameItem->text());
        }
    }

    // Remove from data list
    for (const QString &macroName : std::as_const(macroNamesToDelete)) {
        macroDataList->remove(macroName);
    }

    // Refresh the display
    refreshMacroListTabWidget(macroDataTable, *macroDataList);

    // Reselect the row at the top of the deleted range, or the last row if the table is empty
    if (macroDataTable->rowCount() > 0) {
        int newRow = qMin(topRow, macroDataTable->rowCount() - 1);
        QTableWidgetSelectionRange newSelection = QTableWidgetSelectionRange(newRow, 0, newRow, MACROLISTDATA_TABLE_COLUMN_COUNT - 1);
        macroDataTable->clearSelection();
        macroDataTable->setRangeSelected(newSelection, true);

        // Update current cell to match the new selection for Ctrl/Shift+Click consistency
        macroDataTable->setCurrentCell(newRow, 0, QItemSelectionModel::NoUpdate);
    }

    QString popupMessage;
    QString popupMessageColor = SUCCESS_COLOR;
    int popupMessageDisplayTime = POPUP_MESSAGE_DISPLAY_TIME_DEFAULT;

    if (macroNamesToDelete.size() == 1) {
        popupMessage = tr("Macro \"%1\" deleted successfully").arg(macroNamesToDelete.first());
    } else {
        popupMessage = tr("%1 macros deleted successfully").arg(macroNamesToDelete.size());
    }
    emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
}
