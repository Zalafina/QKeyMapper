#include <QApplication>
#include "qkeymapper.h"

#ifdef DEBUG_LOGOUT_ON
//#include "vld.h"
#endif

int main(int argc, char *argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    qSetMessagePattern("%{time [hh:mm:ss.zzz]} %{message}");

#ifdef ADJUST_PRIVILEGES
//    BOOL adjustresult = QKeyMapper::AdjustPrivileges();
//    qDebug() << "AdjustPrivileges Result:" << adjustresult;

    BOOL adjustresult = QKeyMapper::EnableDebugPrivilege();
    qDebug() << "EnableDebugPrivilege Result:" << adjustresult;
#endif

    QKeyMapper w;

    // Remove "?" Button from QDialog
    Qt::WindowFlags flags = Qt::Dialog;
    flags |= Qt::WindowMinimizeButtonHint;
    flags |= Qt::WindowCloseButtonHint;
    w.setWindowFlags(flags);

    if (true == w.getAutoStartMappingStatus()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Auto Start Mapping = TRUE, hide QKeyMapper window at startup.";
#endif
    }
    else {
        w.show();
    }

    return a.exec();
}
