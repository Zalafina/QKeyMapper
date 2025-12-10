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

using namespace QKeyMapperConstants;

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

#if 0
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
#endif

void setupQtScaleEnvironment(const QString &program_dir)
{
    QString config_file_path = program_dir + "/" + CONFIG_FILENAME;
    QSettings settingFile(config_file_path, QSettings::IniFormat);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    settingFile.setIniCodec("UTF-8");
#endif
    int display_scale = settingFile.value(DISPLAY_SCALE, DISPLAY_SCALE_DEFAULT).toInt();

    constexpr double SCALE_100 = 1.0;
    constexpr double SCALE_125 = 1.25;
    constexpr double SCALE_150 = 1.5;
    constexpr double SCALE_175 = 1.75;
    constexpr double SCALE_200 = 2.0;

    int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    HDC hdc = GetDC(NULL);
    int width = GetDeviceCaps(hdc, DESKTOPHORZRES);
    int height = GetDeviceCaps(hdc, DESKTOPVERTRES);
    Q_UNUSED(height);
    double dWidth = static_cast<double>(width);
    double dScreenWidth = static_cast<double>(nScreenWidth);
    double system_scale_value = dWidth / dScreenWidth;
    ReleaseDC(NULL, hdc);

    bool high_dpi = false;
    double scale_value = 0;
    switch (display_scale) {
    case DISPLAY_SCALE_PERCENT_100:
        scale_value = SCALE_100;
        qputenv("QT_SCALE_FACTOR", QByteArray::number(scale_value));
        break;
    case DISPLAY_SCALE_PERCENT_125:
        scale_value = SCALE_125;
        qputenv("QT_SCALE_FACTOR", QByteArray::number(scale_value));
        high_dpi = true;
        system_scale_value = SCALE_125;
        break;
    case DISPLAY_SCALE_PERCENT_150:
        scale_value = SCALE_150;
        qputenv("QT_SCALE_FACTOR", QByteArray::number(scale_value));
        high_dpi = true;
        system_scale_value = SCALE_150;
        break;
    case DISPLAY_SCALE_PERCENT_175:
        scale_value = SCALE_175;
        qputenv("QT_SCALE_FACTOR", QByteArray::number(scale_value));
        high_dpi = true;
        system_scale_value = SCALE_175;
        break;
    case DISPLAY_SCALE_PERCENT_200:
        scale_value = SCALE_200;
        qputenv("QT_SCALE_FACTOR", QByteArray::number(scale_value));
        high_dpi = true;
        system_scale_value = SCALE_200;
        break;
    default:
        if (width >= 3840) {
            high_dpi = true;
        }
        break;
    }

#ifdef DEBUG_LOGOUT_ON
    if (scale_value > 0) {
        qDebug() << "[setupQtScaleEnvironment] Set QT_SCALE_FACTOR from DisplayScale setting value ->" << scale_value;
    }
#endif

    QKeyMapper::setDisplayScaleValue(system_scale_value);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (high_dpi) {
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Ceil);
    }
    else {
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    }
#else
    Q_UNUSED(high_dpi);
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

    // Check if a scaling factor argument is passed to the program.
    bool scale_from_param = false;
    if (argc > 1) {  // Ensure at least one additional argument is passed
        QString argument = QString(argv[1]);
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace() << "Program argument count=" << argc << ", argument1=" << argument;
#endif
        if (argument.startsWith("--scale=")) {
            QString scaleValue = argument.mid(QString("--scale=").length());
            qputenv("QT_SCALE_FACTOR", scaleValue.toUtf8());
            scale_from_param = true;

            if (scaleValue == "1.0") {
                int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
                HDC hdc = GetDC(NULL);
                int width = GetDeviceCaps(hdc, DESKTOPHORZRES);
                double dWidth = static_cast<double>(width);
                double dScreenWidth = static_cast<double>(nScreenWidth);
                double system_scale_value = dWidth / dScreenWidth;
                ReleaseDC(NULL, hdc);

                QKeyMapper::setDisplayScaleValue(system_scale_value);
            }

#ifdef DEBUG_LOGOUT_ON
            qDebug() << "Passed scale parameter ->" << scaleValue;
#endif
        }
    }

    // updateQtDisplayEnvironment();
    if (!scale_from_param) {
        QString programPath = QString::fromLocal8Bit(argv[0]);
        QFileInfo fileInfo(programPath);
        QString programDir = fileInfo.absolutePath();
        setupQtScaleEnvironment(programDir);
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
    QObject::connect(workerThread, &QThread::started, keymapper_worker, &QKeyMapper_Worker::threadStarted, Qt::DirectConnection);
    QObject::connect(workerThread, &QThread::finished, keymapper_worker, &QKeyMapper_Worker::threadFinished, Qt::DirectConnection);
    workerThread->start();

    QKeyMapper w;
    emit QKeyMapper::getInstance()->updateGamepadSelectComboBox_Signal(QKeyMapperConstants::JOYSTICK_INVALID_INSTANCE_ID);
    emit QKeyMapper::getInstance()->checkOSVersionMatched_Signal();
#ifndef ENABLE_SYSTEMFILTERKEYS_DEFAULT
    emit QKeyMapper::getInstance()->checkFilterKeysEnabled_Signal();
#endif

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
    }
    else {
        w.show();
    }

    int ret = app.exec();

#ifdef LOGOUT_TOFILE
    delete logfile_mutex;
#endif

#ifdef FAKERINPUT_SUPPORT
    // Explicitly release FakerInput before thread cleanup to avoid DLL unload issues
    QKeyMapper_Worker::FakerInputClient_Disconnect();
    QKeyMapper_Worker::FakerInputClient_Free();
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
