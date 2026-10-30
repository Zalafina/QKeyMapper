#include "qkm_ibinput_popup.h"
#include "qkm_ibinput_client.h"
#include "qkm_ui_scale.h"
#include "qkeymapper_qt_compat.h"
#include <QComboBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHideEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <tlhelp32.h>

namespace {
QString version(const QString &path) {
    const auto native=reinterpret_cast<LPCWSTR>(path.utf16());
    DWORD ignored=0; const DWORD size=GetFileVersionInfoSizeW(native,&ignored);
    if (!size) return {};
    QByteArray bytes(static_cast<int>(size),'\0');
    if (!GetFileVersionInfoW(native,0,size,bytes.data())) return {};
    struct Translation { WORD language; WORD codePage; };
    Translation *translations=nullptr; UINT length=0;
    if (VerQueryValueW(bytes.data(),L"\\VarFileInfo\\Translation",reinterpret_cast<LPVOID *>(&translations),&length)) {
        for (UINT index=0;index<length/sizeof(Translation);++index) {
            const QString key=QStringLiteral("\\StringFileInfo\\%1%2\\ProductVersion")
                .arg(translations[index].language,4,16,QLatin1Char('0'))
                .arg(translations[index].codePage,4,16,QLatin1Char('0'));
            LPWSTR value=nullptr; UINT characters=0;
            if (VerQueryValueW(bytes.data(),reinterpret_cast<LPCWSTR>(key.utf16()),reinterpret_cast<LPVOID *>(&value),&characters) && characters>1) {
                const QString display=QString::fromWCharArray(value).trimmed();
                if (!display.isEmpty()) return display;
            }
        }
    }
    VS_FIXEDFILEINFO *info=nullptr;
    if (!VerQueryValueW(bytes.data(),L"\\",reinterpret_cast<LPVOID *>(&info),&length) || length<sizeof(*info)) return {};
    return QStringLiteral("%1.%2.%3.%4").arg(HIWORD(info->dwProductVersionMS)).arg(LOWORD(info->dwProductVersionMS)).arg(HIWORD(info->dwProductVersionLS)).arg(LOWORD(info->dwProductVersionLS));
}
QStringList processes() {
    QStringList observed;
    const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (snapshot==INVALID_HANDLE_VALUE) return observed;
    PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry);
    if (Process32FirstW(snapshot,&entry)) do {
        const QString name=QString::fromWCharArray(entry.szExeFile);
        if (name.compare(QStringLiteral("lghub_agent.exe"),Qt::CaseInsensitive)==0
            || name.compare(QStringLiteral("lghub_updater.exe"),Qt::CaseInsensitive)==0
            || name.compare(QStringLiteral("Razer Synapse.exe"),Qt::CaseInsensitive)==0
            || name.compare(QStringLiteral("Razer Synapse 3.exe"),Qt::CaseInsensitive)==0) observed.append(name);
    } while (Process32NextW(snapshot,&entry));
    CloseHandle(snapshot); observed.removeDuplicates(); return observed;
}
}
QkmIbInputPopup::QkmIbInputPopup(QWidget *parent):QFrame(parent,Qt::Popup) {
    setObjectName(QStringLiteral("ibInputPopup")); setFrameStyle(QFrame::Box|QFrame::Raised);
    setFont(parent->font());
    auto *layout=new QVBoxLayout(this); layout->setContentsMargins(8,8,8,8); layout->setSpacing(6);
    auto *form=new QFormLayout; form->setSpacing(6);
    m_backendLabel=new QLabel(this); m_policyLabel=new QLabel(this);
    m_backend=new QComboBox(this); m_policy=new QComboBox(this);
    m_backend->setObjectName(QStringLiteral("ibInputBackendComboBox")); m_policy->setObjectName(QStringLiteral("ibInputPolicyComboBox"));
    m_backend->setFocusPolicy(Qt::ClickFocus); m_policy->setFocusPolicy(Qt::ClickFocus);
    m_backend->addItems({QString(),QString(),QString(),QString()});
    m_policy->addItems({QString(),QString()});
    form->addRow(m_backendLabel,m_backend); form->addRow(m_policyLabel,m_policy); layout->addLayout(form);
    m_feedbackLabel=new QLabel(this); m_feedbackLabel->setObjectName(QStringLiteral("ibInputFeedbackLabel"));
    m_feedbackLabel->setWordWrap(true); m_feedbackLabel->setTextFormat(Qt::PlainText);
    m_feedbackLabel->setMinimumHeight(fontMetrics().lineSpacing()*2); layout->addWidget(m_feedbackLabel);
    m_details=new QPlainTextEdit(this); m_details->setReadOnly(true); m_details->setMinimumSize(360,120); layout->addWidget(m_details);
    auto *actions=new QHBoxLayout;
    m_refresh=new QPushButton(this); m_check=new QPushButton(this);
    m_refresh->setObjectName(QStringLiteral("ibInputRefreshButton")); m_check->setObjectName(QStringLiteral("ibInputCheckButton"));
    for (auto *button:{m_refresh,m_check}) { button->setAutoDefault(false); button->setFocusPolicy(Qt::NoFocus); actions->addWidget(button); }
    layout->addLayout(actions);
    m_help=new QLabel(this); m_help->setTextFormat(Qt::RichText); m_help->setOpenExternalLinks(true); m_help->setWordWrap(true); layout->addWidget(m_help);
    connect(m_refresh,&QPushButton::clicked,this,[this] { m_feedback=Feedback::Refreshed; m_feedbackAt=QDateTime::currentDateTime(); refresh(); });
    connect(m_check,&QPushButton::clicked,this,[this] { if (m_running || m_checkPending || QkmIbInputClient::instance().busy()) return; m_checkPending=true; m_feedback=Feedback::Checking; refresh(); emit connectionCheckRequested(); });
    const auto changed=[this] {
        QkmIbInputClient::Preferences preferences;
        const QStringList values{QStringLiteral("auto"),QStringLiteral("logitech_ghub"),QStringLiteral("logitech_legacy"),QStringLiteral("razer")};
        preferences.backend=values.value(m_backend->currentIndex(),QStringLiteral("auto"));
        preferences.unavailablePolicy=m_policy->currentIndex()==1 ? QStringLiteral("sendinput_before_start") : QStringLiteral("block");
        if (QkmIbInputClient::instance().setPreferences(preferences)) { m_feedback=Feedback::None; emit preferencesChanged(); }
        refresh();
    };
    connect(m_backend,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[changed](int) { changed(); });
    connect(m_policy,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[changed](int) { changed(); });
    m_timer.setInterval(5000); connect(&m_timer,&QTimer::timeout,this,&QkmIbInputPopup::refresh);
    retranslate(); m_scale=new QkmUiScale(this); m_scale->capture();
}
void QkmIbInputPopup::retranslate() {
    const QSignalBlocker backendBlock(m_backend),policyBlock(m_policy);
    m_backendLabel->setText(tr("Preferred backend")); m_policyLabel->setText(tr("If unavailable"));
    const QStringList backends{tr("Automatic (recommended)"),QStringLiteral("Logitech G HUB"),tr("Logitech legacy"),QStringLiteral("Razer Synapse")};
    for (int i=0;i<backends.size();++i) m_backend->setItemText(i,backends.at(i));
    m_policy->setItemText(0,tr("Do not start mapping")); m_policy->setItemText(1,tr("Use SendInput before start"));
    m_refresh->setText(tr("Refresh status")); m_check->setText(tr("Check connection"));
    m_refresh->setToolTip(tr("Refresh installation, process and driver information without connecting or sending input."));
    m_check->setToolTip(tr("Temporarily connect to the preferred backend, then disconnect. No test input is sent."));
    const QColor link=palette().color(QPalette::Window).lightness()<128 ? QColor(114,178,255) : palette().color(QPalette::Link);
    m_help->setText(QStringLiteral("<style>a { color: %1; }</style>").arg(link.name())+tr("<a href=\"https://www.logitechg.com/innovation/g-hub.html\">G HUB</a> | <a href=\"https://www.razer.com/synapse\">Razer Synapse</a><br>Connection and accepted calls do not prove that an application received input."));
    refresh();
}
void QkmIbInputPopup::refresh() {
    const auto preferences=QkmIbInputClient::instance().preferences();
    const auto status=QkmIbInputClient::instance().statusSnapshot();
    const QSignalBlocker backendBlock(m_backend),policyBlock(m_policy);
    m_backend->setCurrentIndex(QStringList{QStringLiteral("auto"),QStringLiteral("logitech_ghub"),QStringLiteral("logitech_legacy"),QStringLiteral("razer")}.indexOf(preferences.backend));
    m_policy->setCurrentIndex(preferences.unavailablePolicy==QStringLiteral("sendinput_before_start") ? 1 : 0);
    const bool busy=m_running || m_checkPending || QkmIbInputClient::instance().busy();
    m_backend->setEnabled(!busy); m_policy->setEnabled(!busy); m_check->setEnabled(!busy);
    QString feedback;
    const QString time=m_feedbackAt.toString(QStringLiteral("HH:mm:ss"));
    switch (m_feedback) {
    case Feedback::None: break;
    case Feedback::Refreshed: feedback=tr("Status refreshed at %1.").arg(time); break;
    case Feedback::Checking: feedback=tr("Checking connection..."); break;
    case Feedback::Passed: feedback=tr("Connection check passed at %1. The temporary connection is closed.").arg(time); break;
    case Feedback::Failed: feedback=tr("Connection check failed at %1. See the result below.").arg(time); break;
    }
    m_feedbackLabel->setText(feedback);
    const bool dark=palette().color(QPalette::Window).lightness()<128;
    m_feedbackLabel->setStyleSheet(m_feedback==Feedback::Passed
        ? (dark ? QStringLiteral("color:#70dc70;") : QStringLiteral("color:#006400;"))
        : m_feedback==Feedback::Failed
            ? (dark ? QStringLiteral("color:#ff8585;") : QStringLiteral("color:#c00000;")) : QString());
    const bool fallback=status.fallback && m_running;
    const QString backend=status.connected ? (status.backend==QStringLiteral("logitech_ghub") ? QStringLiteral("Logitech G HUB") : status.backend)
        : (fallback ? QStringLiteral("SendInput") : tr("Not connected"));
    const QString lastResult=m_feedback==Feedback::Passed ? feedback : status.detail;
    const QString base=qEnvironmentVariable("ProgramW6432",qEnvironmentVariable("ProgramFiles"));
    const QString installed=version(base+QStringLiteral("/LGHUB/lghub_agent.exe"));
    QSettings bus(QStringLiteral("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\logi_joy_bus_enum"),QSettings::NativeFormat);
    QStringList lines{tr("G HUB version: %1").arg(installed.isEmpty() ? tr("Unknown") : installed),
        tr("Observed process names: %1").arg(processes().join(QStringLiteral(", "))),
        tr("G HUB driver registration: %1").arg(bus.contains(QStringLiteral("ImagePath")) ? tr("Observed") : tr("Unknown")),
        tr("Razer installation and driver: Unknown"),
        tr("Backend: %1; fallback: %2").arg(backend,fallback ? tr("Yes") : tr("No")),
        tr("Last result: %1").arg(lastResult),
        tr("Sampled: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate))};
    if (busy) lines.append(tr("Stop mapping and wait for cleanup before changing preferences."));
    lines.append(tr("Status refresh does not initialize a driver."));
    lines.append(tr("Currently supports G HUB relative mouse movement and basic key output (A-Z, 0-9, F1-F24)."));
    m_details->setPlainText(lines.join(QLatin1Char('\n')));
}
void QkmIbInputPopup::setRunning(bool running) { if (running && !m_running) m_feedback=Feedback::None; m_running=running; if (isVisible()) refresh(); }
void QkmIbInputPopup::connectionChecked() { m_checkPending=false; if (m_feedback==Feedback::Checking) m_feedback=Feedback::None; if (isVisible()) refresh(); }
void QkmIbInputPopup::connectionChecked(bool success) { m_feedback=success ? Feedback::Passed : Feedback::Failed; m_feedbackAt=QDateTime::currentDateTime(); connectionChecked(); }
void QkmIbInputPopup::showAt(QWidget *anchor,qreal scale) {
    retranslate(); m_scale->apply(scale); adjustSize();
    QPoint point=anchor->mapToGlobal(QPoint(0,anchor->height()));
    if (QScreen *screen=QKeyMapperQtCompat::widgetScreen(anchor)) {
        const QRect area=screen->availableGeometry();
        point.setX(qBound(area.left(),point.x(),qMax(area.left(),area.right()-width()+1)));
        if (point.y()+height()>area.bottom()+1) point.setY(anchor->mapToGlobal(QPoint(0,0)).y()-height());
        point.setY(qBound(area.top(),point.y(),qMax(area.top(),area.bottom()-height()+1)));
    }
    move(point); show();
}
void QkmIbInputPopup::showEvent(QShowEvent *event) { QFrame::showEvent(event); refresh(); m_timer.start(); }
void QkmIbInputPopup::hideEvent(QHideEvent *event) { m_timer.stop(); QFrame::hideEvent(event); }
