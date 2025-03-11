#include <QApplication>
#include <QDir>
#include "qkeymapper.h"
#include "qkeymapper_worker.h"
#include "interception_worker.h"
#ifdef SINGLE_APPLICATION
#include "singleapp/singleapplication.h"
#endif
#ifdef LOGOUT_TOFILE
#include <QDateTime>
#include <QTextStream>
#endif

#ifdef QT_DEBUG
#ifdef VLD_DETECT
#include "vld.h"
#endif
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
    text_stream << message << "\r\n";
    file.flush();
    file.close();

#ifdef QT_NO_DEBUG
#else
    fprintf(stderr, "%s\n", message.toLocal8Bit().constData());
#endif

    logfile_mutex->unlock();
}
#endif

void updateQtDisplayEnvironment(void)
{
    int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    //    HWND hwd = GetDesktopWindow();
    HDC hdc = GetDC(NULL);
    int width = GetDeviceCaps(hdc, DESKTOPHORZRES);
    int height = GetDeviceCaps(hdc, DESKTOPVERTRES);
    Q_UNUSED(height);
    double dWidth = static_cast<double>(width);
    double dScreenWidth = static_cast<double>(nScreenWidth);
    double scale = dWidth / dScreenWidth;
    ReleaseDC(NULL, hdc);

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "Original QT_SCALE_FACTOR ->" << qgetenv("QT_SCALE_FACTOR");
#endif

    bool Flag_4K = false;

    if (scale < 1.10) {
        if (dWidth >= 3840) {
            qputenv("QT_SCALE_FACTOR", "1.5");
            qputenv("WINDOWS_SCALE_FACTOR", "4K_1.0");
            Flag_4K = true;
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Set QT_SCALE_FACTOR to [1.5] for screen ->" << width << "*" << height;
#endif
        }
        else if (dWidth >= 2560) {
            qputenv("QT_SCALE_FACTOR", "1.25");
            qputenv("WINDOWS_SCALE_FACTOR", "2K_1.0");
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Set QT_SCALE_FACTOR to [1.25] for screen ->" << width << "*" << height;
#endif
        }
        else {
            qputenv("QT_SCALE_FACTOR", "1");
            qputenv("WINDOWS_SCALE_FACTOR", "1K_1.0");
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "set QT_SCALE_FACTOR to [1] for screen ->" << width << "*" << height;
#endif
        }
    }
    else if (1.24 <= scale &&  scale <= 1.26) {
        if (dWidth >= 3840) {
            Flag_4K = true;
            qunsetenv("QT_SCALE_FACTOR");
            qputenv("WINDOWS_SCALE_FACTOR", "4K_1.25");
        }
        else if (dWidth >= 2560) {
            qunsetenv("QT_SCALE_FACTOR");
            qputenv("WINDOWS_SCALE_FACTOR", "2K_1.25");
        }
        else {
            qunsetenv("QT_SCALE_FACTOR");
            qputenv("WINDOWS_SCALE_FACTOR", "1K_1.25");
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Updated QT_SCALE_FACTOR ->" << qgetenv("QT_SCALE_FACTOR") << ", for screen ->" << width << "*" << height;
#endif
    }
    else if (1.49 <= scale &&  scale <= 1.51) {
        if (dWidth >= 3840) {
            Flag_4K = true;
            qunsetenv("QT_SCALE_FACTOR");
            qputenv("WINDOWS_SCALE_FACTOR", "4K_1.5");
        }
        else if (dWidth >= 2560) {
            qunsetenv("QT_SCALE_FACTOR");
            qputenv("WINDOWS_SCALE_FACTOR", "2K_1.5");
        }
        else {
            qunsetenv("QT_SCALE_FACTOR");
            qputenv("WINDOWS_SCALE_FACTOR", "1K_1.5");
        }
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Updated QT_SCALE_FACTOR ->" << qgetenv("QT_SCALE_FACTOR") << ", for screen ->" << width << "*" << height;
#endif
    }
    else {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Original screen scale ->" << scale << "," << "Screen [" << width << "*" << height <<"], do not need to set QT_SCALE_FACTOR.";
#endif
    }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (Flag_4K == true) {
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Ceil);
    }
    else {
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    }
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)) && (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}

int main(int argc, char *argv[])
{
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows10) {
        qputenv("QT_OPENGL", "software");
        qputenv("QT_ANGLE_PLATFORM", "d3d9");
    }

    qSetMessagePattern("%{time [hh:mm:ss.zzz]} %{message}");

    updateQtDisplayEnvironment();

    // Check if a scaling factor argument is passed to the program.
    if (argc > 1) {  // Ensure at least one additional argument is passed
        QString argument = QString(argv[1]);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "Program argument count=" << argc << ", argument1=" << argument;
#endif
        if (argument.startsWith("--scale=")) {
            QString scaleValue = argument.mid(QString("--scale=").length());
            qputenv("QT_SCALE_FACTOR", scaleValue.toUtf8());
#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Passed scale parameter ->" << scaleValue;
#endif
        }
    }

    QApplication::setApplicationName(QString(argv[0]));
    QApplication::setOrganizationName("AsukaVoV");
#ifdef DEBUG_LOGOUT_ON
    qDebug() << "ApplicationName ->" << QApplication::applicationName();
#endif
#ifdef SINGLE_APPLICATION
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
    QApplication::setFont(QFont(FONTNAME_ENGLISH, 9));

    QThread::currentThread()->setObjectName("QKeyMapper");

    Interception_Worker * const interception_worker = Interception_Worker::getInstance();;
    // Move Interception_Worker Process to a sub thread
    QThread * const interceptionThread = new QThread();
    interceptionThread->setObjectName("Interception_Worker");
    interception_worker->moveToThread(interceptionThread);
    QObject::connect(interceptionThread, &QThread::started, interception_worker, &Interception_Worker::InterceptionThreadStarted);
    interceptionThread->start();

    QKeyMapper_Hook_Proc * const keymapper_hook_proc = QKeyMapper_Hook_Proc::getInstance();
    // Move Hook Process to a sub thread
    QThread * const hookprocThread = new QThread();
    keymapper_hook_proc->moveToThread(hookprocThread);
    hookprocThread->setObjectName("QKeyMapper_Hook_Proc");
    QObject::connect(hookprocThread, &QThread::started, keymapper_hook_proc, &QKeyMapper_Hook_Proc::HookProcThreadStarted);
    QObject::connect(hookprocThread, &QThread::finished, keymapper_hook_proc, &QKeyMapper_Hook_Proc::HookProcThreadFinished);
    hookprocThread->start();

    QKeyMapper_Worker * const keymapper_worker = QKeyMapper_Worker::getInstance();
    // Move Worker to a sub thread
    QThread * const workerThread = new QThread();
    keymapper_worker->moveToThread(workerThread);
    workerThread->setObjectName("QKeyMapper_Worker");
    QObject::connect(workerThread, &QThread::started, keymapper_worker, &QKeyMapper_Worker::threadStarted);
    workerThread->start();

    QKeyMapper w;
    emit QKeyMapper::getInstance()->checkOSVersionMatched_Signal();

#ifdef SINGLE_APPLICATION
    QObject::connect(&app, &SingleApplication::showUp, &w, &QKeyMapper::otherInstancesStarted);
#endif

    // Remove "?" Button from QDialog
    Qt::WindowFlags flags = Qt::Dialog;
    flags |= Qt::WindowMinimizeButtonHint;
    flags |= Qt::WindowCloseButtonHint;
    w.setWindowFlags(flags);

    if (QKeyMapper::getStartupMinimizedStatus()) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "Auto Start Mapping = TRUE, hide QKeyMapper window at startup.";
#endif
        /* Add show&hide to trigger QKeyMapper::showEvent */
        /* m_MainWindowHandle will update by winId() at the startup with minimize to systemtray */
        w.show();
        w.hide();
    }
    else {
        w.show();
    }

    int ret = app.exec();

#ifdef LOGOUT_TOFILE
    delete logfile_mutex;
#endif

    Interception_Worker::interceptionLoopBreak();
    interceptionThread->quit();
    interceptionThread->wait();
    delete interceptionThread;

    hookprocThread->quit();
    hookprocThread->wait();
    delete hookprocThread;

    workerThread->quit();
    workerThread->wait();
    delete workerThread;

    return ret;
}
