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
    layout->addWidget(m_revoke);
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
    approvalLayout->addWidget(m_read);
    approvalLayout->addWidget(m_configure);
    approvalLayout->addWidget(m_run);
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
    layout->addWidget(m_close, 0, Qt::AlignRight);
    // Enter must never approve a client or revoke a pairing accidentally.
    for (QPushButton *button : {m_approve, m_reject, m_revoke, m_close}) button->setAutoDefault(false);
    connect(m_enabled, &QCheckBox::toggled, this, &ControlDialog::setEnabledByUser);
    connect(m_close, &QPushButton::clicked, this, &QDialog::close);
    connect(m_pairings, &QListWidget::currentRowChanged, this, [this]() {
        m_revoke->setEnabled(m_server.isEnabled() && m_pairings->currentItem());
    });
    connect(m_revoke, &QPushButton::clicked, this, [this]() {
        if (const auto *item = m_pairings->currentItem()) {
            if (!m_server.revoke(item->data(Qt::UserRole).toString())) m_failed = true;
            refresh();
        }
    });
    connect(m_approve, &QPushButton::clicked, this, [this]() {
        quint32 scopes = Read;
        if (m_configure->isChecked()) scopes |= Configure;
        if (m_run->isChecked()) scopes |= RunMapping;
        if (!m_server.approvePairing(m_pending.requestId, scopes)) {
            m_failed = true;
            m_server.rejectPairing(m_pending.requestId);
            finishPending();
        } else {
            m_approve->setEnabled(false);
            m_configure->setEnabled(false);
            m_run->setEnabled(false);
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
        m_configure->setEnabled(true);
        m_run->setEnabled(true);
        m_approve->setEnabled(true);
        showPending();
    });
    connect(&m_server, &AutomationSessionServer::pairingFinished, this,
        [this](const QString &id, bool, const QString &) {
            if (id == m_pending.requestId) finishPending();
            refresh();
        });
    connect(&m_server, &AutomationSessionServer::enabledChanged, this, [this]() { refresh(); });
    connect(&m_server, &AutomationSessionServer::failed, this, [this]() { m_failed = true; refresh(); });
    setUILanguage(0);
    finishPending();
}

QString ControlDialog::text(const char *chinese, const char *english, const char *japanese) const
{
    return QString::fromUtf8(m_language == 1 ? english : m_language == 2 ? japanese : chinese);
}

QString ControlDialog::title(int language)
{
    return QString::fromUtf8(language == 1 ? "Agent / MCP Control" : language == 2 ? "Agent / MCP 操作" : "Agent / MCP 控制");
}

void ControlDialog::setUILanguage(int language)
{
    m_language = language;
    setWindowTitle(title(language));
    m_enabled->setText(text("启用本次运行的 MCP 接口", "Enable MCP for this run", "今回の実行で MCP を有効にする"));
    m_notice->setText(text("仅允许同一用户登录会话。请在此批准配对；关闭窗口不会关闭接口。重启软件后接口关闭。",
        "Only this user and logon session may connect. Approve pairing here. Closing this window keeps MCP enabled; restarting QKeyMapper disables it.",
        "同じユーザーのログオンセッションのみ接続できます。ここでペアリングを承認してください。画面を閉じても有効のままです。再起動すると無効になります。"));
    m_connection->setPlaceholderText(text("接口已关闭", "MCP is disabled", "MCP は無効です"));
    m_connection->setAccessibleName(text("连接信息文件路径", "Connection information file", "接続情報ファイルのパス"));
    m_connection->setToolTip(text("可复制此路径配置 MCP 桥接。文件不含配对密钥。",
        "Copy this path when configuring the MCP bridge. It contains no pairing key.",
        "MCP ブリッジの設定にこのパスをコピーできます。ペアリングキーは含まれません。"));
    m_pairings->setAccessibleName(text("已配对客户端及权限", "Paired clients and permissions", "ペアリング済みクライアントと権限"));
    m_revoke->setText(text("撤销选中客户端的配对", "Revoke selected pairing", "選択したペアリングを解除"));
    m_approval->setTitle(text("批准新的客户端（60 秒内）", "Approve a new client (within 60 seconds)", "新しいクライアントを承認（60 秒以内）"));
    m_executable->setAccessibleName(text("系统核实的客户端程序路径", "Verified client executable path", "確認済みクライアントのパス"));
    m_read->setText(text("读取状态与设定（必需）", "Read state and configuration (required)", "状態と設定の読み取り（必須）"));
    m_configure->setText(text("更改映射与软件设定", "Change mappings and settings", "マッピングと設定の変更"));
    m_run->setText(text("开始与停止映射（可能发送输入）", "Start and stop mapping (may send input)", "マッピングの開始と停止（入力を送信する場合あり）"));
    m_approve->setText(text("批准选定权限", "Approve selected permissions", "選択した権限で承認"));
    m_reject->setText(text("拒绝", "Reject", "拒否"));
    m_close->setText(text("关闭窗口", "Close window", "画面を閉じる"));
    showPending();
    refresh();
}

void ControlDialog::setEnabledByUser(bool enabled)
{
    m_failed = false;
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
    QString status = m_server.isEnabled() ? text("接口已启用", "MCP is enabled", "MCP は有効です")
                                         : text("接口已关闭", "MCP is disabled", "MCP は無効です");
    if (m_failed) status += text("；上次操作失败。未保存的撤销不能视为成功。",
        "; the last operation failed. A revocation that was not saved did not succeed.",
        "。前の操作に失敗しました。保存されなかった解除は完了していません。");
    m_status->setText(status);
    m_connection->setText(m_server.isEnabled() ? m_server.discoveryPath() : QString());
    const QString selected = m_pairings->currentItem() ? m_pairings->currentItem()->data(Qt::UserRole).toString() : QString();
    m_pairings->clear();
    const auto pairs = m_server.pairings();
    for (const PairingInfo &pair : pairs) {
        QStringList scopes{text("读取", "Read", "読取")};
        if (pair.scopes & Configure) scopes << text("设定", "Configure", "設定");
        if (pair.scopes & RunMapping) scopes << text("启停", "Run mapping", "開始・停止");
        if (pair.scopes & Observe) scopes << text("诊断", "Observe", "診断");
        if (pair.scopes & TestInput) scopes << text("真实输入测试", "Test input", "実入力テスト");
        if (pair.scopes & ManageDevices) scopes << text("设备管理", "Manage devices", "デバイス管理");
        if (pair.scopes & ExecuteCommands) scopes << text("执行命令", "Execute commands", "コマンド実行");
        auto *item = new QListWidgetItem(pair.clientName + QStringLiteral("\n") + scopes.join(QStringLiteral(", ")), m_pairings);
        item->setData(Qt::UserRole, pair.pairId);
        item->setToolTip(pair.pairId);
        if (pair.pairId == selected) m_pairings->setCurrentItem(item);
    }
    m_revoke->setEnabled(m_server.isEnabled() && m_pairings->currentItem());
}

void ControlDialog::showPending()
{
    m_approval->setVisible(!m_pending.requestId.isEmpty());
    if (m_pending.requestId.isEmpty()) { m_request->clear(); m_executable->clear(); return; }
    m_request->setText(text("客户端名称（自报）：%1\nPID：%2；创建时间：%3\n系统核实的程序：",
        "Client name (self-reported): %1\nPID: %2; creation time: %3\nVerified executable:",
        "クライアント名（自己申告）：%1\nPID：%2；作成時刻：%3\n確認済みプログラム：")
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
