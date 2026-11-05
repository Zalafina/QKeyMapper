#include "qkm_ibinput_popup.h"
#include "qkm_ibinput_client.h"
#include "qkm_ui_scale.h"
#include "qkeymapper_qt_compat.h"
#include <QComboBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHideEvent>
#include <QLabel>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QtMath>
#include <QScrollBar>
#include <QEvent>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <tlhelp32.h>
#include <setupapi.h>

namespace {

struct RazerObservation {
    QList<QPair<QString,bool>> services;
    bool servicesKnown=false,interfaceKnown=false,interfaceObserved=false;
};
RazerObservation observeRazer() {
    RazerObservation result;
    const SC_HANDLE manager=OpenSCManagerW(nullptr,nullptr,SC_MANAGER_ENUMERATE_SERVICE);
    if (manager) {
        DWORD needed=0,count=0,resume=0;
        const BOOL queried=EnumServicesStatusExW(manager,SC_ENUM_PROCESS_INFO,SERVICE_DRIVER,SERVICE_STATE_ALL,
            nullptr,0,&needed,&count,&resume,nullptr);
        const DWORD error=queried ? ERROR_SUCCESS : GetLastError();
        if (queried) result.servicesKnown=true;
        else if (error==ERROR_MORE_DATA && needed>0 && needed<=1024*1024) {
            QByteArray bytes(static_cast<int>(needed),'\0'); resume=0;
            if (EnumServicesStatusExW(manager,SC_ENUM_PROCESS_INFO,SERVICE_DRIVER,SERVICE_STATE_ALL,
                reinterpret_cast<LPBYTE>(bytes.data()),needed,&needed,&count,&resume,nullptr)) {
                result.servicesKnown=true;
                const auto entries=reinterpret_cast<const ENUM_SERVICE_STATUS_PROCESSW *>(bytes.constData());
                for (DWORD index=0;index<count;++index) {
                    const QString name=QString::fromWCharArray(entries[index].lpServiceName);
                    const QString display=QString::fromWCharArray(entries[index].lpDisplayName);
                    if (name.startsWith(QStringLiteral("Rz"),Qt::CaseInsensitive)
                        || display.contains(QStringLiteral("Razer"),Qt::CaseInsensitive))
                        result.services.append(qMakePair(name,entries[index].ServiceStatusProcess.dwCurrentState==SERVICE_RUNNING));
                }
            }
        }
        CloseServiceHandle(manager);
    }
    const GUID channel={0xe3be005d,0xd130,0x4910,{0x88,0xff,0x09,0xae,0x02,0xf6,0x80,0xe9}};
    const HDEVINFO devices=SetupDiGetClassDevsW(&channel,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if (devices!=INVALID_HANDLE_VALUE) {
        SP_DEVICE_INTERFACE_DATA item{}; item.cbSize=sizeof(item);
        for (DWORD index=0;;++index) {
            if (!SetupDiEnumDeviceInterfaces(devices,nullptr,&channel,index,&item)) {
                result.interfaceKnown=GetLastError()==ERROR_NO_MORE_ITEMS; break;
            }
            if (item.Flags & SPINT_ACTIVE) { result.interfaceKnown=true; result.interfaceObserved=true; break; }
        }
        SetupDiDestroyDeviceInfoList(devices);
    }
    return result;
}

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
enum class DriverRegistration { Registered, Missing, QueryFailed };
DriverRegistration ghubRegistration() {
    HKEY key=nullptr;
    LONG error=RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Services\\logi_joy_bus_enum",0,KEY_QUERY_VALUE,&key);
    if (error==ERROR_SUCCESS) {
        DWORD bytes=0; error=RegQueryValueExW(key,L"ImagePath",nullptr,nullptr,nullptr,&bytes);
        RegCloseKey(key);
    }
    if (error==ERROR_SUCCESS) return DriverRegistration::Registered;
    return error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND ? DriverRegistration::Missing : DriverRegistration::QueryFailed;
}
QStringList processes(bool &known) {
    known=false;
    QStringList observed;
    const HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (snapshot==INVALID_HANDLE_VALUE) return observed;
    PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry);
    if (Process32FirstW(snapshot,&entry)) do {
        const QString name=QString::fromWCharArray(entry.szExeFile);
        if (name.compare(QStringLiteral("lghub_agent.exe"),Qt::CaseInsensitive)==0
            || name.compare(QStringLiteral("lghub_updater.exe"),Qt::CaseInsensitive)==0) observed.append(name);
    } while (Process32NextW(snapshot,&entry));
    known=GetLastError()==ERROR_NO_MORE_FILES;
    CloseHandle(snapshot); observed.removeDuplicates(); observed.sort(Qt::CaseInsensitive); return observed;
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
    form->addRow(m_backendLabel,m_backend);
    m_autoHint=new QLabel(this); m_autoHint->setObjectName(QStringLiteral("ibInputAutoHintLabel"));
    m_autoHint->setWordWrap(true); m_autoHint->setTextFormat(Qt::PlainText); form->addRow(m_autoHint);
    form->addRow(m_policyLabel,m_policy); layout->addLayout(form);
    m_feedbackView=new QTextEdit(this); m_feedbackView->setObjectName(QStringLiteral("ibInputFeedbackLabel"));
    m_feedbackView->setReadOnly(true); m_feedbackView->setFocusPolicy(Qt::ClickFocus);
    m_feedbackView->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_feedbackView->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
    m_feedbackView->setFixedHeight(fontMetrics().lineSpacing()*2+8);
    layout->addWidget(m_feedbackView);
    m_details=new QTextEdit(this); m_details->setObjectName(QStringLiteral("ibInputDetails"));
    m_details->setReadOnly(true); m_details->setFocusPolicy(Qt::ClickFocus);
    m_details->setMinimumSize(440,fontMetrics().lineSpacing()*10+16); layout->addWidget(m_details);
    auto *actions=new QHBoxLayout;
    m_refresh=new QPushButton(this); m_check=new QPushButton(this);
    m_refresh->setObjectName(QStringLiteral("ibInputRefreshButton")); m_check->setObjectName(QStringLiteral("ibInputCheckButton"));
    for (auto *button:{m_refresh,m_check}) { button->setAutoDefault(false); button->setFocusPolicy(Qt::NoFocus); actions->addWidget(button); }
    layout->addLayout(actions);
    m_help=new QLabel(this); m_help->setObjectName(QStringLiteral("ibInputHelpLabel")); m_help->setTextFormat(Qt::RichText); m_help->setOpenExternalLinks(true); m_help->setWordWrap(true); layout->addWidget(m_help);
    connect(m_refresh,&QPushButton::clicked,this,[this] { m_feedback=Feedback::Refreshed; m_feedbackAt=QDateTime::currentDateTime(); refresh(); });
    connect(m_check,&QPushButton::clicked,this,[this] { if (m_running || m_checkPending || QkmIbInputClient::instance().busy()) return; m_checkedBackend.clear(); m_checkedPreference=QkmIbInputClient::instance().preferences().backend; m_checkPending=true; m_feedback=Feedback::Checking; refresh(); emit connectionCheckRequested(); });
    const auto changed=[this] {
        QkmIbInputClient::Preferences preferences;
        const QStringList values{QStringLiteral("auto"),QStringLiteral("logitech_ghub"),QStringLiteral("logitech_legacy"),QStringLiteral("razer")};
        preferences.backend=values.value(m_backend->currentIndex(),QStringLiteral("auto"));
        preferences.unavailablePolicy=m_policy->currentIndex()==1 ? QStringLiteral("sendinput_before_start") : QStringLiteral("block");
        if (QkmIbInputClient::instance().setPreferences(preferences)) { m_feedback=Feedback::None; m_checkedBackend.clear(); m_checkedPreference.clear(); emit preferencesChanged(); }
        refresh();
    };
    connect(m_backend,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[changed](int) { changed(); });
    connect(m_policy,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[changed](int) { changed(); });
    m_timer.setInterval(5000); connect(&m_timer,&QTimer::timeout,this,&QkmIbInputPopup::refresh);
    retranslate(); m_scale=new QkmUiScale(this); m_scale->capture();
}
QString QkmIbInputPopup::backendName(const QString &backend) const {
    if (backend==QStringLiteral("logitech_ghub")) return tr("Logitech G HUB");
    if (backend==QStringLiteral("logitech_legacy")) return tr("Logitech legacy");
    if (backend==QStringLiteral("razer")) return tr("Razer");
    return backend;
}
void QkmIbInputPopup::retranslate() {
    const QSignalBlocker backendBlock(m_backend),policyBlock(m_policy);
    m_backendLabel->setText(tr("Preferred backend")); m_policyLabel->setText(tr("If unavailable"));
    const QStringList backends{tr("Automatic (recommended)"),backendName(QStringLiteral("logitech_ghub")),backendName(QStringLiteral("logitech_legacy")),backendName(QStringLiteral("razer"))};
    for (int i=0;i<backends.size();++i) m_backend->setItemText(i,backends.at(i));
    m_policy->setItemText(0,tr("Do not start mapping")); m_policy->setItemText(1,tr("Use SendInput before start"));
    m_refresh->setText(tr("Refresh status")); m_check->setText(tr("Check connection"));
    m_refresh->setToolTip(tr("Refresh installation, process and driver information without connecting or sending input."));
    m_check->setToolTip(tr("Temporarily connect to the preferred backend, then disconnect. No test input is sent."));
    const QString automaticTip=tr("At mapping start, QKeyMapper tries backends in order and uses the first that supports the mapping and connects successfully. Check connection checks connectivity only; mapping start selects again. Refresh status does not connect.");
    m_backend->setToolTip(automaticTip); m_autoHint->setToolTip(automaticTip);
    refresh();
}
void QkmIbInputPopup::refresh() {
    const auto preferences=QkmIbInputClient::instance().preferences();
    const auto status=QkmIbInputClient::instance().statusSnapshot();
    if (!m_checkedPreference.isEmpty() && m_checkedPreference!=preferences.backend) {
        m_checkedBackend.clear();
        if (m_feedback==Feedback::Passed) m_feedback=Feedback::None;
    }
    QStringList order;
    for (const auto &candidate:QkmIbInputClient::candidates(QStringLiteral("auto"))) order.append(backendName(candidate));
    m_autoHint->setText(tr("Automatic order: %1").arg(order.join(QStringLiteral(" → "))));
    m_autoHint->setVisible(preferences.backend==QStringLiteral("auto"));
    const QSignalBlocker backendBlock(m_backend),policyBlock(m_policy);
    m_backend->setCurrentIndex(QStringList{QStringLiteral("auto"),QStringLiteral("logitech_ghub"),QStringLiteral("logitech_legacy"),QStringLiteral("razer")}.indexOf(preferences.backend));
    m_policy->setCurrentIndex(preferences.unavailablePolicy==QStringLiteral("sendinput_before_start") ? 1 : 0);
    const bool busy=m_running || m_checkPending || QkmIbInputClient::instance().busy();
    m_backend->setEnabled(!busy); m_policy->setEnabled(!busy); m_check->setEnabled(!busy);
    QString feedback;
    const bool lastCheck=m_feedback==Feedback::None && !m_running && !status.connected && !m_checkedBackend.isEmpty();
    const QString time=(lastCheck ? m_checkedAt : m_feedbackAt).toString(QStringLiteral("HH:mm:ss"));
    const auto passedText=[&] {
        if (m_checkedBackend.isEmpty()) return tr("Connection check passed at %1. The temporary connection is closed.").arg(time);
        return (m_checkedPreference==QStringLiteral("auto")
            ? tr("Connection check passed at %1: automatically selected %2. The temporary connection is closed.")
            : tr("Connection check passed at %1: %2. The temporary connection is closed."))
            .arg(time,backendName(m_checkedBackend));
    };
    switch (m_feedback) {
    case Feedback::None: if (lastCheck) feedback=passedText(); break;
    case Feedback::Refreshed: feedback=tr("Status refreshed at %1.").arg(time); break;
    case Feedback::Checking: feedback=tr("Checking connection..."); break;
    case Feedback::Passed: feedback=passedText(); break;
    case Feedback::Failed: feedback=tr("Connection check failed at %1.").arg(time); break;
    }
    QStringList feedbackLines;
    if (!feedback.isEmpty()) feedbackLines.append(feedback);
    if (!status.detail.isEmpty() && ((m_feedback!=Feedback::Passed && m_feedback!=Feedback::Checking) || status.releaseIncomplete || status.stopping))
        feedbackLines.append(lastCheck ? tr("Last result: %1").arg(status.detail) : status.detail);
    if (busy && m_feedback!=Feedback::Checking)
        feedbackLines.append(tr("Stop mapping and wait for cleanup before changing preferences."));
    const QString feedbackText=feedbackLines.join(QLatin1Char('\n'));
    if (m_feedbackView->toPlainText()!=feedbackText) m_feedbackView->setPlainText(feedbackText);
    const int lineHeight=m_feedbackView->fontMetrics().lineSpacing();
    const int frame=2*m_feedbackView->frameWidth();
    const int padding=frame+static_cast<int>(2*m_feedbackView->document()->documentMargin());
    m_feedbackView->setFixedHeight(qBound(2*lineHeight+padding,
        qCeil(m_feedbackView->document()->size().height())+frame,4*lineHeight+padding));
    QPalette feedbackPalette=m_feedbackView->palette();
    feedbackPalette.setColor(QPalette::Base,palette().color(QPalette::Window));
    m_feedbackView->setPalette(feedbackPalette);
    const bool dark=palette().color(QPalette::Window).lightness()<128;
    const QString positive=dark ? QStringLiteral("#70dc70") : QStringLiteral("#006400");
    const QString negative=dark ? QStringLiteral("#ff8585") : QStringLiteral("#c00000");
    const QString secondary=dark ? QStringLiteral("#b0b0b0") : QStringLiteral("#666666");
    m_autoHint->setStyleSheet(QStringLiteral("color:%1;").arg(secondary));
    m_feedbackView->setStyleSheet(m_feedback==Feedback::Failed || status.releaseIncomplete
        ? QStringLiteral("color:%1;").arg(negative)
        : m_feedback==Feedback::Passed ? QStringLiteral("color:%1;").arg(positive) : QString());
    const bool fallback=status.fallback && m_running;
    QString backend=status.connected ? backendName(status.backend) : (fallback ? QStringLiteral("SendInput") : tr("Not connected"));
    if (status.connected && preferences.backend==QStringLiteral("auto")) backend=tr("%1 (automatic selection)").arg(backend);
    const QString base=qEnvironmentVariable("ProgramW6432",qEnvironmentVariable("ProgramFiles"));
    const QString installed=version(base+QStringLiteral("/LGHUB/lghub_agent.exe"));
    const auto registration=ghubRegistration();
    bool processesKnown=false; const auto observedProcesses=processes(processesKnown);
    const auto razer=observeRazer();
    QStringList services;
    for (const auto &service:razer.services)
        services.append(QStringLiteral("%1 (%2)").arg(service.first,service.second ? tr("Running") : tr("Not running")));
    const QString missing=tr("Not detected"),queryFailed=tr("Query failed");
    const QString ghubState=registration==DriverRegistration::Registered ? tr("Driver registered")
        : registration==DriverRegistration::Missing ? tr("Driver not registered") : queryFailed;
    const QString razerState=!razer.interfaceKnown ? queryFailed : razer.interfaceObserved ? tr("Detected") : missing;
    const auto group=[&](const QString &title,const QString &state,bool detected,bool known,const QStringList &details) {
        QStringList escaped;
        for (const auto &line:details) escaped.append(line.toHtmlEscaped());
        const QString color=!known ? negative : detected ? positive : secondary;
        return QStringLiteral("<p style='margin-top:8px;margin-bottom:2px;'><b>%1: <span style='color:%2;'>%3 %4</span></b></p>"
            "<p style='margin-top:0px;margin-bottom:0px;margin-left:12px;color:%5;'>%6</p>")
            .arg(title.toHtmlEscaped(),color,detected ? QStringLiteral("●") : QStringLiteral("○"),state.toHtmlEscaped(),secondary,escaped.join(QStringLiteral("<br>")));
    };
    QString html=QStringLiteral("<table width='100%' cellspacing='0' cellpadding='0'><tr><td>%1</td>"
        "<td align='right' style='font-size:small;color:%2;'>%3</td></tr></table>")
        .arg(tr("Current backend: %1 (fallback: %2)").arg(backend,fallback ? tr("Yes") : tr("No")).toHtmlEscaped(),secondary,
            tr("Updated at %1").arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))).toHtmlEscaped());
    html+=group(tr("Logitech G HUB"),ghubState,registration==DriverRegistration::Registered,registration!=DriverRegistration::QueryFailed,
        {tr("Version: %1").arg(installed.isEmpty() ? tr("Unknown") : installed),
         tr("Processes: %1").arg(!processesKnown ? queryFailed : observedProcesses.isEmpty() ? missing : observedProcesses.join(QStringLiteral(", ")))});
    html+=group(tr("Razer driver"),razerState,razer.interfaceKnown && razer.interfaceObserved,razer.interfaceKnown,
        {tr("Services: %1").arg(!razer.servicesKnown ? queryFailed : services.isEmpty() ? missing : services.join(QStringLiteral(", ")))});
    // Preserve selection and scroll position while the visible timer updates the sample time.
    const QTextCursor previous=m_details->textCursor();
    const int scroll=m_details->verticalScrollBar()->value();
    m_details->document()->setDefaultFont(m_details->font()); m_details->setHtml(html);
    QTextCursor restored=m_details->textCursor(); const int end=m_details->document()->characterCount()-1;
    restored.setPosition(qMin(previous.anchor(),end)); restored.setPosition(qMin(previous.position(),end),QTextCursor::KeepAnchor);
    m_details->setTextCursor(restored); m_details->verticalScrollBar()->setValue(scroll);
    QString guidance;
    if (preferences.backend==QStringLiteral("razer")) {
        if (!razer.interfaceKnown)
            guidance=tr("QKeyMapper could not check the Razer driver status. Click Refresh status or Check connection.");
        else if (!razer.interfaceObserved)
            guidance=tr("QKeyMapper did not detect a Razer driver that IbInput can use. See the installation requirements in the Razer driver guide.");
    }
    const QColor link=dark ? QColor(114,178,255) : palette().color(QPalette::Link);
    m_help->setText(QStringLiteral("<style>a { color: %1; }</style>").arg(link.name())
        +tr("<a href=\"https://www.logitechg.com/innovation/g-hub.html\">G HUB</a> | <a href=\"https://github.com/Chaoses-Ib/IbInputSimulator/issues/7\">Razer driver guide</a><br>Connection and accepted calls do not prove that an application received input.")
        +(guidance.isEmpty() ? QString() : QStringLiteral("<br>")+guidance.toHtmlEscaped()));
}
void QkmIbInputPopup::setRunning(bool running) { if (running && !m_running) m_feedback=Feedback::None; m_running=running; if (isVisible()) refresh(); }
void QkmIbInputPopup::connectionChecked() { m_checkPending=false; if (m_feedback==Feedback::Checking) m_feedback=Feedback::None; if (isVisible()) refresh(); }
void QkmIbInputPopup::connectionChecked(bool success,const QString &backend) {
    m_feedbackAt=QDateTime::currentDateTime();
    if (m_checkedPreference!=QkmIbInputClient::instance().preferences().backend) {
        m_checkedBackend.clear(); m_feedback=Feedback::None;
    } else {
        m_checkedBackend=success ? backend : QString(); m_checkedAt=m_feedbackAt;
        m_feedback=success ? Feedback::Passed : Feedback::Failed;
    }
    connectionChecked();
}
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
void QkmIbInputPopup::changeEvent(QEvent *event) {
    QFrame::changeEvent(event);
    if (!m_help) return;
    if (event->type()==QEvent::LanguageChange) retranslate();
    else if (event->type()==QEvent::PaletteChange || event->type()==QEvent::FontChange) refresh();
}
void QkmIbInputPopup::showEvent(QShowEvent *event) { QFrame::showEvent(event); refresh(); m_timer.start(); }
void QkmIbInputPopup::hideEvent(QHideEvent *event) { m_timer.stop(); QFrame::hideEvent(event); }
