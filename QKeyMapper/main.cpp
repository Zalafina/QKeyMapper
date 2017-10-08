#include <QApplication>
#include <QStyleFactory>
#include "qkeymapper.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QKeyMapper w;

    // Remove "?" Button from QDialog
    Qt::WindowFlags flags = Qt::Dialog;
    flags |= Qt::WindowMinMaxButtonsHint;
    flags |= Qt::WindowCloseButtonHint;
    w.setWindowFlags(flags);

    w.show();

    return a.exec();
}
