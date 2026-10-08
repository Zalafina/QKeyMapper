#include "../qkm_ui_scale.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <qt_windows.h>
#include <cstdio>
#include <cstdlib>

namespace {
void require(bool ok, const char *message)
{
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
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
    auto *table = new QTableWidget(2, 2, &root);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->setColumnWidth(0, 80);
    table->verticalHeader()->setMinimumSectionSize(10);
    table->verticalHeader()->setDefaultSectionSize(25);
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
