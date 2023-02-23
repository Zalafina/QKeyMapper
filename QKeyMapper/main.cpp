#include <QApplication>
#include <QDir>
#include "qkeymapper.h"
#include "qkeymapper_worker.h"
#ifdef SINGLE_APPLICATION
#include "singleapp/singleapplication.h"
#endif
#ifdef LOGOUT_TOFILE
#include <QDateTime>
#endif

#ifdef DEBUG_LOGOUT_ON
//#include "vld.h"
#endif

#ifdef LOGOUT_TOFILE
static QMutex *logfile_mutex = Q_NULLPTR;
void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    logfile_mutex->lock();

    QString level;
    switch(type)
    {
    case QtDebugMsg:
        level = QString("[D]");
        break;

    case QtInfoMsg:
        level = QString("[I]");
        break;

    case QtWarningMsg:
        level = QString("[W]");
        break;

    case QtCriticalMsg:
        level = QString("[E]");
        break;

    case QtFatalMsg:
        level = QString("[F]");
    }

//    QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString current_date = QString("[%1]").arg(current_date_time);
    QString message = QString("%1%2 %3").arg(current_date).arg(level).arg(msg);

    QFile file("log.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << message << endl;
    file.flush();
    file.close();

    fprintf(stderr, "%s\n", message.toLocal8Bit().constData());

    logfile_mutex->unlock();
}
#endif

int main(int argc, char *argv[])
{
    qSetMessagePattern("%{time [hh:mm:ss.zzz]} %{message}");

    int nScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    HWND hwd = ::GetDesktopWindow();
    HDC hdc = ::GetDC(hwd);
    int width = GetDeviceCaps(hdc, DESKTOPHORZRES);
    double dWidth = static_cast<double>(width);
    double dScreenWidth = static_cast<double>(nScreenWidth);
    double scale = dWidth / dScreenWidth;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)) && (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    if (scale < 1.10) {
        if (dWidth >= 3840) {
            qputenv("QT_SCALE_FACTOR", "1.5");
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Set QT_SCALE_FACTOR to [1.5] for screen width ->" << dWidth;
#endif
        }
        else if (dWidth >= 2560) {
            qputenv("QT_SCALE_FACTOR", "1.25");
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Set QT_SCALE_FACTOR to [1.25] for screen width ->" << dWidth;
#endif
        }
        else {
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Do not set QT_SCALE_FACTOR for screen width ->" << dWidth;
#endif
        }
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Original screen scale ->" << scale << "," << "Screen width ->" << dWidth << ", do not need to set QT_SCALE_FACTOR.";
#endif
    }

#ifdef SINGLE_APPLICATION
    QApplication::setApplicationName("QKeyMapper");
    QApplication::setOrganizationName("AsukaVoV");
    SingleApplication app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

    if (QDir::currentPath() != QCoreApplication::applicationDirPath()) {
        QDir::setCurrent(QCoreApplication::applicationDirPath());
    }

#ifdef LOGOUT_TOFILE
    logfile_mutex = new QMutex();
    qInstallMessageHandler(outputMessage);
#endif

    QApplication::setStyle(QStyleFactory::create("Fusion"));

#ifdef ADJUST_PRIVILEGES
//    BOOL adjustresult = QKeyMapper::AdjustPrivileges();
//    qDebug() << "AdjustPrivileges Result:" << adjustresult;

    BOOL adjustresult = QKeyMapper::EnableDebugPrivilege();
    qDebug() << "EnableDebugPrivilege Result:" << adjustresult;
#endif
    QThread::currentThread()->setObjectName("QKeyMapper");

    QKeyMapper_Worker * const keymapper_worker = QKeyMapper_Worker::getInstance();
    // Move Checksumer to a sub thread
    QThread * const workerThread = new QThread();
    keymapper_worker->moveToThread(workerThread);
    workerThread->setObjectName("QKeyMapper_Worker");
    QObject::connect(workerThread, SIGNAL(started()), keymapper_worker, SLOT(threadStarted()));
    workerThread->start();

    QKeyMapper w;

#ifdef SINGLE_APPLICATION
    QObject::connect(&app, &SingleApplication::showUp, &w, &QKeyMapper::raiseQKeyMapperWindow);
#endif

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

    int ret = app.exec();

#ifdef LOGOUT_TOFILE
    delete logfile_mutex;
#endif

    return ret;
}
