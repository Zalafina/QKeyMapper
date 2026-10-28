#include "qkm_control_dialog.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <utility>

namespace QkmAutomation {
namespace {

QByteArray applicationBinding()
{
    const QString executable = QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath();
    const auto own = queryProcessIdentity(GetCurrentProcessId());
    if (executable.isEmpty() || !own.ok()) return {};
    QByteArray binding("QKeyMapper-MCP-App/v1");
    binding.append('\0').append(executable.toUtf8()).append('\0').append(QByteArray::number(own.identity.sessionId));
    return QCryptographicHash::hash(binding, QCryptographicHash::Sha256);
}

QString defaultStoreDirectory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QByteArray binding = applicationBinding();
    if (base.isEmpty() || binding.isEmpty()) return {};
    return QDir(base).filePath(QStringLiteral("QKeyMapper/MCP/")
        + QString::fromLatin1(binding.toHex()));
}

QLabel *plainLabel(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setFocusPolicy(Qt::ClickFocus);
    return label;
}

} // namespace

ControlDialog::ControlDialog(QWidget *parent)
    : ControlDialog(defaultStoreDirectory(), applicationBinding(), parent)
{
}

ControlDialog::ControlDialog(QString storeDirectory, QByteArray binding, QWidget *parent)
    : QDialog(parent), m_storeDirectory(std::move(storeDirectory)), m_server(m_storeDirectory, std::move(binding))
{
    setObjectName(QStringLiteral("mcpControlDialog"));
    setMinimumWidth(540);
    resize(650, 560);
    auto *layout = new QVBoxLayout(this);
    m_enabled = new QCheckBox(this);
    m_enabled->setObjectName(QStringLiteral("mcpEnabled"));
    m_notice = plainLabel(this);
    m_status = plainLabel(this);
    m_status->setObjectName(QStringLiteral("mcpStatus"));
    m_connection = new QLineEdit(this);
    m_connection->setObjectName(QStringLiteral("mcpConnection"));
    m_connection->setReadOnly(true);
    layout->addWidget(m_enabled);
    layout->addWidget(m_notice);
    layout->addWidget(m_status);
    layout->addWidget(m_connection);
    m_pairings = new QListWidget(this);
    m_pairings->setObjectName(QStringLiteral("mcpPairings"));
    m_pairings->setWordWrap(true);
    m_pairings->setMinimumHeight(85);
    layout->addWidget(m_pairings);
    m_revoke = new QPushButton(this);
    m_revoke->setObjectName(QStringLiteral("mcpRevoke"));
    m_windowPermissions = new QPushButton(this);
    m_windowPermissions->setObjectName(QStringLiteral("mcpWindowPermissions"));
    layout->addWidget(m_windowPermissions);
    layout->addWidget(m_revoke);
    m_windowApproval = new QDialog(this);
    m_windowApproval->setObjectName(QStringLiteral("mcpWindowApproval"));
    m_windowApproval->setMinimumWidth(500);
    auto *windowLayout = new QVBoxLayout(m_windowApproval);
    m_windowPair = plainLabel(m_windowApproval);
    m_windowPair->setObjectName(QStringLiteral("mcpWindowPair"));
    m_windowNotice = plainLabel(m_windowApproval);
    m_windowShow = new QCheckBox(m_windowApproval);
    m_windowShow->setObjectName(QStringLiteral("mcpWindowShow"));
    m_windowCapture = new QCheckBox(m_windowApproval);
    m_windowCapture->setObjectName(QStringLiteral("mcpWindowCapture"));
    windowLayout->addWidget(m_windowPair);
    windowLayout->addWidget(m_windowNotice);
    windowLayout->addWidget(m_windowShow);
    windowLayout->addWidget(m_windowCapture);
    auto *windowButtons = new QHBoxLayout;
    m_windowApprove = new QPushButton(m_windowApproval);
    m_windowApprove->setObjectName(QStringLiteral("mcpWindowApprove"));
    m_windowClose = new QPushButton(m_windowApproval);
    m_windowClose->setObjectName(QStringLiteral("mcpWindowClose"));
    windowButtons->addStretch();
    windowButtons->addWidget(m_windowApprove);
    windowButtons->addWidget(m_windowClose);
    windowLayout->addLayout(windowButtons);
    connect(m_windowClose, &QPushButton::clicked, m_windowApproval, &QDialog::reject);
    m_approval = new QGroupBox(this);
    auto *approvalLayout = new QVBoxLayout(m_approval);
    m_request = plainLabel(m_approval);
    m_request->setObjectName(QStringLiteral("mcpPairRequest"));
    approvalLayout->addWidget(m_request);
    m_executable = new QLineEdit(m_approval);
    m_executable->setObjectName(QStringLiteral("mcpClientExecutable"));
    m_executable->setReadOnly(true);
    approvalLayout->addWidget(m_executable);
    m_read = new QCheckBox(m_approval);
    m_read->setChecked(true);
    m_read->setEnabled(false);
    m_configure = new QCheckBox(m_approval);
    m_configure->setObjectName(QStringLiteral("mcpConfigure"));
    m_run = new QCheckBox(m_approval);
    m_run->setObjectName(QStringLiteral("mcpRunMapping"));
    m_showWindows = new QCheckBox(m_approval);
    m_showWindows->setObjectName(QStringLiteral("mcpShowWindows"));
    m_captureWindows = new QCheckBox(m_approval);
    m_captureWindows->setObjectName(QStringLiteral("mcpCaptureWindows"));
    approvalLayout->addWidget(m_read);
    approvalLayout->addWidget(m_configure);
    approvalLayout->addWidget(m_run);
    approvalLayout->addWidget(m_showWindows);
    approvalLayout->addWidget(m_captureWindows);
    auto *buttons = new QHBoxLayout;
    m_approve = new QPushButton(m_approval);
    m_approve->setObjectName(QStringLiteral("mcpApprove"));
    m_reject = new QPushButton(m_approval);
    m_reject->setObjectName(QStringLiteral("mcpReject"));
    buttons->addStretch();
    buttons->addWidget(m_approve);
    buttons->addWidget(m_reject);
    approvalLayout->addLayout(buttons);
    layout->addWidget(m_approval);
    m_close = new QPushButton(this);
    m_close->setObjectName(QStringLiteral("mcpClose"));
    layout->addWidget(m_close, 0, Qt::AlignRight);
    // Use mouse-driven actions; keep selection and copying available by click.
    for (QPushButton *button : {m_approve, m_reject, m_revoke, m_close, m_windowPermissions, m_windowApprove, m_windowClose}) {
        button->setAutoDefault(false);
        button->setFocusPolicy(Qt::NoFocus);
    }
    for (QCheckBox *check : {m_enabled, m_read, m_configure, m_run, m_showWindows, m_captureWindows, m_windowShow, m_windowCapture})
        check->setFocusPolicy(Qt::NoFocus);
    for (QLineEdit *edit : {m_connection, m_executable}) edit->setFocusPolicy(Qt::ClickFocus);
    m_pairings->setFocusPolicy(Qt::ClickFocus);
    connect(m_enabled, &QCheckBox::toggled, this, &ControlDialog::setEnabledByUser);
    connect(m_close, &QPushButton::clicked, this, &QDialog::close);
    connect(m_pairings, &QListWidget::itemSelectionChanged, this, [this]() {
        m_revoke->setEnabled(m_pairings->selectedItems().size() == 1);
        m_windowPermissions->setEnabled(m_server.isEnabled() && m_pending.requestId.isEmpty()
            && m_pairings->selectedItems().size() == 1);
    });
    connect(m_windowPermissions, &QPushButton::clicked, this, &ControlDialog::approveWindowPermissions);
    connect(m_revoke, &QPushButton::clicked, this, [this]() {
        // Accessibility selection can differ from the current keyboard-navigation row.
        const auto selected = m_pairings->selectedItems();
        if (selected.size() == 1) {
            const auto *item = selected.first();
            m_windowFailed = false;
            m_failed = !m_server.revoke(item->data(Qt::UserRole).toString());
            refresh();
        }
    });
    connect(m_approve, &QPushButton::clicked, this, [this]() {
        quint32 scopes = Read;
        if (m_configure->isChecked()) scopes |= Configure;
        if (m_run->isChecked()) scopes |= RunMapping;
        if (m_showWindows->isChecked()) scopes |= ShowWindows;
        if (m_captureWindows->isChecked()) scopes |= CaptureWindows;
        if (!m_server.approvePairing(m_pending.requestId, scopes)) {
            m_failed = true;
            m_server.rejectPairing(m_pending.requestId);
            finishPending();
        } else {
            m_approve->setEnabled(false);
            m_configure->setEnabled(false);
            m_run->setEnabled(false);
            m_showWindows->setEnabled(false);
            m_captureWindows->setEnabled(false);
        }
        refresh();
    });
    connect(m_reject, &QPushButton::clicked, this, [this]() {
        m_server.rejectPairing(m_pending.requestId);
        finishPending();
    });
    connect(&m_server, &AutomationSessionServer::pairingRequested, this, [this](const PairingRequest &request) {
        m_peerExecutable = queryProcessExecutable(request.peer);
        if (m_peerExecutable.isEmpty()) { m_server.rejectPairing(request.requestId); return; }
        m_pending = request;
        m_configure->setChecked(false);
        m_run->setChecked(false);
        m_showWindows->setChecked(false);
        m_captureWindows->setChecked(false);
        m_configure->setEnabled(true);
        m_run->setEnabled(true);
        m_showWindows->setEnabled(true);
        m_captureWindows->setEnabled(true);
        m_approve->setEnabled(true);
        showPending();
        m_windowPermissions->setEnabled(false);
    });
    connect(&m_server, &AutomationSessionServer::pairingFinished, this,
        [this](const QString &id, bool, const QString &) {
            if (id == m_pending.requestId) finishPending();
            refresh();
        });
    connect(&m_server, &AutomationSessionServer::enabledChanged, this, [this]() { refresh(); });
    connect(&m_server, &AutomationSessionServer::failed, this, [this]() { m_failed = true; refresh(); });
    retranslateUi();
    finishPending();
}

QString ControlDialog::title()
{
    return tr("Agent / MCP Control");
}

void ControlDialog::retranslateUi()
{
    setWindowTitle(title());
    m_enabled->setText(tr("Enable MCP for this run"));
    m_notice->setText(tr("Only this user and logon session may connect. Approve pairing here. Closing this window keeps MCP enabled; restarting QKeyMapper disables it."));
    m_connection->setPlaceholderText(tr("MCP is disabled"));
    m_connection->setAccessibleName(tr("Connection information file"));
    m_connection->setToolTip(tr("Copy this path when configuring the MCP bridge. It contains no pairing key."));
    m_pairings->setAccessibleName(tr("Paired clients and permissions"));
    m_revoke->setText(tr("Revoke selected pairing"));
    m_approval->setTitle(tr("Approve a new client (within 60 seconds)"));
    m_executable->setAccessibleName(tr("Verified client executable path"));
    m_read->setText(tr("Read state and configuration (required)"));
    m_configure->setText(tr("Change mappings and settings"));
    m_run->setText(tr("Start and stop mapping (may send input)"));
    const auto windowText = [this](QCheckBox *show, QCheckBox *capture) {
        show->setText(tr("Show windows"));
        show->setToolTip(tr("May restore or focus this instance's windows. Does not grant mapping execution or system input."));
        capture->setText(tr("Capture windows"));
        capture->setToolTip(tr("Captures only supported windows in this instance. Images may contain configuration and paths."));
    };
    windowText(m_showWindows, m_captureWindows);
    windowText(m_windowShow, m_windowCapture);
    m_windowPermissions->setText(tr("Approve window permissions"));
    m_windowApproval->setWindowTitle(tr("Approve window permissions"));
    m_windowNotice->setText(tr("Keep existing permissions and approve additional window access. Reconnect the bridge after approval."));
    m_windowApprove->setText(tr("Approve selected permissions"));
    m_windowClose->setText(tr("Close"));
    if (m_windowApproval->isVisible()) {
        const auto pairs = m_server.pairings();
        for (const PairingInfo &pair : pairs) {
            if (pair.pairId == m_windowPairId)
                m_windowPair->setText(tr("Selected pairing: %1\nPair ID: %2").arg(pair.clientName, pair.pairId));
        }
    }
    m_approve->setText(tr("Approve selected permissions"));
    m_reject->setText(tr("Reject"));
    m_close->setText(tr("Close"));
    showPending();
    refresh();
}

void ControlDialog::setEnabledByUser(bool enabled)
{
    m_failed = false;
    m_windowFailed = false;
    if (enabled) {
        if (m_storeDirectory.isEmpty() || !QDir().mkpath(QFileInfo(m_storeDirectory).absolutePath())
            || !m_server.enable()) m_failed = true;
    } else {
        m_server.disable();
        finishPending();
    }
    refresh();
}

void ControlDialog::refresh()
{
    const QSignalBlocker block(m_enabled);
    m_enabled->setChecked(m_server.isEnabled());
    QString status = m_server.isEnabled() ? tr("MCP is enabled")
                                         : tr("MCP is disabled");
    if (m_windowFailed) status += QStringLiteral("\n") + tr("The window permission update did not complete. MCP is disabled if saving failed.");
    else if (m_failed) status += tr("; the last operation failed. A revocation that was not saved did not succeed.");
    if (m_failed && m_server.lastStoreError() == PairStoreError::Busy)
        status += tr(" Another instance owns this pairing store; manage it there or disable its interface first.");
    m_status->setText(status);
    m_connection->setText(m_server.isEnabled() ? m_server.discoveryPath() : QString());
    const auto selectedItems = m_pairings->selectedItems();
    const QString selected = selectedItems.size() == 1 ? selectedItems.first()->data(Qt::UserRole).toString() : QString();
    m_pairings->clear();
    const auto pairs = m_server.pairings();
    for (const PairingInfo &pair : pairs) {
        QStringList scopes{tr("Read")};
        if (pair.scopes & Configure) scopes << tr("Configure");
        if (pair.scopes & RunMapping) scopes << tr("Run mapping");
        if (pair.scopes & Observe) scopes << tr("Observe");
        if (pair.scopes & TestInput) scopes << tr("Test input");
        if (pair.scopes & ManageDevices) scopes << tr("Manage devices");
        if (pair.scopes & ExecuteCommands) scopes << tr("Execute commands");
        if (pair.scopes & ShowWindows) scopes << tr("Show windows");
        if (pair.scopes & CaptureWindows) scopes << tr("Capture windows");
        auto *item = new QListWidgetItem(pair.clientName + QStringLiteral("\n") + scopes.join(QStringLiteral(", ")), m_pairings);
        item->setData(Qt::UserRole, pair.pairId);
        item->setToolTip(pair.pairId);
        if (pair.pairId == selected) m_pairings->setCurrentItem(item);
    }
    m_revoke->setEnabled(m_pairings->selectedItems().size() == 1);
    m_windowPermissions->setEnabled(m_server.isEnabled() && m_pending.requestId.isEmpty()
        && m_pairings->selectedItems().size() == 1);
}

void ControlDialog::approveWindowPermissions()
{
    const auto selected = m_pairings->selectedItems();
    if (!m_server.isEnabled() || !m_pending.requestId.isEmpty() || selected.size() != 1) return;
    const QString pairId = selected.first()->data(Qt::UserRole).toString();
    const auto pairs = m_server.pairings();
    for (const PairingInfo &pair : pairs) {
        if (pair.pairId != pairId) continue;
        m_windowPairId = pairId;
        m_windowPair->setText(tr("Selected pairing: %1\nPair ID: %2").arg(pair.clientName, pairId));
        m_windowShow->setChecked(pair.scopes & ShowWindows);
        m_windowShow->setEnabled(!(pair.scopes & ShowWindows));
        m_windowCapture->setChecked(pair.scopes & CaptureWindows);
        m_windowCapture->setEnabled(!(pair.scopes & CaptureWindows));
        const auto approve = connect(m_windowApprove, &QPushButton::clicked, m_windowApproval, [this, pairId]() {
            bool completed = false;
            const auto current = m_server.pairings();
            for (const PairingInfo &pair : current) {
                if (pair.pairId != pairId || !m_server.isEnabled()) continue;
                quint32 scopes = pair.scopes;
                if (m_windowShow->isEnabled() && m_windowShow->isChecked()) scopes |= ShowWindows;
                if (m_windowCapture->isEnabled() && m_windowCapture->isChecked()) scopes |= CaptureWindows;
                completed = scopes == pair.scopes || m_server.updatePermissions(pairId, scopes);
                break;
            }
            m_failed = m_windowFailed = !completed;
            refresh();
            m_windowApproval->accept();
        });
        m_windowApproval->exec();
        disconnect(approve);
        m_windowPairId.clear();
        return;
    }
}

void ControlDialog::showEvent(QShowEvent *event)
{
    if (!m_server.refreshPairings()) m_failed = true;
    refresh();
    QDialog::showEvent(event);
}

void ControlDialog::showPending()
{
    m_approval->setVisible(!m_pending.requestId.isEmpty());
    if (m_pending.requestId.isEmpty()) { m_request->clear(); m_executable->clear(); return; }
    m_request->setText(tr("Client name (self-reported): %1\nPID: %2; creation time: %3\nVerified executable:")
        .arg(m_pending.clientName, QString::number(m_pending.peer.processId), QString::number(m_pending.peer.creationTime)));
    // Keep even a long executable path selectable without pushing approval controls off screen.
    m_executable->setText(m_peerExecutable);
    m_executable->setCursorPosition(0);
}

void ControlDialog::finishPending()
{
    m_pending = {};
    m_peerExecutable.clear();
    showPending();
}

void ControlDialog::done(int result)
{
    if (!m_pending.requestId.isEmpty()) m_server.rejectPairing(m_pending.requestId);
    finishPending();
    QDialog::done(result);
}

} // namespace QkmAutomation
