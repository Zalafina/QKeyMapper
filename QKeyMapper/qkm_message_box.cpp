#include "qkm_message_box.h"
#include "qkeymapper.h"
#include "qkm_ui_scale.h"

#include <QDialogButtonBox>
#include <QPushButton>

namespace {
qreal ownerRatio(QWidget *owner)
{
    for (QWidget *root = owner; root; root = root->parentWidget()) {
        for (QObject *child : root->children()) {
            if (auto *scale = dynamic_cast<QkmUiScale *>(child)) { return scale->ratio(); }
        }
    }
    return 1;
}
}

int QkmMessageBox::exec(QMessageBox &box)
{
    // Leave the default path untouched, including Qt's native-dialog selection.
    if (!qFuzzyCompare(ownerRatio(box.parentWidget()), qreal(1))) {
        if (auto *main = QKeyMapper::getInstance()) {
            box.ensurePolished();
            main->registerEditorUiScale(&box);
        }
    }
    return box.exec();
}

QMessageBox::StandardButton QkmMessageBox::show(QWidget *parent, QMessageBox::Icon icon,
    const QString &title, const QString &text, QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    // Retain Qt's legacy default-button overload behavior as well as R=1.
    if (qFuzzyCompare(ownerRatio(parent), qreal(1)) || (defaultButton && !(buttons & defaultButton))) {
        switch (icon) {
        case QMessageBox::Information: return QMessageBox::information(parent, title, text, buttons, defaultButton);
        case QMessageBox::Question: return QMessageBox::question(parent, title, text, buttons, defaultButton);
        case QMessageBox::Critical: return QMessageBox::critical(parent, title, text, buttons, defaultButton);
        default: return QMessageBox::warning(parent, title, text, buttons, defaultButton);
        }
    }
    QMessageBox box(icon, title, text, QMessageBox::NoButton, parent);
    box.setFont(QFont(QKeyMapperConstants::FONTNAME_ENGLISH, 9));
    // Match Qt's static helpers: ascending buttons, first AcceptRole default.
    for (uint mask = QMessageBox::FirstButton; mask <= QMessageBox::LastButton; mask <<= 1) {
        if (!(buttons & mask)) { continue; }
        const auto standard = static_cast<QMessageBox::StandardButton>(mask);
        auto *button = box.addButton(standard);
        if (!box.defaultButton()
            && ((defaultButton == QMessageBox::NoButton && box.buttonRole(button) == QMessageBox::AcceptRole)
                || defaultButton == standard)) {
            box.setDefaultButton(button);
        }
    }
    if (exec(box) == -1) { return QMessageBox::Cancel; }
    return box.standardButton(box.clickedButton());
}

QMessageBox::StandardButton QkmMessageBox::information(QWidget *parent, const QString &title,
    const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    return show(parent, QMessageBox::Information, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton QkmMessageBox::question(QWidget *parent, const QString &title,
    const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    return show(parent, QMessageBox::Question, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton QkmMessageBox::warning(QWidget *parent, const QString &title,
    const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    return show(parent, QMessageBox::Warning, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton QkmMessageBox::critical(QWidget *parent, const QString &title,
    const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    return show(parent, QMessageBox::Critical, title, text, buttons, defaultButton);
}
