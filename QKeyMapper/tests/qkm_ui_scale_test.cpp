#include "../qkm_ui_scale.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QScrollBar>
#include <QStyleFactory>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <qt_windows.h>
#include <cstdio>
#include <cstdlib>

namespace {
void require(bool ok, const char *message)
{
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}

void verifyClearButtonScaling()
{
    QDialog root;
    root.setFont(QFont(QStringLiteral("Arial"), 9));
    auto *fusion = QStyleFactory::create(QStringLiteral("Fusion"));
    fusion->setParent(&root);
    root.setStyle(fusion);
    auto *layout = new QVBoxLayout(&root);
    auto *edit = new QLineEdit(&root);
    edit->setStyle(fusion);
    edit->setClearButtonEnabled(true);
    edit->setText(QStringLiteral("vjoy"));
    layout->addWidget(edit);
    root.resize(400, 100);
    root.show();
    QApplication::processEvents();
    auto *button = edit->findChild<QToolButton *>(QString(), Qt::FindDirectChildrenOnly);
    require(button, "Native clear button missing");
    const QRect authoredButton = button->geometry();
    QList<QStyle::PixelMetric> metrics = {QStyle::PM_SmallIconSize, QStyle::PM_TabBarIconSize,
        QStyle::PM_ListViewIconSize, QStyle::PM_IconViewIconSize, QStyle::PM_ScrollView_ScrollBarSpacing};
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    metrics.prepend(QStyle::PM_LineEditIconMargin);
    metrics.prepend(QStyle::PM_LineEditIconSize);
#endif
    QList<int> baseline;
    for (auto metric : metrics) { baseline.append(edit->style()->pixelMetric(metric, nullptr, edit)); }
    QkmUiScale scale(&root);
    scale.manageWindow();
    for (qreal ratio : {1.0, 2.0, 0.5, 1.0}) {
        scale.applyWindow(ratio);
        QApplication::processEvents();
        for (int i = 0; i < metrics.size(); ++i) {
            const int actual = edit->style()->pixelMetric(metrics[i], nullptr, edit);
            std::printf("Style metric=%d R=%.2f baseline=%d actual=%d\n", int(metrics[i]), double(ratio), baseline[i], actual);
            std::fflush(stdout);
            require(actual == qRound(baseline[i] * ratio), "Style metric was scaled more than once");
        }
        require(edit->rect().contains(button->geometry()), "Clear button clipped by line edit");
        if (qFuzzyCompare(ratio, qreal(1))) {
            require(button->geometry() == authoredButton, "Default clear button geometry changed");
        }
        const auto *focus = QApplication::focusWidget();
        button->click();
        require(edit->text().isEmpty(), "Native clear button did not clear text");
        require(QApplication::focusWidget() == focus, "Clear button stole focus");
        edit->setText(QStringLiteral("vjoy"));
        QApplication::processEvents();
    }
    std::puts("Native clear button and nested style metrics passed");
}
}

namespace {
void verifyDynamicFixedPresentation()
{
    QDialog root;
    auto *layout = new QVBoxLayout(&root);
    auto *label = new QLabel(QStringLiteral("Target"), &root);
    layout->addWidget(label);
    root.setFixedSize(220, 82);
    root.setStyleSheet(QStringLiteral("QDialog { padding: 4px; color: red; }"));
    label->setStyleSheet(QStringLiteral("padding: 3px; color: red;"));
    QkmUiScale scale(&root);
    scale.manageWindow();
    scale.applyWindow(2);
    scale.setAuthoredFixedSize(QSize(360, 82));
    const QString rootSheet = QStringLiteral("QDialog { padding: 4px; color: blue; }");
    const QString labelSheet = QStringLiteral("padding: 3px; color: blue;");
    scale.setAuthoredStyleSheet(&root, rootSheet);
    scale.setAuthoredStyleSheet(label, labelSheet);
    require(root.size() == QSize(720, 164) && root.minimumSize() == root.maximumSize(), "Dynamic fixed root not scaled");
    require(root.styleSheet().contains(QStringLiteral("padding: 8px"))
            && label->styleSheet().contains(QStringLiteral("padding: 6px")), "New theme template not scaled");
    for (qreal ratio : {0.5, 2.0, 1.0}) {
        scale.applyWindow(ratio);
        QApplication::processEvents();
        require(root.size() == QSize(qRound(360 * ratio), qRound(82 * ratio)), "Dynamic fixed root baseline lost");
        require(root.styleSheet().contains(QStringLiteral("color: blue"))
                && label->styleSheet().contains(QStringLiteral("color: blue")), "New theme reverted");
    }
    require(root.styleSheet() == rootSheet && label->styleSheet() == labelSheet, "Authored theme not restored");
    std::puts("Dynamic fixed root and authored theme passed");
}

void verifyAuthoredWindowSize()
{
    class HintDialog final : public QDialog {
    public:
        QSize minimumSizeHint() const override { return QSize(240, 120); }
    };
    HintDialog root;
    root.resize(180, 120);
    const QSize authored = root.size();
    QkmUiScale scale(&root);
    scale.manageWindow();
    scale.applyWindow(2);
    scale.applyWindow(1);
    require(root.size() == authored, "Default session size was expanded by an implicit hint");
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    verifyDynamicFixedPresentation();
    verifyAuthoredWindowSize();
    verifyClearButtonScaling();
    QDialog root;
    root.setFont(QFont(QStringLiteral("Arial"), 9));
    auto *layout = new QVBoxLayout(&root);
    layout->setContentsMargins(12, 8, 12, 8);
    auto *edit = new QLineEdit(&root);
    edit->setText(QStringLiteral("unsaved draft"));
    edit->setSelection(2, 4);
    layout->addWidget(edit);
    auto *preview = new QLabel(&root);
    QPixmap picture(48, 24);
    picture.fill(Qt::red);
    preview->setPixmap(picture);
    layout->addWidget(preview);
    auto *reservedIconCombo = new QComboBox(&root);
    auto *iconCombo = new QComboBox(&root);
    for (auto *combo : {reservedIconCombo, iconCombo}) {
        combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        combo->setMinimumContentsLength(5);
        combo->setIconSize(QSize(16, 16));
        combo->addItems({QStringLiteral("First"), QStringLiteral("Second")});
        combo->setCurrentIndex(1);
        layout->addWidget(combo);
    }
    iconCombo->setItemIcon(1, QIcon(picture));
    auto *table = new QTableWidget(0, 2, &root);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->setColumnWidth(0, 80);
    table->verticalHeader()->setMinimumSectionSize(10);
    table->verticalHeader()->setDefaultSectionSize(25);
    table->verticalHeader()->setStyleSheet(QStringLiteral("QHeaderView::section { color: #1A9EDB; padding-left: 2px; padding-right: 1px; }"));
    auto *tableStyle = QStyleFactory::create(QStringLiteral("Fusion"));
    tableStyle->setParent(table);
    table->setStyle(tableStyle);
    table->setRowCount(2);
    layout->addWidget(table);
    root.resize(600, 400);
    QDialog nested(&root);
    nested.setFont(QFont(QStringLiteral("Arial"), 11));
    const QFont nestedFont = nested.font();
    QkmUiScale scale(&root);
    scale.manageWindow();
    root.show();
    app.processEvents();
    const QSize baseline = root.size();
    const int reservedComboHeight = reservedIconCombo->sizeHint().height();
    const int iconComboHeight = iconCombo->sizeHint().height();
    for (int round = 0; round < 10; ++round) {
        for (qreal ratio : {0.5, 0.6, 0.7, 0.8, 0.9, 1.25, 1.5, 1.75, 2.0, 1.0}) {
            scale.applyWindow(ratio);
            app.processEvents();
            require(edit->text() == QStringLiteral("unsaved draft") && edit->selectedText() == QStringLiteral("save"), "Draft/selection changed");
            require(qAbs(edit->font().pointSizeF() - 9 * ratio) < 0.01, "Font accumulated scaling");
            require(layout->contentsMargins().left() == qRound(12 * ratio), "Root layout was not scaled");
            const QSize previewSize = QKeyMapperQtCompat::labelPixmap(preview).size();
            require(qAbs(previewSize.width() - qRound(48 * ratio)) <= 1
                    && qAbs(previewSize.height() - qRound(24 * ratio)) <= 1, "Preview accumulated scaling");
            require(table->columnWidth(0) == qRound(80 * ratio), "Fixed column was not scaled");
            require(table->rowHeight(0) == qRound(25 * ratio), "Actual table row height was not scaled");
            require(nested.font() == nestedFont, "Nested root scaled twice");
            require(qAbs(reservedIconCombo->sizeHint().height() - qRound(reservedComboHeight * ratio)) <= 1,
                    "Reserved-icon combo height was not scaled once");
            require(qAbs(iconCombo->sizeHint().height() - qRound(iconComboHeight * ratio)) <= 1,
                    "Icon combo height was not scaled once");
            for (auto *combo : {reservedIconCombo, iconCombo}) {
                require(combo->iconSize() == QSize(qRound(16 * ratio), qRound(16 * ratio)), "Combo icon size changed");
                require(combo->currentIndex() == 1 && combo->currentText() == QStringLiteral("Second"), "Combo selection changed");
            }
        }
        require(root.size() == baseline, "Round trip lost session size");
    }
    std::puts("Ten scale cycles passed");
    std::fflush(stdout);
    scale.applyWindow(2);
    table->setRowCount(0);
    table->setRowCount(20);
    table->setItem(4, 1, new QTableWidgetItem(QStringLiteral("pending comment")));
    table->setRangeSelected(QTableWidgetSelectionRange(3, 0, 5, 1), true);
    table->setCurrentCell(4, 1, QItemSelectionModel::NoUpdate);
    scale.applyWindow(2);
    app.processEvents();
    require(table->rowHeight(19) == 50, "Rebuilt table lost scaled row height");
    table->verticalScrollBar()->setValue(5);
    const int scroll = table->verticalScrollBar()->value();
    scale.applyWindow(2);
    app.processEvents();
    require(table->item(4, 1)->text() == QStringLiteral("pending comment")
            && table->currentRow() == 4 && table->currentColumn() == 1
            && table->selectedItems().size() == 1 && table->selectedRanges().size() == 1
            && table->selectedRanges().constFirst().topRow() == 3
            && table->selectedRanges().constFirst().bottomRow() == 5, "Table draft/selection changed on refresh");
    require(table->verticalScrollBar()->value() == scroll, "Repeated refresh changed table scroll");
    scale.applyWindow(1);
    app.processEvents();
    require(table->rowHeight(19) == 25, "Rebuilt table lost authored row height");
    table->setRowCount(2);
    scale.applyWindow(2);
    QPixmap replacement(32, 16);
    replacement.fill(Qt::blue);
    preview->setPixmap(replacement);
    app.processEvents();
    require(QKeyMapperQtCompat::labelPixmap(preview).size() == QSize(64, 32), "New preview was not scaled before painting");
    scale.applyWindow(1);
    require(QKeyMapperQtCompat::labelPixmap(preview).size() == replacement.size(), "New preview lost its authored baseline");
    preview->clear();
    app.processEvents();
    scale.applyWindow(2);
    require(QKeyMapperQtCompat::labelPixmap(preview).isNull(), "Cleared preview was restored");
    scale.applyWindow(1.5);
    MSG message = {};
    message.hwnd = reinterpret_cast<HWND>(root.winId());
    message.message = WM_ENTERSIZEMOVE;
    static_cast<QAbstractNativeEventFilter *>(&scale)->nativeEventFilter("windows_generic_MSG", &message, nullptr);
    root.resize(1050, 750);
    message.message = WM_EXITSIZEMOVE;
    static_cast<QAbstractNativeEventFilter *>(&scale)->nativeEventFilter("windows_generic_MSG", &message, nullptr);
    app.processEvents();
    scale.applyWindow(1);
    require(root.size() == QSize(700, 500), "User resize did not update session baseline");
    root.hide();
    scale.applyWindow(0.5);
    root.show();
    app.processEvents();
    require(root.size() == QSize(350, 250), "Hidden reopen lost latest ratio");
    std::puts("Session resize and hidden reopen passed");
    std::fflush(stdout);
    auto *pending = new QDialog;
    auto *pendingScale = new QkmUiScale(pending);
    pendingScale->manageWindow();
    pendingScale->applyWindow(1.5);
    pending->show();
    delete pending;
    app.processEvents();
    std::puts("UI scale checks passed (10 cycles, draft, nested root, table, session resize, hidden reopen, pending destruction)");
    return 0;
}
