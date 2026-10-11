#ifndef QKM_MESSAGE_BOX_H
#define QKM_MESSAGE_BOX_H

#include <QMessageBox>

// Internal presentation adapter; button and modal contracts remain Qt-owned.
class QkmMessageBox final
{
public:
    static QMessageBox::StandardButton information(QWidget *parent, const QString &title,
        const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton question(QWidget *parent, const QString &title,
        const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton warning(QWidget *parent, const QString &title,
        const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton critical(QWidget *parent, const QString &title,
        const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static int exec(QMessageBox &box);

private:
    static QMessageBox::StandardButton show(QWidget *parent, QMessageBox::Icon icon,
        const QString &title, const QString &text, QMessageBox::StandardButtons buttons,
        QMessageBox::StandardButton defaultButton);
};

#endif
