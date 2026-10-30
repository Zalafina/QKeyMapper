#include "qkm_demo_host.h"
#include "qkm_demo_mode.h"
#include "../qkeymapper_constants.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTranslator>
#include <QVBoxLayout>

namespace QkmAutomation {

DemoHostDialog::DemoHostDialog(QWidget *parent) : QDialog(parent)
{
    setObjectName(QStringLiteral("demoHostDialog"));
    setMinimumWidth(520);
    auto *layout = new QVBoxLayout(this);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("demoHostStatus"));
    m_status->setTextFormat(Qt::PlainText);
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_status->setFocusPolicy(Qt::ClickFocus);
    layout->addWidget(m_status);
    m_setup = new QPushButton(this);
    m_setup->setObjectName(QStringLiteral("demoHostPairing"));
    m_setup->setAutoDefault(false);
    m_setup->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_setup, 0, Qt::AlignRight);
    m_close = new QPushButton(this);
    m_close->setObjectName(QStringLiteral("demoHostClose"));
    m_close->setAutoDefault(false);
    m_close->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_close, 0, Qt::AlignRight);
    connect(m_close, &QPushButton::clicked, this, &QWidget::close);
    connect(m_setup, &QPushButton::clicked, this, [this] {
        m_pairingResult = {};
        if (!m_pairing.start(m_process, tr("QKeyMapper Demo Host"),
                tr("Finish MCP pairing in the QKeyMapper management window, then click OK to return to your desktop."),
                tr("The original desktop could not be restored.\nClick OK to try returning again.\nReason: %1")))
            m_pairingResult = m_pairing.result();
        refreshText();
    });
    connect(&m_pairing, &DemoPairingSession::finished, this, [this] {
        m_pairingResult = m_pairing.result();
        refreshText();
        if (m_closeAfterPairing || !m_process.running()) close();
    });
    m_started = m_process.start();
    refreshText();
    connect(&m_poll, &QTimer::timeout, this, [this] {
        if (m_started && !m_process.running() && !m_pairing.isRunning()) close();
    });
    m_poll.start(100);
}

void DemoHostDialog::refreshText()
{
    setWindowTitle(tr("QKeyMapper Demo Host"));
    m_close->setText(tr("Close Demo"));
    m_setup->setText(tr("Set Up Pairing"));
    m_setup->setEnabled(m_started && m_process.running() && !m_pairing.isRunning());
    if (m_closeFailed)
        m_status->setText(tr("The demo instance has not exited.\nReason: %1").arg(m_process.failure()));
    else if (!m_pairingResult.ok()) {
        const QString detail = m_pairingResult.failure + (m_pairingResult.nativeError
            ? QStringLiteral(" (%1)").arg(*m_pairingResult.nativeError) : QString());
        m_status->setText(tr("Pairing setup could not be shown.\nReason: %1").arg(detail));
    }
    else if (!m_started)
        m_status->setText(tr("The demo instance could not be started.\nReason: %1").arg(m_process.failure()));
    else
        m_status->setText(tr("Private demo instance: %1\nExecutable: %2")
            .arg(m_process.target().processId).arg(m_process.executable()));
}

void DemoHostDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) refreshText();
    QDialog::changeEvent(event);
}

void DemoHostDialog::closeEvent(QCloseEvent *event)
{
    if (m_pairing.isRunning()) {
        m_closeAfterPairing = true;
        m_pairing.cancel();
        event->ignore();
        return;
    }
    if (m_process.close()) event->accept();
    else {
        m_closeFailed = true;
        refreshText();
        event->ignore();
    }
}

int runDemoHost(int &argc, char **argv)
{
    if (!configurationOnlyDemo()) return ERROR_INVALID_PARAMETER;
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setFont(QFont(QString::fromLatin1(QKeyMapperConstants::FONTNAME_ENGLISH), 9));
    QSettings settings(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("keymapdata.ini")), QSettings::IniFormat);
    const QStringList locales{QStringLiteral("zh_CN"), QStringLiteral("en_US"), QStringLiteral("ja_JP")};
    const int language = settings.value(QStringLiteral("LanguageIndex"), 0).toInt();
    QTranslator translator;
    if (translator.load(QStringLiteral(":/QKeyMapper_%1.qm").arg(locales.value(language, locales.first()))))
        app.installTranslator(&translator);
    DemoHostDialog dialog;
    dialog.show();
    app.exec();
    const auto code = dialog.process().exitCode();
    return code ? int(code.value()) : ERROR_PROCESS_ABORTED;
}

} // namespace QkmAutomation
