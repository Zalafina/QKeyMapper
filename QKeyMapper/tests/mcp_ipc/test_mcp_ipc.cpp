#include "../../automation/qkm_ipc_frame.h"
#include "../../automation/qkm_local_identity.h"
#include "../../automation/qkm_local_listener.h"
#include "../../automation/qkm_pair_store.h"
#include "../../automation/qkm_session_server.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QEventLoop>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QProcess>
#include <QPointer>
#include <QScopeGuard>
#include <QTest>
#include <QTimer>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <QUuid>
#include <QtEndian>
#include <aclapi.h>
#include <sddl.h>
#include <limits>

using namespace QkmAutomation;

namespace {

QByteArray rawFrame(const QByteArray &payload)
{
    QByteArray bytes(4, '\0');
    qToBigEndian<quint32>(static_cast<quint32>(payload.size()), bytes.data());
    return bytes + payload;
}

FrameReadResult feed(FrameReader &reader, QByteArray bytes, QJsonObject &message)
{
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadOnly);
    return reader.read(buffer, message);
}

bool receiveMessage(QLocalSocket &socket, FrameReader &reader, QJsonObject &message, int timeoutMs = 3000)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    FrameReadResult result = FrameReadResult::Incomplete;
    const auto consume = [&]() {
        result = reader.read(socket, message);
        if (result != FrameReadResult::Incomplete
            || socket.state() == QLocalSocket::UnconnectedState) loop.quit();
    };
    QObject::connect(&socket, &QLocalSocket::readyRead, &loop, consume);
    QObject::connect(&socket, &QLocalSocket::disconnected, &loop, consume);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    consume();
    if (result == FrameReadResult::Incomplete
        && socket.state() != QLocalSocket::UnconnectedState) {
        timer.start(timeoutMs);
        loop.exec();
    }
    return result == FrameReadResult::Message;
}

int runPipeClient(const QStringList &arguments)
{
    if (arguments.size() != 5) return 10;
    const quint32 parentPid = arguments.at(3).toUInt();
    const quint64 parentCreated = arguments.at(4).toULongLong();
    const DWORD access = FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_EA | FILE_WRITE_EA
        | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE;
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(arguments.at(2).utf16()), access,
                                0, nullptr, OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                                nullptr);
    if (handle == INVALID_HANDLE_VALUE) return 11;
    auto closeHandle = qScopeGuard([handle]() { CloseHandle(handle); });
    const IdentityResult peer = queryPipePeerIdentity(handle, PipeSide::Client);
    const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
    if (!peer.ok() || !own.ok() || !sameLogonSession(peer.identity, own.identity)
        || peer.identity.processId != parentPid || peer.identity.creationTime != parentCreated)
        return 12;
    QLocalSocket socket;
    if (!socket.setSocketDescriptor(reinterpret_cast<qintptr>(handle))) return 13;
    closeHandle.dismiss();
    FrameError error;
    const QJsonObject request{{QStringLiteral("peer"), QStringLiteral("child")},
                             {QStringLiteral("claimed_pid"), QString::number(parentPid)}};
    const QByteArray frame = encodeFrame(request, MaxHandshakePayload, error);
    if (error != FrameError::None || socket.write(frame) != frame.size()) return 14;
    socket.flush();
    FrameReader reader(MaxHandshakePayload);
    QJsonObject reply;
    if (!receiveMessage(socket, reader, reply)
        || reply != QJsonObject{{QStringLiteral("peer"), QStringLiteral("parent")}}) return 15;
    return 0;
}

QJsonObject helloMessage(const QString &instance, bool pairing)
{
    QJsonObject message{{QStringLiteral("version"), HandshakeVersion},
                        {QStringLiteral("instance"), instance},
                        {QStringLiteral("client_nonce"), QString(64, QLatin1Char('a'))}};
    if (pairing) {
        message.insert(QStringLiteral("type"), QStringLiteral("pair.request"));
        message.insert(QStringLiteral("client_name"), QStringLiteral("测试客户端 日本語"));
    } else {
        message.insert(QStringLiteral("type"), QStringLiteral("hello"));
        message.insert(QStringLiteral("pair_id"), QString(32, QLatin1Char('b')));
    }
    return message;
}

AuthBinding proofBinding()
{
    AuthBinding binding;
    const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
    binding.instanceId = QStringLiteral("00112233445566778899aabbccddeeff");
    binding.pairId = QStringLiteral("ffeeddccbbaa99887766554433221100");
    binding.permissionEpoch = (std::numeric_limits<quint64>::max)();
    binding.client = own.identity;
    binding.server = own.identity;
    binding.client.processId = 0x12345678;
    binding.client.creationTime = Q_UINT64_C(0xfedcba9876543210);
    binding.server.processId = 0x87654321;
    binding.server.creationTime = Q_UINT64_C(0x0123456789abcdef);
    binding.clientNonce = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    return binding;
}

PairCredential storeCredential()
{
    return {QString(32, QLatin1Char('a')), QStringLiteral("测试 Agent 日本語"),
            (std::numeric_limits<quint64>::max)(), KnownScopes, QByteArray(AuthSecretBytes, 'k')};
}

QByteArray readTestFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

bool replaceTestFile(const QString &path, const QByteArray &bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(bytes) == bytes.size();
}

int runStoreReader(const QStringList &arguments)
{
    if (arguments.size() != 4) return 40;
    PairCredentialStore store(arguments.at(2), arguments.at(3).toUtf8());
    QList<PairCredential> records;
    PairStoreError error;
    if (!store.load(records, error)) return 50 + static_cast<int>(error);
    const PairCredential expected = storeCredential();
    if (records.size() != 1) return 41;
    const PairCredential &actual = records.first();
    return actual.pairId == expected.pairId && actual.clientName == expected.clientName
        && actual.permissionEpoch == expected.permissionEpoch && actual.scopes == expected.scopes
        && actual.secret == expected.secret ? 0 : 42;
}

int runSessionClient(const QStringList &arguments)
{
    if (arguments.size() != 8) return 60;
    const QString mode = arguments.at(7);
    const bool pairing = mode.startsWith(QStringLiteral("pair"));
    const DWORD access = FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_EA | FILE_WRITE_EA
        | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE;
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(arguments.at(2).utf16()), access,
        0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return 61;
    auto closeHandle = qScopeGuard([handle]() { CloseHandle(handle); });
    const IdentityResult server = queryPipePeerIdentity(handle, PipeSide::Client);
    const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
    if (!server.ok() || !own.ok() || !sameLogonSession(server.identity, own.identity)
        || server.identity.processId != arguments.at(4).toUInt()
        || server.identity.creationTime != arguments.at(5).toULongLong()) return 62;
    QLocalSocket socket;
    socket.setReadBufferSize(MaxHandshakePayload + 4);
    if (!socket.setSocketDescriptor(reinterpret_cast<qintptr>(handle))) return 63;
    closeHandle.dismiss();
    PairCredentialStore clientStore(arguments.at(6), "test-session-client");
    PairCredential credential;
    PairStoreError storeError;
    if (!pairing) {
        QList<PairCredential> records;
        if (!clientStore.load(records, storeError) || records.size() != 1) return 64;
        credential = records.first();
    }
    const QByteArray clientNonce = secureRandomSecret();
    if (clientNonce.size() != AuthSecretBytes) return 65;
    QJsonObject hello = helloMessage(arguments.at(3), pairing);
    hello.insert(QStringLiteral("client_nonce"), QString::fromLatin1(clientNonce.toHex()));
    if (!pairing) hello.insert(QStringLiteral("pair_id"), credential.pairId);
    if (mode == QStringLiteral("pair-claimed-approval")) hello.insert(QStringLiteral("confirmed"), true);
    const auto send = [&](const QJsonObject &message) {
        FrameError error;
        const QByteArray bytes = encodeFrame(message, MaxFramePayload, error);
        if (error != FrameError::None || socket.write(bytes) != bytes.size()) return false;
        socket.flush();
        return true;
    };
    if (!send(hello)) return 66;
    FrameReader reader(MaxHandshakePayload);
    QJsonObject challenge;
    const bool initialReject = mode == QStringLiteral("revoked") || mode == QStringLiteral("pair-claimed-approval")
        || mode == QStringLiteral("pair-rejected");
    if (!receiveMessage(socket, reader, challenge))
        return initialReject && socket.state() == QLocalSocket::UnconnectedState ? 0 : 67;
    if (initialReject) return 68;
    const QString expectedType = pairing ? QStringLiteral("pair.grant") : QStringLiteral("challenge");
    if (challenge.size() != (pairing ? 13 : 11)
        || challenge.value(QStringLiteral("type")) != QJsonValue(expectedType)
        || challenge.value(QStringLiteral("version")) != QJsonValue(HandshakeVersion)
        || challenge.value(QStringLiteral("instance")) != QJsonValue(arguments.at(3))
        || challenge.value(QStringLiteral("client_pid")) != QJsonValue(QString::number(own.identity.processId))
        || challenge.value(QStringLiteral("client_created")) != QJsonValue(QString::number(own.identity.creationTime))
        || challenge.value(QStringLiteral("server_pid")) != QJsonValue(QString::number(server.identity.processId))
        || challenge.value(QStringLiteral("server_created")) != QJsonValue(QString::number(server.identity.creationTime))) return 69;
    bool epochOk = false;
    const QString epoch = challenge.value(QStringLiteral("epoch")).toString();
    const quint64 number = epoch.toULongLong(&epochOk);
    const QString pairId = challenge.value(QStringLiteral("pair_id")).toString();
    const QString nonceHex = challenge.value(QStringLiteral("server_nonce")).toString();
    const QString proofHex = challenge.value(QStringLiteral("server_proof")).toString();
    if (!epochOk || !number || QString::number(number) != epoch || !isLowerHex(pairId, 32)
        || !isLowerHex(nonceHex, 64) || !isLowerHex(proofHex, 64)) return 70;
    if (pairing) {
        const QString secret = challenge.value(QStringLiteral("secret")).toString();
        if (!isLowerHex(secret, 64)) return 71;
        credential = {pairId, QStringLiteral("测试客户端 日本語"), number,
            static_cast<quint32>(challenge.value(QStringLiteral("scopes")).toInt()), QByteArray::fromHex(secret.toLatin1())};
        if (!validPairCredential(credential)) return 72;
    } else if (pairId != credential.pairId) return 73;
    AuthBinding binding;
    binding.instanceId = arguments.at(3); binding.pairId = pairId; binding.permissionEpoch = number;
    binding.client = own.identity; binding.server = server.identity; binding.clientNonce = clientNonce;
    const QByteArray nonce = QByteArray::fromHex(nonceHex.toLatin1());
    if (!equalAuthenticationProof(QByteArray::fromHex(proofHex.toLatin1()),
        authenticationProof(binding, nonce, ProofRole::Server, credential.secret))) return 74;
    if (mode == QStringLiteral("pair-no-confirm")) return 0;
    if (pairing && !clientStore.save({credential}, storeError)) return 75;
    const ProofRole role = pairing ? ProofRole::PairingConfirmation : ProofRole::Client;
    QByteArray proof = authenticationProof(binding, nonce, role, credential.secret);
    if (mode == QStringLiteral("bad-proof")) proof[0] = static_cast<char>(proof.at(0) ^ 1);
    if (mode == QStringLiteral("reflection")) proof = QByteArray::fromHex(proofHex.toLatin1());
    if (mode == QStringLiteral("pair-bad-confirm"))
        proof = authenticationProof(binding, nonce, ProofRole::Client, credential.secret);
    if (!send({{QStringLiteral("type"), pairing ? QStringLiteral("pair.confirm") : QStringLiteral("proof")},
               {QStringLiteral("proof"), QString::fromLatin1(proof.toHex())}})) return 76;
    QJsonObject ready;
    const bool rejectProof = mode == QStringLiteral("bad-proof") || mode == QStringLiteral("reflection")
        || mode == QStringLiteral("pair-bad-confirm") || mode == QStringLiteral("pair-store-failure");
    if (!receiveMessage(socket, reader, ready))
        return rejectProof && socket.state() == QLocalSocket::UnconnectedState ? 0 : 77;
    if (rejectProof) return 78;
    if (ready.size() != 6 || ready.value(QStringLiteral("type")) != QJsonValue(QStringLiteral("ready"))
        || ready.value(QStringLiteral("version")) != QJsonValue(HandshakeVersion)
        || ready.value(QStringLiteral("instance")) != QJsonValue(arguments.at(3))
        || ready.value(QStringLiteral("pair_id")) != QJsonValue(pairId)
        || ready.value(QStringLiteral("epoch")) != QJsonValue(epoch)) return 79;
    credential.permissionEpoch = number;
    credential.scopes = static_cast<quint32>(ready.value(QStringLiteral("scopes")).toInt());
    if (!validPairCredential(credential) || !clientStore.save({credential}, storeError)) return 80;
    reader = FrameReader(MaxFramePayload);
    socket.setReadBufferSize(MaxFramePayload + 4);
    if (mode == QStringLiteral("slow-frame")) {
        QElapsedTimer elapsed; elapsed.start();
        socket.write(QByteArray::fromHex("0004")); socket.flush();
        QJsonObject ignored;
        const bool received = receiveMessage(socket, reader, ignored, AuthorizedFrameTimeoutMs + 2000);
        return !received && socket.state() == QLocalSocket::UnconnectedState
            && elapsed.elapsed() >= AuthorizedFrameTimeoutMs ? 0 : 88;
    }
    if (mode == QStringLiteral("auth-idle")) QTest::qWait(PreauthTimeoutMs + 250);
    const QString requestId = QUuid::createUuid().toString(QUuid::Id128);
    QString method = QStringLiteral("qkm_get_state");
    if (mode == QStringLiteral("write-denied") || mode == QStringLiteral("write-allowed")) method = QStringLiteral("qkm_apply_changes");
    if (mode == QStringLiteral("unknown-method")) method = QStringLiteral("setProperty");
    QJsonObject args;
    if (mode == QStringLiteral("large")) args.insert(QStringLiteral("blob"), QString(8192, QLatin1Char('x')));
    if (!send({{QStringLiteral("id"), requestId}, {QStringLiteral("method"), method}, {QStringLiteral("arguments"), args}})) return 81;
    QJsonObject response;
    if (!receiveMessage(socket, reader, response))
        return mode == QStringLiteral("disabled-in-handler") && socket.state() == QLocalSocket::UnconnectedState ? 0 : 82;
    if (response.value(QStringLiteral("id")) != QJsonValue(requestId)) return 82;
    if (mode == QStringLiteral("write-denied") || mode == QStringLiteral("unknown-method")) {
        const QString code = mode == QStringLiteral("write-denied") ? QStringLiteral("permission_denied") : QStringLiteral("unsupported_method");
        return response.value(QStringLiteral("ok")) == QJsonValue(false)
            && response.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")) == QJsonValue(code) ? 0 : 83;
    }
    if (response.value(QStringLiteral("ok")) != QJsonValue(true)
        || response.value(QStringLiteral("result")).toObject().value(QStringLiteral("fixture")) != QJsonValue(true)) return 84;
    if (mode == QStringLiteral("large")
        && response.value(QStringLiteral("result")).toObject().value(QStringLiteral("blob")).toString().size() != 8192) return 85;
    if (mode == QStringLiteral("hold")) {
        QFile output;
        if (!output.open(stdout, QIODevice::WriteOnly) || output.write("holding\n") != 8 || !output.flush()) return 87;
        QJsonObject ignored;
        if (receiveMessage(socket, reader, ignored)) return 86;
    }
    return 0;
}

class SessionChild : public QProcess {
public:
    bool launch(const AutomationSessionServer &server, const QString &clientDirectory, const QString &mode)
    {
        const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
        if (!own.ok()) return false;
        start(QCoreApplication::applicationFilePath(), {QStringLiteral("--session-client"), server.pipeName(),
            server.instanceId(), QString::number(GetCurrentProcessId()), QString::number(own.identity.creationTime), clientDirectory, mode});
        return waitForStarted(3000);
    }
    ~SessionChild() override
    {
        if (state() != NotRunning) { kill(); waitForFinished(3000); }
    }
};

int runListenerClient(const QStringList &arguments)
{
    if (arguments.size() != 7) return 20;
    const DWORD access = FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_EA | FILE_WRITE_EA
        | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE;
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(arguments.at(2).utf16()), access,
        0, nullptr, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT
            | (arguments.at(6) == QStringLiteral("anonymous") ? SECURITY_ANONYMOUS : SECURITY_IDENTIFICATION), nullptr);
    if (handle == INVALID_HANDLE_VALUE) return 21;
    auto close = qScopeGuard([handle]() { CloseHandle(handle); });
    const IdentityResult peer = queryPipePeerIdentity(handle, PipeSide::Client);
    const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
    if (!peer.ok() || !own.ok() || !sameLogonSession(peer.identity, own.identity)
        || peer.identity.processId != arguments.at(4).toUInt()
        || peer.identity.creationTime != arguments.at(5).toULongLong()) return 22;
    QLocalSocket socket;
    socket.setReadBufferSize(MaxHandshakePayload + 4);
    if (!socket.setSocketDescriptor(reinterpret_cast<qintptr>(handle))) return 23;
    close.dismiss();
    const QString kind = arguments.at(6);
    QJsonObject message = helloMessage(arguments.at(3), kind == QStringLiteral("pair"));
    if (kind == QStringLiteral("version")) message.insert(QStringLiteral("version"), 2);
    if (kind == QStringLiteral("version-string")) message.insert(QStringLiteral("version"), QStringLiteral("1"));
    if (kind == QStringLiteral("instance")) message.insert(QStringLiteral("instance"), QString(32, QLatin1Char('0')));
    if (kind == QStringLiteral("unknown")) message.insert(QStringLiteral("confirmed"), true);
    if (kind == QStringLiteral("missing")) message.remove(QStringLiteral("client_nonce"));
    if (kind == QStringLiteral("nonce")) message.insert(QStringLiteral("client_nonce"), QString(64, QLatin1Char('A')));
    if (kind == QStringLiteral("pair-id")) message.insert(QStringLiteral("pair_id"), 1);
    if (kind == QStringLiteral("business")) message.insert(QStringLiteral("type"), QStringLiteral("qkm_apply_changes"));
    if (kind == QStringLiteral("name-control") || kind == QStringLiteral("name-long")) {
        message = helloMessage(arguments.at(3), true);
        message.insert(QStringLiteral("client_name"), kind == QStringLiteral("name-control")
            ? QString::fromUtf8("x\0y", 3) : QString(81, QLatin1Char('x')));
    }
    FrameError error;
    QByteArray bytes = encodeFrame(message, MaxHandshakePayload, error);
    if (error != FrameError::None) return 24;
    if (kind == QStringLiteral("oversize")) bytes = QByteArray::fromHex("00001001");
    if (kind == QStringLiteral("invalid-json")) bytes = rawFrame("{\"x\":}");
    if (kind == QStringLiteral("invalid-utf8")) bytes = rawFrame(QByteArray::fromHex("ff"));
    if (kind == QStringLiteral("truncated")) bytes = bytes.left(2);
    if (kind == QStringLiteral("coalesced")) bytes += rawFrame("{\"type\":\"qkm_apply_changes\"}");
    if (kind == QStringLiteral("flood")) bytes = QByteArray::fromHex("00001001") + QByteArray(65536, 'x');
    if (kind == QStringLiteral("idle") || kind == QStringLiteral("trickle")) bytes.clear();
    if (!bytes.isEmpty()) {
        if (socket.write(bytes) != bytes.size()) return 25;
        socket.flush();
    }
    QTimer trickle;
    QTimer extraMessage;
    extraMessage.setSingleShot(true);
    if (kind == QStringLiteral("second-message")) {
        QObject::connect(&extraMessage, &QTimer::timeout, &socket, [&socket]() {
            socket.write(rawFrame("{\"type\":\"qkm_apply_changes\"}"));
            socket.flush();
        });
        extraMessage.start(250);
    }
    if (kind == QStringLiteral("trickle")) {
        QObject::connect(&trickle, &QTimer::timeout, &socket, [&socket]() {
            socket.write("\0", 1);
            socket.flush();
        });
        // A valid incomplete 4096-byte frame: periodic traffic must not extend the deadline.
        socket.write(QByteArray::fromHex("00001000"));
        socket.flush();
        trickle.start(200);
    }
    QEventLoop loop;
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&socket, &QLocalSocket::disconnected, &loop, &QEventLoop::quit);
    QObject::connect(&watchdog, &QTimer::timeout, &loop, &QEventLoop::quit);
    if (socket.state() != QLocalSocket::UnconnectedState) {
        watchdog.start(6000);
        loop.exec();
    }
    return socket.state() == QLocalSocket::UnconnectedState && socket.bytesAvailable() == 0 ? 0 : 26;
}

class ListenerChild : public QProcess {
public:
    bool launch(const LocalPipeListener &listener, const QString &kind)
    {
        const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
        if (!own.ok()) return false;
        start(QCoreApplication::applicationFilePath(), {QStringLiteral("--listener-client"),
            listener.pipeName(), listener.instanceId(), QString::number(GetCurrentProcessId()),
            QString::number(own.identity.creationTime), kind});
        return waitForStarted(3000);
    }
    ~ListenerChild() override
    {
        if (state() != NotRunning) {
            kill();
            waitForFinished(3000);
        }
    }
};

class FailingDevice : public QIODevice {
public:
    FailingDevice() { open(QIODevice::ReadOnly | QIODevice::Unbuffered); }
    bool isSequential() const override { return true; }
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *, qint64) override { return -1; }
};

} // namespace

class McpIpcTest : public QObject {
    Q_OBJECT
private slots:
    void roundTrip_data();
    void roundTrip();
    void fragmented();
    void coalesced();
    void malformed_data();
    void malformed();
    void payloadLimits();
    void depthLimits();
    void encoderRejectsInvalidUnicode();
    void truncatedAndTerminal();
    void ioFailure();
    void currentIdentity();
    void identityMismatch();
    void unavailableIdentity();
    void crossProcessPipe();
    void listenerLifecycle();
    void listenerHandshake_data();
    void listenerHandshake();
    void listenerReject_data();
    void listenerReject();
    void listenerDeadline_data();
    void listenerDeadline();
    void listenerDisconnectAndReentry();
    void listenerSecondMessage();
    void requestContext_data();
    void requestContext();
    void requestContextDisposal();
    void proofVectors_data();
    void proofVectors();
    void proofBindingChanges();
    void proofRejectsInvalidBinding();
    void proofComparison();
    void challengeLifecycle();
    void storeRoundTrip();
    void storeInvalidInput();
    void storeCorruptionAndContext();
    void storeAtomicFailure();
    void storeUnsafePaths();
    void sessionPairAndReconnect();
    void sessionRejectedPairing_data();
    void sessionRejectedPairing();
    void sessionStaleApproval();
    void sessionStoreFailure();
    void sessionSlowFrame();
    void sessionHandlerLifecycle();
#ifdef QKM_AUTOMATION_TESTING
    void storeMalformedProtectedData();
    void listenerVerifierStopAndRestart();
    void listenerVerifierTimeout_data();
    void listenerVerifierTimeout();
    void listenerRestoreFailure();
#endif
};

void McpIpcTest::roundTrip_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::newRow("empty") << QByteArray("{}");
    QTest::newRow("unicode") << QByteArray("{\"text\":\"中文 日本語 😀\",\"空键\":null}");
    QTest::newRow("types") << QByteArray("{\"a\":[true,false,null,1.25,{\"x\":\"\\n\\u0000\"}]}");
    QTest::newRow("nul-key-value") << QByteArray("{\"a\\u0000b\":\"x\\u0000y\"}");
}

void McpIpcTest::roundTrip()
{
    QFETCH(QByteArray, json);
    const QJsonObject expected = QJsonDocument::fromJson(json).object();
    FrameError error;
    const QByteArray frame = encodeFrame(expected, MaxFramePayload, error);
    QCOMPARE(error, FrameError::None);
    QCOMPARE(qFromBigEndian<quint32>(frame.constData()), static_cast<quint32>(frame.size() - 4));
    FrameReader reader;
    QJsonObject message;
    QCOMPARE(feed(reader, frame, message), FrameReadResult::Message);
    QCOMPARE(message, expected);
    if (qstrcmp(QTest::currentDataTag(), "types") == 0) {
        QCOMPARE(message.value(QStringLiteral("a")).toArray().at(4).toObject()
                     .value(QStringLiteral("x")).toString(), QString::fromUtf8("\n\0", 2));
    }
    if (qstrcmp(QTest::currentDataTag(), "nul-key-value") == 0) {
        const QString key = QString::fromUtf8("a\0b", 3);
        QVERIFY(message.contains(key));
        QCOMPARE(message.value(key).toString(), QString::fromUtf8("x\0y", 3));
    }
    QVERIFY(reader.finish());
}

void McpIpcTest::fragmented()
{
    const QByteArray frame = rawFrame("{\"text\":\"中文😀\",\"n\":123}");
    const QJsonObject expected = QJsonDocument::fromJson(frame.mid(4)).object();
    for (int split = 1; split < frame.size(); ++split) {
        FrameReader reader;
        QJsonObject message{{QStringLiteral("stale"), true}};
        QCOMPARE(feed(reader, frame.left(split), message), FrameReadResult::Incomplete);
        QVERIFY(message.isEmpty());
        QCOMPARE(feed(reader, frame.mid(split), message), FrameReadResult::Message);
        QCOMPARE(message, expected);
        QVERIFY(reader.finish());
    }
    FrameReader bytewise;
    QJsonObject message;
    for (int i = 0; i < frame.size(); ++i)
        QCOMPARE(feed(bytewise, frame.mid(i, 1), message),
                 i + 1 == frame.size() ? FrameReadResult::Message : FrameReadResult::Incomplete);
    QCOMPARE(message, expected);
}

void McpIpcTest::coalesced()
{
    const QByteArray first = rawFrame("{\"id\":1}");
    QByteArray bytes = first + rawFrame("{\"id\":2}");
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    FrameReader reader;
    QJsonObject message;
    QCOMPARE(reader.read(buffer, message), FrameReadResult::Message);
    QCOMPARE(message.value(QStringLiteral("id")).toInt(), 1);
    QCOMPARE(buffer.pos(), qint64(first.size()));
    QCOMPARE(reader.read(buffer, message), FrameReadResult::Message);
    QCOMPARE(message.value(QStringLiteral("id")).toInt(), 2);
    QCOMPARE(reader.read(buffer, message), FrameReadResult::Incomplete);
    QVERIFY(reader.finish());
}

void McpIpcTest::malformed_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<int>("expected");
    const auto row = [](const char *name, const QByteArray &payload, FrameError error) {
        QTest::newRow(name) << rawFrame(payload) << static_cast<int>(error);
    };
    QTest::newRow("zero") << QByteArray(4, '\0') << static_cast<int>(FrameError::InvalidLength);
    QTest::newRow("uint-max") << QByteArray::fromHex("ffffffff") << static_cast<int>(FrameError::TooLarge);
    row("array-root", "[]", FrameError::InvalidJson);
    row("scalar-root", "true", FrameError::InvalidJson);
    row("syntax", "{\"x\":}", FrameError::InvalidJson);
    row("multiple-roots", "{}{}", FrameError::InvalidJson);
    row("nan", "{\"x\":NaN}", FrameError::InvalidJson);
    row("overflow-number", "{\"x\":1e400}", FrameError::InvalidJson);
    row("bom", QByteArray::fromHex("efbbbf") + "{}", FrameError::InvalidUtf8);
    row("invalid-utf8", QByteArray("{\"x\":\"") + QByteArray::fromHex("c080") + "\"}", FrameError::InvalidUtf8);
    row("utf8-surrogate", QByteArray("{\"x\":\"") + QByteArray::fromHex("eda080") + "\"}", FrameError::InvalidUtf8);
    row("escaped-surrogate", "{\"x\":\"\\ud800\"}", FrameError::InvalidUnicode);
    row("surrogate-key", "{\"\\udc00\":1}", FrameError::InvalidUnicode);
}

void McpIpcTest::malformed()
{
    QFETCH(QByteArray, bytes);
    QFETCH(int, expected);
    FrameReader reader;
    QJsonObject message{{QStringLiteral("stale"), true}};
    QCOMPARE(feed(reader, bytes, message), FrameReadResult::Failed);
    QCOMPARE(static_cast<int>(reader.error()), expected);
    QVERIFY(message.isEmpty());
    QCOMPARE(feed(reader, rawFrame("{}"), message), FrameReadResult::Failed);
    QCOMPARE(static_cast<int>(reader.error()), expected);
    QVERIFY(!reader.finish());
}

void McpIpcTest::payloadLimits()
{
    for (int limit : {MaxHandshakePayload, MaxFramePayload}) {
        const QJsonObject message{{QStringLiteral("x"), QString(limit - 8, QLatin1Char('x'))}};
        FrameError error;
        const QByteArray frame = encodeFrame(message, limit, error);
        QCOMPARE(error, FrameError::None);
        QCOMPARE(frame.size(), limit + 4);
        FrameReader reader(limit);
        QJsonObject decoded;
        QCOMPARE(feed(reader, frame, decoded), FrameReadResult::Message);
        QCOMPARE(decoded, message);
        QJsonObject tooLarge = message;
        tooLarge.insert(QStringLiteral("x"), QString(limit - 7, QLatin1Char('x')));
        QVERIFY(encodeFrame(tooLarge, limit, error).isEmpty());
        QCOMPARE(error, FrameError::TooLarge);
        FrameReader limitedReader(limit);
        const QByteArray header = rawFrame(QByteArray(limit + 1, ' ')).left(4);
        QCOMPARE(feed(limitedReader, header, decoded), FrameReadResult::Failed);
        QCOMPARE(limitedReader.error(), FrameError::TooLarge);
    }
    for (int limit : {-1, 0, MaxFramePayload + 1}) {
        FrameReader reader(limit);
        QJsonObject message;
        QCOMPARE(feed(reader, rawFrame("{}"), message), FrameReadResult::Failed);
        QCOMPARE(reader.error(), FrameError::InvalidLimit);
        FrameError error;
        QVERIFY(encodeFrame({}, limit, error).isEmpty());
        QCOMPARE(error, FrameError::InvalidLimit);
    }
}

void McpIpcTest::depthLimits()
{
    for (int arrays : {MaxJsonDepth - 1, MaxJsonDepth}) {
        const QByteArray payload = "{\"x\":" + QByteArray(arrays, '[') + "0"
            + QByteArray(arrays, ']') + "}";
        FrameReader reader;
        QJsonObject message;
        const bool allowed = arrays < MaxJsonDepth;
        QCOMPARE(feed(reader, rawFrame(payload), message),
                 allowed ? FrameReadResult::Message : FrameReadResult::Failed);
        QCOMPARE(reader.error(), allowed ? FrameError::None : FrameError::TooDeep);
        FrameError error;
        const QByteArray encoded = encodeFrame(QJsonDocument::fromJson(payload).object(),
                                               MaxFramePayload, error);
        QCOMPARE(error, allowed ? FrameError::None : FrameError::TooDeep);
        QCOMPARE(encoded.isEmpty(), !allowed);
    }
}

void McpIpcTest::encoderRejectsInvalidUnicode()
{
    const QString bad(QChar(0xd800));
    FrameError error;
    QVERIFY(encodeFrame({{QStringLiteral("x"), bad}}, MaxFramePayload, error).isEmpty());
    QCOMPARE(error, FrameError::InvalidUnicode);
    QVERIFY(encodeFrame({{bad, true}}, MaxFramePayload, error).isEmpty());
    QCOMPARE(error, FrameError::InvalidUnicode);
    QVERIFY(encodeFrame({{QStringLiteral("x"), std::numeric_limits<double>::infinity()}},
                        MaxFramePayload, error).isEmpty());
    QCOMPARE(error, FrameError::InvalidJson);
    QVERIFY(encodeFrame({{QStringLiteral("x"), std::numeric_limits<double>::quiet_NaN()}},
                        MaxFramePayload, error).isEmpty());
    QCOMPARE(error, FrameError::InvalidJson);
}

void McpIpcTest::truncatedAndTerminal()
{
    const QByteArray frame = rawFrame("{\"a\":123}");
    for (int size = 0; size < frame.size(); ++size) {
        FrameReader reader;
        QJsonObject message;
        QCOMPARE(feed(reader, frame.left(size), message), FrameReadResult::Incomplete);
        QCOMPARE(reader.finish(), size == 0);
        QCOMPARE(reader.error(), size ? FrameError::Truncated : FrameError::None);
    }
    FrameReader closed;
    QVERIFY(closed.finish());
    QJsonObject message;
    QCOMPARE(feed(closed, frame, message), FrameReadResult::Failed);
    QCOMPARE(closed.error(), FrameError::Closed);
}

void McpIpcTest::ioFailure()
{
    for (int size : {0, 2, 4, 5}) {
        FailingDevice device;
        FrameReader reader;
        QJsonObject message;
        if (size) QCOMPARE(feed(reader, rawFrame("{}").left(size), message), FrameReadResult::Incomplete);
        QCOMPARE(reader.read(device, message), FrameReadResult::Failed);
        QCOMPARE(reader.error(), FrameError::IoError);
    }
}

void McpIpcTest::currentIdentity()
{
    const IdentityResult first = queryProcessIdentity(GetCurrentProcessId());
    const IdentityResult second = queryProcessIdentity(GetCurrentProcessId());
    QVERIFY(first.ok());
    QVERIFY(second.ok());
    QVERIFY(sameProcess(first.identity, second.identity));
    QVERIFY(first.identity.creationTime > 0);
}

void McpIpcTest::identityMismatch()
{
    const IdentityResult result = queryProcessIdentity(GetCurrentProcessId());
    QVERIFY(result.ok());
    const ProcessIdentity own = result.identity;
    ProcessIdentity changed = own;
    ++changed.processId;
    QVERIFY(sameLogonSession(own, changed));
    QVERIFY(!sameProcess(own, changed));
    changed = own;
    ++changed.creationTime;
    QVERIFY(sameLogonSession(own, changed));
    QVERIFY(!sameProcess(own, changed));
    changed = own;
    ++changed.sessionId;
    QVERIFY(!sameLogonSession(own, changed));
    changed = own;
    ++changed.authenticationId;
    QVERIFY(!sameLogonSession(own, changed));
    changed = own;
    changed.userSid[changed.userSid.size() - 1] = static_cast<char>(changed.userSid.back() ^ 1);
    QVERIFY(changed.isValid());
    QVERIFY(!sameLogonSession(own, changed));
    changed = own;
    changed.logonSid[changed.logonSid.size() - 1] = static_cast<char>(changed.logonSid.back() ^ 1);
    QVERIFY(changed.isValid());
    QVERIFY(!sameLogonSession(own, changed));
    changed = own;
    changed.userSid = QByteArray(12, '\0');
    changed.userSid[1] = static_cast<char>(255);
    QVERIFY(!changed.isValid());
    QVERIFY(!sameLogonSession({}, {}));
    QVERIFY(!sameProcess(own, {}));
}

void McpIpcTest::unavailableIdentity()
{
    for (quint32 pid : {quint32(0), quint32(0xffffffff)}) {
        const IdentityResult result = queryProcessIdentity(pid);
        QVERIFY(!result.ok());
        QVERIFY(!result.identity.isValid());
        QCOMPARE(result.error, IdentityError::ProcessUnavailable);
    }
    for (PipeSide side : {PipeSide::Server, PipeSide::Client}) {
        const IdentityResult result = queryPipePeerIdentity(INVALID_HANDLE_VALUE, side);
        QVERIFY(!result.ok());
        QCOMPARE(result.error, IdentityError::PeerUnavailable);
    }
}

void McpIpcTest::crossProcessPipe()
{
    const IdentityResult own = queryProcessIdentity(GetCurrentProcessId());
    QVERIFY(own.ok());
    LPWSTR sid = nullptr;
    QVERIFY(ConvertSidToStringSidW(const_cast<char *>(own.identity.logonSid.constData()), &sid));
    const auto freeSid = qScopeGuard([sid]() { LocalFree(sid); });
    const QString sddl = QStringLiteral("D:P(A;;0x0012019b;;;%1)").arg(QString::fromWCharArray(sid));
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    QVERIFY(ConvertStringSecurityDescriptorToSecurityDescriptorW(
        reinterpret_cast<LPCWSTR>(sddl.utf16()), SDDL_REVISION_1, &descriptor, nullptr));
    const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
    const QString name = QStringLiteral("\\\\.\\pipe\\qkm-ipc-test-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);
    HANDLE pipe = CreateNamedPipeW(reinterpret_cast<LPCWSTR>(name.utf16()),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1, 4096, 4096, 3000, &attributes);
    QVERIFY(pipe != INVALID_HANDLE_VALUE);
    auto closePipe = qScopeGuard([pipe]() { CloseHandle(pipe); });
    HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    QVERIFY(event != nullptr);
    const auto closeEvent = qScopeGuard([event]() { CloseHandle(event); });
    OVERLAPPED overlapped{};
    overlapped.hEvent = event;
    const BOOL connected = ConnectNamedPipe(pipe, &overlapped);
    QVERIFY(connected || GetLastError() == ERROR_IO_PENDING);
    bool pending = !connected;
    const auto cancelPending = qScopeGuard([&]() {
        if (pending) {
            CancelIoEx(pipe, &overlapped);
            DWORD ignored = 0;
            GetOverlappedResult(pipe, &overlapped, &ignored, TRUE);
        }
    });
    QProcess child;
    const auto stopChild = qScopeGuard([&]() {
        if (child.state() != QProcess::NotRunning) {
            child.kill();
            child.waitForFinished(3000);
        }
    });
    child.start(QCoreApplication::applicationFilePath(),
        {QStringLiteral("--pipe-client"), name, QString::number(GetCurrentProcessId()),
         QString::number(own.identity.creationTime)});
    QVERIFY(child.waitForStarted(3000));
    if (pending) {
        QCOMPARE(WaitForSingleObject(event, 3000), DWORD(WAIT_OBJECT_0));
        DWORD ignored = 0;
        QVERIFY(GetOverlappedResult(pipe, &overlapped, &ignored, FALSE));
        pending = false;
    }
    const quint32 childPid = static_cast<quint32>(child.processId());
    const IdentityResult expected = queryProcessIdentity(childPid);
    const IdentityResult peer = queryPipePeerIdentity(pipe, PipeSide::Server);
    QVERIFY(expected.ok());
    QVERIFY(peer.ok());
    QVERIFY(sameProcess(peer.identity, expected.identity));
    QVERIFY(sameLogonSession(peer.identity, own.identity));
    QVERIFY(!sameProcess(peer.identity, own.identity));
    QLocalSocket socket;
    QVERIFY(socket.setSocketDescriptor(reinterpret_cast<qintptr>(pipe)));
    closePipe.dismiss();
    FrameReader reader(MaxHandshakePayload);
    QJsonObject message;
    QVERIFY(receiveMessage(socket, reader, message));
    QCOMPARE(message.value(QStringLiteral("claimed_pid")).toString(), QString::number(GetCurrentProcessId()));
    QVERIFY(peer.identity.processId != message.value(QStringLiteral("claimed_pid")).toString().toUInt());
    FrameError error;
    const QByteArray reply = encodeFrame({{QStringLiteral("peer"), QStringLiteral("parent")}},
                                         MaxHandshakePayload, error);
    QCOMPARE(error, FrameError::None);
    QCOMPARE(socket.write(reply), qint64(reply.size()));
    socket.flush();
    QVERIFY(child.waitForFinished(3000));
    QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    QCOMPARE(child.exitCode(), 0);
    QVERIFY(child.readAllStandardError().isEmpty());
    QVERIFY(!queryProcessIdentity(childPid).ok());
}

void McpIpcTest::listenerLifecycle()
{
    LocalPipeListener listener;
    QVERIFY(!listener.isListening());
    QVERIFY(listener.pipeName().isEmpty());
    QVERIFY(listener.instanceId().isEmpty());
    listener.stop();
    QString previous;
    for (int i = 0; i < 20; ++i) {
        QVERIFY(listener.start());
        QVERIFY(listener.isListening());
        QVERIFY(listener.instanceId() != previous);
        previous = listener.instanceId();
        const QString name = listener.pipeName();
        QVERIFY(listener.start());
        QCOMPARE(listener.pipeName(), name);
        listener.stop();
        QVERIFY(!listener.isListening());
        QVERIFY(listener.pipeName().isEmpty());
        HANDLE gone = CreateFileW(reinterpret_cast<LPCWSTR>(name.utf16()), FILE_READ_DATA,
                                   0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        const DWORD error = GetLastError();
        if (gone != INVALID_HANDLE_VALUE) CloseHandle(gone);
        QCOMPARE(gone, INVALID_HANDLE_VALUE);
        QCOMPARE(error, DWORD(ERROR_FILE_NOT_FOUND));
        QCoreApplication::processEvents();
    }
    { LocalPipeListener pending; QVERIFY(pending.start()); }
    QCoreApplication::processEvents();
}

void McpIpcTest::listenerHandshake_data()
{
    QTest::addColumn<QString>("kind");
    QTest::newRow("hello") << QStringLiteral("hello");
    QTest::newRow("pair") << QStringLiteral("pair");
}

void McpIpcTest::listenerHandshake()
{
    QFETCH(QString, kind);
    LocalPipeListener listener;
    int received = 0, failures = 0;
    ProcessIdentity peer;
    QJsonObject hello;
    connect(&listener, &LocalPipeListener::failed, this, [&]() { ++failures; });
    connect(&listener, &LocalPipeListener::handshakeReceived, this,
        [&](const QJsonObject &message, const ProcessIdentity &identity) {
            ++received;
            hello = message;
            peer = identity;
        });
    QVERIFY(listener.start());
    const QString name = listener.pipeName();
    for (int i = 0; i < 4; ++i) {
        ListenerChild child;
        QVERIFY(child.launch(listener, kind));
        QTRY_COMPARE_WITH_TIMEOUT(received, i + 1, 3000);
        QCOMPARE(hello, helloMessage(listener.instanceId(), kind == QStringLiteral("pair")));
        const IdentityResult actual = queryProcessIdentity(static_cast<quint32>(child.processId()));
        QVERIFY(actual.ok());
        QVERIFY(sameProcess(peer, actual.identity));
        const auto sockets = listener.findChildren<QLocalSocket *>();
        QLocalSocket *active = nullptr;
        for (QLocalSocket *socket : sockets) {
            if (socket->state() == QLocalSocket::ConnectedState) active = socket;
        }
        if (!active) QFAIL("No connected server socket");
        QCOMPARE(active->readBufferSize(), qint64(MaxHandshakePayload + 4));
        QCOMPARE(active->bytesToWrite(), qint64(0));
        PSECURITY_DESCRIPTOR security = nullptr;
        PACL dacl = nullptr;
        QCOMPARE(GetSecurityInfo(reinterpret_cast<HANDLE>(active->socketDescriptor()),
            SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dacl, nullptr,
            &security), DWORD(ERROR_SUCCESS));
        const auto freeSecurity = qScopeGuard([security]() { LocalFree(security); });
        SECURITY_DESCRIPTOR_CONTROL control = 0;
        DWORD revision = 0;
        QVERIFY(GetSecurityDescriptorControl(security, &control, &revision));
        QVERIFY(control & SE_DACL_PROTECTED);
        QVERIFY(dacl && dacl->AceCount == 1);
        void *rawAce = nullptr;
        QVERIFY(GetAce(dacl, 0, &rawAce));
        const auto *ace = static_cast<const ACCESS_ALLOWED_ACE *>(rawAce);
        QCOMPARE(ace->Header.AceType, BYTE(ACCESS_ALLOWED_ACE_TYPE));
        QCOMPARE(ace->Mask, DWORD(0x0012019b));
        const auto own = queryProcessIdentity(GetCurrentProcessId());
        QVERIFY(own.ok());
        QVERIFY(EqualSid(const_cast<DWORD *>(&ace->SidStart), const_cast<char *>(own.identity.logonSid.constData())));
        listener.closeClient();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 0);
        QVERIFY(child.readAllStandardError().isEmpty());
        QCOMPARE(listener.pipeName(), name);
        HANDLE takeover = CreateNamedPipeW(reinterpret_cast<LPCWSTR>(name.utf16()),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_BYTE, 1, 4096,
            4096, 1000, nullptr);
        const DWORD error = GetLastError();
        if (takeover != INVALID_HANDLE_VALUE) CloseHandle(takeover);
        QCOMPARE(takeover, INVALID_HANDLE_VALUE);
        QCOMPARE(error, DWORD(ERROR_ACCESS_DENIED));
    }
    QCOMPARE(failures, 0);
}

void McpIpcTest::listenerReject_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<int>("reason");
    for (const char *kind : {"version", "version-string", "instance", "unknown", "missing",
                            "nonce", "pair-id", "business", "name-control", "name-long", "coalesced"})
        QTest::newRow(kind) << QString::fromLatin1(kind) << int(ListenerError::InvalidHandshake);
    for (const char *kind : {"oversize", "invalid-json", "invalid-utf8", "flood"})
        QTest::newRow(kind) << QString::fromLatin1(kind) << int(ListenerError::InvalidFrame);
    QTest::newRow("anonymous") << QStringLiteral("anonymous") << int(ListenerError::IdentityRejected);
}

void McpIpcTest::listenerReject()
{
    QFETCH(QString, kind);
    QFETCH(int, reason);
    LocalPipeListener listener;
    int received = 0, failures = 0;
    QList<ListenerError> closed;
    connect(&listener, &LocalPipeListener::handshakeReceived, this, [&]() { ++received; });
    connect(&listener, &LocalPipeListener::clientClosed, this, [&](ListenerError e) { closed.append(e); });
    connect(&listener, &LocalPipeListener::failed, this, [&]() { ++failures; });
    QVERIFY(listener.start());
    ListenerChild child;
    QVERIFY(child.launch(listener, kind));
    QTRY_COMPARE_WITH_TIMEOUT(closed.size(), 1, 3000);
    QCOMPARE(int(closed.first()), reason);
    // A byte stream may deliver the valid first frame before the coalesced second frame.
    if (kind == QStringLiteral("coalesced")) QVERIFY(received <= 1);
    else QCOMPARE(received, 0);
    QCOMPARE(failures, 0);
    QVERIFY(listener.isListening());
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    QCOMPARE(child.exitCode(), 0);
    QVERIFY(child.readAllStandardError().isEmpty());
    ListenerChild reconnect;
    const int before = received;
    QVERIFY(reconnect.launch(listener, QStringLiteral("hello")));
    QTRY_COMPARE_WITH_TIMEOUT(received, before + 1, 3000);
    listener.closeClient();
    QTRY_COMPARE_WITH_TIMEOUT(reconnect.state(), QProcess::NotRunning, 3000);
    QCOMPARE(reconnect.exitCode(), 0);
}

void McpIpcTest::listenerDeadline_data()
{
    QTest::addColumn<QString>("kind");
    for (const char *kind : {"idle", "trickle", "truncated", "hello"})
        QTest::newRow(kind) << QString::fromLatin1(kind);
}

void McpIpcTest::listenerDeadline()
{
    QFETCH(QString, kind);
    LocalPipeListener listener;
    QList<ListenerError> closed;
    int received = 0;
    connect(&listener, &LocalPipeListener::handshakeReceived, this, [&]() { ++received; });
    connect(&listener, &LocalPipeListener::clientClosed, this, [&](ListenerError e) { closed.append(e); });
    QVERIFY(listener.start());
    ListenerChild child;
    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(child.launch(listener, kind));
    QTRY_COMPARE_WITH_TIMEOUT(closed.size(), 1, PreauthTimeoutMs + 1500);
    QCOMPARE(closed.first(), ListenerError::Timeout);
    QVERIFY(elapsed.elapsed() >= PreauthTimeoutMs);
    QVERIFY(elapsed.elapsed() < PreauthTimeoutMs + 1500);
    QCOMPARE(received, kind == QStringLiteral("hello") ? 1 : 0);
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
}

void McpIpcTest::listenerDisconnectAndReentry()
{
    LocalPipeListener listener;
    int received = 0, closed = 0, failures = 0;
    connect(&listener, &LocalPipeListener::handshakeReceived, this, [&]() { ++received; });
    connect(&listener, &LocalPipeListener::clientClosed, this, [&]() { ++closed; });
    connect(&listener, &LocalPipeListener::failed, this, [&]() { ++failures; });
    QVERIFY(listener.start());
    {
        ListenerChild child;
        QVERIFY(child.launch(listener, QStringLiteral("hello")));
        QTRY_COMPARE_WITH_TIMEOUT(received, 1, 3000);
        child.kill();
        QVERIFY(child.waitForFinished(3000));
        QTRY_COMPARE_WITH_TIMEOUT(closed, 1, 3000);
    }
    const auto stopOnHello = connect(&listener, &LocalPipeListener::handshakeReceived,
        this, [&]() { listener.stop(); });
    {
        ListenerChild child;
        QVERIFY(child.launch(listener, QStringLiteral("pair")));
        QTRY_COMPARE_WITH_TIMEOUT(received, 2, 3000);
        QVERIFY(!listener.isListening());
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
        QCOMPARE(child.exitCode(), 0);
    }
    disconnect(stopOnHello);
    QVERIFY(listener.start());
    {
        ListenerChild child;
        QVERIFY(child.launch(listener, QStringLiteral("hello")));
        QTRY_COMPARE_WITH_TIMEOUT(received, 3, 3000);
        listener.closeClient();
        listener.stop();
        QVERIFY(listener.start());
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
        QCOMPARE(child.exitCode(), 0);
    }
    {
        ListenerChild child;
        QVERIFY(child.launch(listener, QStringLiteral("hello")));
        QTRY_COMPARE_WITH_TIMEOUT(received, 4, 3000);
        listener.closeClient();
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
        QCOMPARE(child.exitCode(), 0);
    }
    QCOMPARE(failures, 0);
}

void McpIpcTest::listenerSecondMessage()
{
    LocalPipeListener listener;
    int received = 0;
    QList<ListenerError> closed;
    connect(&listener, &LocalPipeListener::handshakeReceived, this, [&]() { ++received; });
    connect(&listener, &LocalPipeListener::clientClosed, this, [&](ListenerError e) { closed.append(e); });
    QVERIFY(listener.start());
    ListenerChild child;
    QVERIFY(child.launch(listener, QStringLiteral("second-message")));
    QTRY_COMPARE_WITH_TIMEOUT(received, 1, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(closed.size(), 1, 3000);
    QCOMPARE(closed.first(), ListenerError::InvalidHandshake);
    QCOMPARE(received, 1);
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
}

void McpIpcTest::requestContext_data()
{
    QTest::addColumn<QString>("kind");
    for (const char *kind : {"normal", "changed-process", "changed-session"})
        QTest::newRow(kind) << QString::fromLatin1(kind);
#ifdef QKM_AUTOMATION_TESTING
    QTest::newRow("restore-failure") << QStringLiteral("restore-failure");
#endif
}

void McpIpcTest::requestContext()
{
    QFETCH(QString, kind);
    LocalPipeListener listener;
    ProcessIdentity peer;
    connect(&listener, &LocalPipeListener::handshakeReceived, this,
        [&](const QJsonObject &, const ProcessIdentity &identity) { peer = identity; });
    QVERIFY(listener.start());
    ListenerChild child;
    QVERIFY(child.launch(listener, kind == QStringLiteral("anonymous") ? kind : QStringLiteral("hello")));
    QTRY_VERIFY_WITH_TIMEOUT(peer.isValid(), 3000);
    const auto sockets = listener.findChildren<QLocalSocket *>();
    QCOMPARE(sockets.size(), 1);
    const HANDLE pipe = reinterpret_cast<HANDLE>(sockets.first()->socketDescriptor());
#ifdef QKM_AUTOMATION_TESTING
    setRequestIdentityTestHooks(nullptr, kind == QStringLiteral("restore-failure"));
    const auto resetHooks = qScopeGuard([]() { setRequestIdentityTestHooks(nullptr, false); });
#endif
    ProcessIdentity expected = peer;
    if (kind == QStringLiteral("changed-process")) ++expected.creationTime;
    if (kind == QStringLiteral("changed-session")) ++expected.sessionId;
    PipeRequestVerifier verifier;
    QVERIFY(!verifier.isRunning());
    QVERIFY(!verifier.start(INVALID_HANDLE_VALUE, expected));
    QVERIFY(!verifier.start(nullptr, expected));
    QVERIFY(!verifier.start(pipe, {}));
    int done = 0;
    DWORD callbackThread = 0;
    IdentityResult result;
    connect(&verifier, &PipeRequestVerifier::finished, this, [&](const IdentityResult &identity) {
        ++done;
        callbackThread = GetCurrentThreadId();
        result = identity;
    });
    HANDLE originalToken = nullptr;
    QVERIFY(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &originalToken));
    QCOMPARE(GetLastError(), DWORD(ERROR_NO_TOKEN));
    for (int i = 0; i < 3; ++i) {
        QVERIFY(verifier.start(pipe, expected));
        QVERIFY(verifier.isRunning());
        QVERIFY(!verifier.start(pipe, expected));
        QTRY_COMPARE_WITH_TIMEOUT(done, i + 1, 3000);
        QVERIFY(!verifier.isRunning());
        QCOMPARE(callbackThread, GetCurrentThreadId());
        if (kind == QStringLiteral("normal")) {
            QCOMPARE(result.error, IdentityError::None);
            QVERIFY(result.ok());
            QVERIFY(sameProcess(result.identity, peer));
        } else {
            QVERIFY(!result.ok());
            QVERIFY(!result.identity.isValid());
            QCOMPARE(result.error, kind == QStringLiteral("restore-failure")
                ? IdentityError::ContextRestoreFailed : IdentityError::RequestMismatch);
        }
        QVERIFY(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &originalToken));
        QCOMPARE(GetLastError(), DWORD(ERROR_NO_TOKEN));
    }
    listener.closeClient();
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
}

void McpIpcTest::requestContextDisposal()
{
    LocalPipeListener listener;
    ProcessIdentity peer;
    connect(&listener, &LocalPipeListener::handshakeReceived, this,
        [&](const QJsonObject &, const ProcessIdentity &identity) { peer = identity; });
    QVERIFY(listener.start());
    const QString name = listener.pipeName();
    ListenerChild child;
    QVERIFY(child.launch(listener, QStringLiteral("hello")));
    QTRY_VERIFY_WITH_TIMEOUT(peer.isValid(), 3000);
    const auto sockets = listener.findChildren<QLocalSocket *>();
    QCOMPARE(sockets.size(), 1);
    const HANDLE pipe = reinterpret_cast<HANDLE>(sockets.first()->socketDescriptor());
    int callbacks = 0;
    for (int i = 0; i < 32; ++i) {
        PipeRequestVerifier verifier;
        connect(&verifier, &PipeRequestVerifier::finished, this, [&]() { ++callbacks; });
        QVERIFY(verifier.start(pipe, peer));
    }
    listener.stop();
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
    const auto gone = [&]() {
        return !WaitNamedPipeW(reinterpret_cast<LPCWSTR>(name.utf16()), NMPWAIT_NOWAIT)
            && GetLastError() == ERROR_FILE_NOT_FOUND;
    };
    QTRY_VERIFY_WITH_TIMEOUT(gone(), 3000);
    QCOMPARE(callbacks, 0);
}

void McpIpcTest::proofVectors_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("server") << 1 << QByteArray("707f9b7b991818ffac75ed543bf43e5bbc13beaa56a821a496207772369ca8b0");
    QTest::newRow("client") << 2 << QByteArray("832a59307600f9ca81e2a58f282f54ecd6f492e8c149662a15371ac05e359f38");
    QTest::newRow("pairing") << 3 << QByteArray("d0846e2424fc8034f6723ac5d5e7bdc951af20a7de76f1f3a42485fa9a3b824b");
}

void McpIpcTest::proofVectors()
{
    QFETCH(int, role);
    QFETCH(QByteArray, expected);
    const AuthBinding binding = proofBinding();
    const QByteArray nonce = QByteArray::fromHex("202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    const QByteArray key = QByteArray::fromHex("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    // These fixed bytes and MACs were independently produced with Python struct/hmac/hashlib.
    QByteArray transcript = QByteArray::fromHex(
        "514b65794d61707065722d4d43502d4155544800010000000100112233445566778899aabbccddeeff"
        "ffeeddccbbaa99887766554433221100ffffffffffffffff12345678fedcba987654321087654321"
        "0123456789abcdef000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
        "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    transcript[20] = static_cast<char>(role);
    QCOMPARE(authTranscript(binding, nonce, static_cast<ProofRole>(role)), transcript);
    QCOMPARE(authenticationProof(binding, nonce, static_cast<ProofRole>(role), key).toHex(), expected);
}

void McpIpcTest::proofBindingChanges()
{
    const AuthBinding original = proofBinding();
    const QByteArray key(AuthSecretBytes, 'k'), nonce(AuthSecretBytes, 'n');
    const QByteArray expected = authenticationProof(original, nonce, ProofRole::Client, key);
    QCOMPARE(expected.size(), AuthSecretBytes);
    QList<AuthBinding> changed;
    AuthBinding value = original; value.instanceId[0] = QLatin1Char('f'); changed.append(value);
    value = original; value.pairId[0] = QLatin1Char('0'); changed.append(value);
    value = original; --value.permissionEpoch; changed.append(value);
    value = original; ++value.client.processId; changed.append(value);
    value = original; ++value.client.creationTime; changed.append(value);
    value = original; ++value.server.processId; changed.append(value);
    value = original; ++value.server.creationTime; changed.append(value);
    value = original; value.clientNonce[0] = 'x'; changed.append(value);
    for (const AuthBinding &binding : changed)
        QVERIFY(!equalAuthenticationProof(expected, authenticationProof(binding, nonce, ProofRole::Client, key)));
    QByteArray otherNonce = nonce; otherNonce[0] = 'x';
    QVERIFY(!equalAuthenticationProof(expected, authenticationProof(original, otherNonce, ProofRole::Client, key)));
    QVERIFY(!equalAuthenticationProof(expected, authenticationProof(original, nonce, ProofRole::Server, key)));
    QVERIFY(!equalAuthenticationProof(expected, authenticationProof(original, nonce, ProofRole::PairingConfirmation, key)));
    QVERIFY(!equalAuthenticationProof(expected, authenticationProof(original, nonce, ProofRole::Client, QByteArray(AuthSecretBytes, 'x'))));
}

void McpIpcTest::proofRejectsInvalidBinding()
{
    const AuthBinding original = proofBinding();
    const QByteArray key(AuthSecretBytes, 'k'), nonce(AuthSecretBytes, 'n');
    QList<AuthBinding> invalid;
    AuthBinding value = original; value.version = 2; invalid.append(value);
    value = original; value.permissionEpoch = 0; invalid.append(value);
    value = original; value.instanceId.clear(); invalid.append(value);
    value = original; value.instanceId[0] = QLatin1Char('A'); invalid.append(value);
    value = original; value.pairId[0] = QLatin1Char('g'); invalid.append(value);
    value = original; value.client = {}; invalid.append(value);
    value = original; value.server = {}; invalid.append(value);
    value = original; ++value.server.sessionId; invalid.append(value);
    value = original; ++value.server.authenticationId; invalid.append(value);
    value = original; value.clientNonce.chop(1); invalid.append(value);
    for (const AuthBinding &binding : invalid) {
        QVERIFY(authTranscript(binding, nonce, ProofRole::Client).isEmpty());
        QVERIFY(authenticationProof(binding, nonce, ProofRole::Client, key).isEmpty());
    }
    for (int size : {0, AuthSecretBytes - 1, AuthSecretBytes + 1}) {
        QVERIFY(authenticationProof(original, nonce, ProofRole::Client, QByteArray(size, 'k')).isEmpty());
        QVERIFY(authTranscript(original, QByteArray(size, 'n'), ProofRole::Client).isEmpty());
    }
    QVERIFY(authTranscript(original, nonce, ProofRole::Invalid).isEmpty());
    QVERIFY(!isLowerHex(QStringLiteral("a"), -1));
    QVERIFY(!isLowerHex(QStringLiteral("a"), 0));
}

void McpIpcTest::proofComparison()
{
    const QByteArray expected(AuthSecretBytes, 'p');
    QVERIFY(equalAuthenticationProof(expected, expected));
    for (int i = 0; i < AuthSecretBytes; ++i) {
        QByteArray changed = expected;
        changed[i] = 'x';
        QVERIFY(!equalAuthenticationProof(expected, changed));
    }
    QVERIFY(!equalAuthenticationProof({}, {}));
    QVERIFY(!equalAuthenticationProof(expected, expected.left(AuthSecretBytes - 1)));
    QVERIFY(!equalAuthenticationProof(expected, expected + 'x'));
}

void McpIpcTest::challengeLifecycle()
{
    const AuthBinding binding = proofBinding();
    const QByteArray key = secureRandomSecret();
    QCOMPARE(key.size(), AuthSecretBytes);
    const QByteArray secondKey = secureRandomSecret();
    QCOMPARE(secondKey.size(), AuthSecretBytes);
    QVERIFY(key != secondKey);
    ServerChallenge challenge;
    QVERIFY(!challenge.isPending());
    QVERIFY(!challenge.acceptClient(QByteArray(AuthSecretBytes, 'x')));
    QVERIFY(challenge.begin(binding, key));
    QVERIFY(challenge.isPending());
    const QByteArray nonce = challenge.serverNonce();
    QCOMPARE(nonce.size(), AuthSecretBytes);
    QCOMPARE(challenge.serverProof(), authenticationProof(binding, nonce, ProofRole::Server, key));
    QVERIFY(!challenge.acceptClient(challenge.serverProof()));
    QVERIFY(!challenge.isPending());
    const QByteArray oldProof = authenticationProof(binding, nonce, ProofRole::Client, key);
    QVERIFY(!challenge.acceptClient(oldProof));
    QVERIFY(challenge.begin(binding, key));
    QVERIFY(challenge.serverNonce() != nonce);
    QVERIFY(!challenge.acceptClient(oldProof));
    QVERIFY(challenge.begin(binding, key));
    const QByteArray proof = authenticationProof(binding, challenge.serverNonce(), ProofRole::Client, key);
    QVERIFY(challenge.acceptClient(proof));
    QVERIFY(!challenge.acceptClient(proof));
    QVERIFY(challenge.begin(binding, key));
    challenge.cancel();
    QVERIFY(!challenge.isPending());
    QVERIFY(challenge.serverNonce().isEmpty());
    QVERIFY(challenge.serverProof().isEmpty());
    QVERIFY(!challenge.acceptClient(proof));
    QVERIFY(challenge.begin(binding, key));
    QVERIFY(!challenge.begin({}, key));
    QVERIFY(!challenge.isPending());
    QVERIFY(challenge.serverNonce().isEmpty());
    QVERIFY(!challenge.begin(binding, {}));
    QVERIFY(!challenge.begin(binding, key, ProofRole::Invalid));
    QVERIFY(!challenge.begin(binding, key, ProofRole::Server));
    QVERIFY(challenge.begin(binding, key, ProofRole::PairingConfirmation));
    QVERIFY(!challenge.acceptClient(authenticationProof(binding, challenge.serverNonce(), ProofRole::Client, key)));
    QVERIFY(challenge.begin(binding, key, ProofRole::PairingConfirmation));
    QVERIFY(challenge.acceptClient(authenticationProof(binding, challenge.serverNonce(), ProofRole::PairingConfirmation, key)));
#ifdef QKM_AUTOMATION_TESTING
    setAuthRandomTestFailure(true);
    const auto resetRandom = qScopeGuard([]() { setAuthRandomTestFailure(false); });
    QVERIFY(secureRandomSecret().isEmpty());
    QVERIFY(!challenge.begin(binding, key));
    QVERIFY(!challenge.isPending());
    QVERIFY(challenge.serverNonce().isEmpty());
    QVERIFY(challenge.serverProof().isEmpty());
#endif
}

void McpIpcTest::storeRoundTrip()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/pairs-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString directory = temporary.path() + QStringLiteral("/private");
    const QByteArray binding("test-application");
    PairCredentialStore store(directory, binding);
    QList<PairCredential> records{storeCredential()};
    PairStoreError error;
    QVERIFY(store.load(records, error));
    QCOMPARE(error, PairStoreError::None);
    QVERIFY(records.isEmpty());
    QVERIFY(!QFileInfo::exists(directory));
    const PairCredential expected = storeCredential();
    QVERIFY2(store.save({expected}, error), qPrintable(QStringLiteral("save error %1").arg(int(error))));
    QCOMPARE(error, PairStoreError::None);
    const QByteArray encrypted = readTestFile(store.filePath());
    QVERIFY(!encrypted.isEmpty());
    QVERIFY(!encrypted.contains(expected.secret));
    QVERIFY(!encrypted.contains(expected.secret.toHex()));
    QVERIFY(!encrypted.contains(expected.clientName.toUtf8()));
    QVERIFY(!encrypted.contains(expected.pairId.toLatin1()));
    PairCredentialStore reopened(directory, binding);
    QVERIFY(reopened.load(records, error));
    QCOMPARE(records.size(), 1);
    QCOMPARE(records.first().pairId, expected.pairId);
    QCOMPARE(records.first().clientName, expected.clientName);
    QCOMPARE(records.first().permissionEpoch, expected.permissionEpoch);
    QCOMPARE(records.first().scopes, expected.scopes);
    QVERIFY(records.first().secret == expected.secret);
    QProcess child;
    child.start(QCoreApplication::applicationFilePath(),
        {QStringLiteral("--store-reader"), directory, QString::fromUtf8(binding)});
    QVERIFY(child.waitForStarted(3000));
    if (!child.waitForFinished(5000)) {
        child.kill(); child.waitForFinished(3000); QFAIL("Store reader did not exit");
    }
    QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    QCOMPARE(child.exitCode(), 0);
    QVERIFY(child.readAllStandardError().isEmpty());
    QVERIFY(store.save({}, error));
    QVERIFY(reopened.load(records, error));
    QVERIFY(records.isEmpty());
}

void McpIpcTest::storeInvalidInput()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/pairs-XXXXXX"));
    QVERIFY(temporary.isValid());
    PairCredentialStore store(temporary.path() + QStringLiteral("/private"), "test-application");
    PairStoreError error;
    const PairCredential original = storeCredential();
    QVERIFY(store.save({original}, error));
    const QByteArray before = readTestFile(store.filePath());
    QList<PairCredential> invalid;
    PairCredential value = original; value.pairId[0] = QLatin1Char('A'); invalid.append(value);
    value = original; value.clientName.clear(); invalid.append(value);
    value = original; value.clientName = QString::fromUtf8("x\0y", 3); invalid.append(value);
    value = original; value.permissionEpoch = 0; invalid.append(value);
    value = original; value.scopes = Configure; invalid.append(value);
    value = original; value.scopes = Read | 128; invalid.append(value);
    value = original; value.secret.chop(1); invalid.append(value);
    for (const PairCredential &credential : invalid) {
        QVERIFY(!validPairCredential(credential));
        QVERIFY(!store.save({credential}, error));
        QCOMPARE(error, PairStoreError::InvalidData);
        QCOMPARE(readTestFile(store.filePath()), before);
    }
    QVERIFY(!store.save({original, original}, error));
    QList<PairCredential> tooMany;
    for (int i = 0; i <= MaxPairCredentials; ++i) {
        value = original; value.pairId = QString::number(i, 16).rightJustified(32, QLatin1Char('0'));
        tooMany.append(value);
    }
    QVERIFY(!store.save(tooMany, error));
    QCOMPARE(readTestFile(store.filePath()), before);
    PairCredentialStore noContext(temporary.path() + QStringLiteral("/other"), {});
    QVERIFY(!noContext.save({original}, error));
    QCOMPARE(error, PairStoreError::InvalidContext);
    QVERIFY(!QFileInfo::exists(temporary.path() + QStringLiteral("/other")));
}

void McpIpcTest::storeCorruptionAndContext()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/pairs-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString directory = temporary.path() + QStringLiteral("/private");
    PairCredentialStore store(directory, "test-application");
    PairStoreError error;
    QVERIFY(store.save({storeCredential()}, error));
    const QByteArray original = readTestFile(store.filePath());
    QList<PairCredential> records{storeCredential()};
    PairCredentialStore wrong(directory, "other-application");
    QVERIFY(!wrong.load(records, error));
    QVERIFY(records.isEmpty());
    QCOMPARE(error, PairStoreError::ProtectionFailed);
    QVERIFY(!wrong.save({storeCredential()}, error));
    QCOMPARE(readTestFile(store.filePath()), original);
    QList<QByteArray> corrupt{QByteArray(), QByteArray("bad"), original.left(16), QByteArray(65537, '\0')};
    QByteArray changed = original; changed[0] = static_cast<char>(changed.at(0) ^ 1); corrupt.append(changed);
    for (const QByteArray &bytes : corrupt) {
        QVERIFY(replaceTestFile(store.filePath(), bytes));
        records = {storeCredential()};
        QVERIFY(!store.load(records, error));
        QVERIFY(records.isEmpty());
        QVERIFY(!store.save({storeCredential()}, error));
        QCOMPARE(readTestFile(store.filePath()), bytes);
    }
    QVERIFY(replaceTestFile(store.filePath(), original));
    QVERIFY(store.load(records, error));
    QCOMPARE(records.size(), 1);
}

void McpIpcTest::storeAtomicFailure()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/pairs-XXXXXX"));
    QVERIFY(temporary.isValid());
    PairCredentialStore store(temporary.path() + QStringLiteral("/private"), "test-application");
    PairStoreError error;
    QVERIFY(store.save({storeCredential()}, error));
    const QByteArray original = readTestFile(store.filePath());
    const QString path = QDir::toNativeSeparators(store.filePath());
    QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()), FILE_ATTRIBUTE_READONLY));
    const auto restore = qScopeGuard([path]() { SetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()), FILE_ATTRIBUTE_NORMAL); });
    QVERIFY(!store.save({}, error));
    QVERIFY(error != PairStoreError::None);
    QCOMPARE(readTestFile(store.filePath()), original);
    QList<PairCredential> records;
    QVERIFY(store.load(records, error));
    QCOMPARE(records.size(), 1);
    QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()), FILE_ATTRIBUTE_NORMAL));
    QVERIFY(store.save({}, error));
    QVERIFY(store.load(records, error));
    QVERIFY(records.isEmpty());
}

void McpIpcTest::storeUnsafePaths()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/pairs-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString publicDirectory = temporary.path() + QStringLiteral("/public");
    QVERIFY(QDir().mkdir(publicDirectory));
    PairCredentialStore publicStore(publicDirectory, "test-application");
    PairStoreError error;
    QVERIFY(!publicStore.save({storeCredential()}, error));
    QCOMPARE(error, PairStoreError::UnsafeStorage);
    QVERIFY(!QFileInfo::exists(publicStore.filePath()));
    PairCredentialStore store(temporary.path() + QStringLiteral("/private"), "test-application");
    QVERIFY(store.save({storeCredential()}, error));
    const QString link = QDir::toNativeSeparators(temporary.path() + QStringLiteral("/alias.dat"));
    const QString file = QDir::toNativeSeparators(store.filePath());
    QVERIFY(CreateHardLinkW(reinterpret_cast<LPCWSTR>(link.utf16()), reinterpret_cast<LPCWSTR>(file.utf16()), nullptr));
    const auto deleteLink = qScopeGuard([link]() { DeleteFileW(reinterpret_cast<LPCWSTR>(link.utf16())); });
    QList<PairCredential> records{storeCredential()};
    QVERIFY(!store.load(records, error));
    QVERIFY(records.isEmpty());
    QCOMPARE(error, PairStoreError::UnsafeStorage);
    QVERIFY(!store.save({}, error));
    QVERIFY(DeleteFileW(reinterpret_cast<LPCWSTR>(link.utf16())));
    QVERIFY(store.load(records, error));
    QCOMPARE(records.size(), 1);
}

void McpIpcTest::sessionPairAndReconnect()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/sessions-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString serverDirectory = temporary.path() + QStringLiteral("/server");
    const QString clientDirectory = temporary.path() + QStringLiteral("/client");
    AutomationSessionServer server(serverDirectory, "test-session-server");
    QVERIFY(!server.isEnabled());
    QVERIFY(server.pipeName().isEmpty());
    int authenticated = 0, paired = 0, handled = 0;
    connect(&server, &AutomationSessionServer::pairingRequested, this, [&](const PairingRequest &request) {
        QVERIFY(request.peer.processId != GetCurrentProcessId());
        QVERIFY(server.approvePairing(request.requestId, Read));
    });
    connect(&server, &AutomationSessionServer::pairingFinished, this,
        [&](const QString &, bool success, const QString &) { if (success) ++paired; });
    connect(&server, &AutomationSessionServer::authenticated, this, [&]() { ++authenticated; });
    server.setRequestHandler([&](AutomationRequest request) {
        ++handled;
        QJsonObject result{{QStringLiteral("fixture"), true}};
        if (request.arguments.contains(QStringLiteral("blob"))) result.insert(QStringLiteral("blob"), request.arguments.value(QStringLiteral("blob")));
        QVERIFY(server.reply(request.connectionId, request.requestId, result));
    });
    QVERIFY(server.enable());
    QVERIFY(server.acceptingConnections());
    const auto run = [&](const QString &mode) {
        SessionChild child;
        if (!child.launch(server, clientDirectory, mode)) return false;
        QElapsedTimer timer; timer.start();
        while (child.state() != QProcess::NotRunning && timer.elapsed() < 7000) QTest::qWait(20);
        const bool ok = child.state() == QProcess::NotRunning && child.exitStatus() == QProcess::NormalExit
            && child.exitCode() == 0 && child.readAllStandardError().isEmpty();
        if (!ok) qWarning("Session client failed in mode %s, exit %d", qPrintable(mode), child.exitCode());
        return ok;
    };
    QVERIFY(run(QStringLiteral("pair")));
    QTRY_COMPARE_WITH_TIMEOUT(paired, 1, 3000);
    QCOMPARE(authenticated, 1);
    QCOMPARE(handled, 1);
    QCOMPARE(server.pairings().size(), 1);
    const QString pairId = server.pairings().at(0).pairId;
    const auto ready = [&]() { QTRY_VERIFY_WITH_TIMEOUT(server.acceptingConnections(), 3000); };
    ready();
    QVERIFY(run(QStringLiteral("write-denied")));
    QCOMPARE(handled, 1);
    ready();
    QVERIFY(run(QStringLiteral("unknown-method")));
    QCOMPARE(handled, 1);
    ready();
    const int beforeBadProof = authenticated;
    QVERIFY(run(QStringLiteral("bad-proof")));
    QCOMPARE(authenticated, beforeBadProof);
    ready();
    QVERIFY(run(QStringLiteral("reflection")));
    QCOMPARE(authenticated, beforeBadProof);
    ready();
    QVERIFY(run(QStringLiteral("large")));
    QCOMPARE(handled, 2);
    ready();
    QVERIFY(run(QStringLiteral("auth-idle")));
    QCOMPARE(handled, 3);
    ready();
    QVERIFY(server.updatePermissions(pairId, Read | Configure));
    QCOMPARE(server.pairings().at(0).permissionEpoch, quint64(2));
    QVERIFY(run(QStringLiteral("write-allowed")));
    QCOMPARE(handled, 4);
    ready();
    server.disable();
    QVERIFY(!server.isEnabled());
    QVERIFY(server.pipeName().isEmpty());
    QVERIFY(server.enable());
    QCOMPARE(server.pairings().size(), 1);
    QVERIFY(run(QStringLiteral("auth")));
    QCOMPARE(handled, 5);
    ready();
    SessionChild held;
    QVERIFY(held.launch(server, clientDirectory, QStringLiteral("hold")));
    QTRY_COMPARE_WITH_TIMEOUT(handled, 6, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(held.canReadLine(), 3000);
    QCOMPARE(held.readLine().trimmed(), QByteArray("holding"));
    QVERIFY(server.revoke(pairId));
    QTRY_COMPARE_WITH_TIMEOUT(held.state(), QProcess::NotRunning, 3000);
    QCOMPARE(held.exitCode(), 0);
    QCOMPARE(server.pairings().size(), 0);
    ready();
    const int afterRevoke = authenticated;
    QVERIFY(run(QStringLiteral("revoked")));
    QCOMPARE(authenticated, afterRevoke);
    ready();
    QVERIFY(!server.reply(QString(32, QLatin1Char('x')), QString(32, QLatin1Char('a')), {}));
    server.disable();
    PairCredentialStore store(serverDirectory, "test-session-server");
    QList<PairCredential> records;
    PairStoreError error;
    QVERIFY(store.load(records, error));
    QVERIFY(records.isEmpty());
}

void McpIpcTest::sessionRejectedPairing_data()
{
    QTest::addColumn<QString>("mode");
    for (const char *mode : {"pair-claimed-approval", "pair-rejected", "pair-bad-confirm", "pair-no-confirm"})
        QTest::newRow(mode) << QString::fromLatin1(mode);
}

void McpIpcTest::sessionRejectedPairing()
{
    QFETCH(QString, mode);
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/sessions-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString directory = temporary.path() + QStringLiteral("/server");
    AutomationSessionServer server(directory, "test-session-server");
    int prompted = 0, authenticated = 0;
    connect(&server, &AutomationSessionServer::authenticated, this, [&]() { ++authenticated; });
    connect(&server, &AutomationSessionServer::pairingRequested, this, [&](const PairingRequest &request) {
        ++prompted;
        QVERIFY(!server.approvePairing(QString(32, QLatin1Char('x')), Read));
        QVERIFY(!server.approvePairing(request.requestId, Read | 128));
        if (mode == QStringLiteral("pair-rejected")) QVERIFY(server.rejectPairing(request.requestId));
        else QVERIFY(server.approvePairing(request.requestId, Read));
    });
    QVERIFY(server.enable());
    SessionChild child;
    QVERIFY(child.launch(server, temporary.path() + QStringLiteral("/client"), mode));
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 5000);
    QCOMPARE(child.exitCode(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(server.acceptingConnections(), 3000);
    QCOMPARE(authenticated, 0);
    QCOMPARE(prompted, mode == QStringLiteral("pair-claimed-approval") ? 0 : 1);
    QVERIFY(server.pairings().isEmpty());
    QVERIFY(!QFileInfo::exists(QDir(directory).filePath(QStringLiteral("pairings.dat"))));
}

void McpIpcTest::sessionStaleApproval()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/sessions-XXXXXX"));
    QVERIFY(temporary.isValid());
    AutomationSessionServer server(temporary.path() + QStringLiteral("/server"), "test-session-server");
    PairingRequest old;
    auto capture = connect(&server, &AutomationSessionServer::pairingRequested, this,
        [&](const PairingRequest &request) { old = request; });
    QVERIFY(server.enable());
    SessionChild first;
    QVERIFY(first.launch(server, temporary.path() + QStringLiteral("/client"), QStringLiteral("pair")));
    QTRY_VERIFY_WITH_TIMEOUT(!old.requestId.isEmpty(), 3000);
    first.kill(); QVERIFY(first.waitForFinished(3000));
    QTRY_VERIFY_WITH_TIMEOUT(server.acceptingConnections(), 3000);
    QVERIFY(!server.approvePairing(old.requestId, Read));
    disconnect(capture);
    QTest::qWait(1000);
    connect(&server, &AutomationSessionServer::pairingRequested, this, [&](const PairingRequest &request) {
        QVERIFY(request.connectionId != old.connectionId);
        QVERIFY(!server.approvePairing(old.requestId, Read));
        QVERIFY(server.approvePairing(request.requestId, Read));
    });
    server.setRequestHandler([&](AutomationRequest request) {
        QVERIFY(server.reply(request.connectionId, request.requestId, {{QStringLiteral("fixture"), true}}));
    });
    SessionChild next;
    QVERIFY(next.launch(server, temporary.path() + QStringLiteral("/client"), QStringLiteral("pair")));
    QTRY_COMPARE_WITH_TIMEOUT(next.state(), QProcess::NotRunning, 5000);
    QCOMPARE(next.exitCode(), 0);
    QCOMPARE(server.pairings().size(), 1);
}

void McpIpcTest::sessionStoreFailure()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/sessions-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString directory = temporary.path() + QStringLiteral("/server");
    AutomationSessionServer server(directory, "test-session-server");
    int authenticated = 0, failed = 0;
    connect(&server, &AutomationSessionServer::authenticated, this, [&]() { ++authenticated; });
    connect(&server, &AutomationSessionServer::failed, this, [&]() { ++failed; });
    connect(&server, &AutomationSessionServer::pairingRequested, this, [&](const PairingRequest &request) {
        // Make the destination unsafe after enable's empty read, before confirmation/save.
        QVERIFY(QDir().mkdir(directory));
        QVERIFY(server.approvePairing(request.requestId, Read));
    });
    QVERIFY(server.enable());
    SessionChild child;
    QVERIFY(child.launch(server, temporary.path() + QStringLiteral("/client"), QStringLiteral("pair-store-failure")));
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 5000);
    QCOMPARE(child.exitCode(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(failed, 1, 3000);
    QCOMPARE(authenticated, 0);
    QVERIFY(server.pairings().isEmpty());
    QVERIFY(!QFileInfo::exists(QDir(directory).filePath(QStringLiteral("pairings.dat"))));
}

void McpIpcTest::sessionSlowFrame()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/sessions-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString clientDirectory = temporary.path() + QStringLiteral("/client");
    AutomationSessionServer server(temporary.path() + QStringLiteral("/server"), "test-session-server");
    connect(&server, &AutomationSessionServer::pairingRequested, this, [&](const PairingRequest &request) {
        QVERIFY(server.approvePairing(request.requestId, Read));
    });
    int handled = 0;
    server.setRequestHandler([&](AutomationRequest request) {
        ++handled;
        QVERIFY(server.reply(request.connectionId, request.requestId, {{QStringLiteral("fixture"), true}}));
    });
    QVERIFY(server.enable());
    SessionChild pair;
    QVERIFY(pair.launch(server, clientDirectory, QStringLiteral("pair")));
    QTRY_COMPARE_WITH_TIMEOUT(pair.state(), QProcess::NotRunning, 5000);
    QCOMPARE(pair.exitCode(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(server.acceptingConnections(), 3000);
    SessionChild slow;
    QVERIFY(slow.launch(server, clientDirectory, QStringLiteral("slow-frame")));
    QTRY_COMPARE_WITH_TIMEOUT(slow.state(), QProcess::NotRunning, AuthorizedFrameTimeoutMs + 4000);
    QCOMPARE(slow.exitCode(), 0);
    QCOMPARE(handled, 1);
    QTRY_VERIFY_WITH_TIMEOUT(server.acceptingConnections(), 3000);
}

void McpIpcTest::sessionHandlerLifecycle()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/sessions-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QString directory = temporary.path() + QStringLiteral("/server");
    const QString clientDirectory = temporary.path() + QStringLiteral("/client");
    QPointer<AutomationSessionServer> server = new AutomationSessionServer(directory, "test-session-server");
    const auto cleanup = qScopeGuard([&]() { if (server) delete server.data(); });
    connect(server, &AutomationSessionServer::pairingRequested, this, [&](const PairingRequest &request) {
        QVERIFY(server->approvePairing(request.requestId, Read));
    });
    server->setRequestHandler([&](AutomationRequest request) {
        QVERIFY(server->reply(request.connectionId, request.requestId, {{QStringLiteral("fixture"), true}}));
    });
    QVERIFY(server->enable());
    SessionChild pair;
    QVERIFY(pair.launch(*server, clientDirectory, QStringLiteral("pair")));
    QTRY_COMPARE_WITH_TIMEOUT(pair.state(), QProcess::NotRunning, 5000);
    QCOMPARE(pair.exitCode(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(server->acceptingConnections(), 3000);
    server->setRequestHandler([&](AutomationRequest) { server->disable(); });
    SessionChild disabled;
    QVERIFY(disabled.launch(*server, clientDirectory, QStringLiteral("disabled-in-handler")));
    QTRY_COMPARE_WITH_TIMEOUT(disabled.state(), QProcess::NotRunning, 5000);
    QCOMPARE(disabled.exitCode(), 0);
    QVERIFY(!server->isEnabled());
    QVERIFY(server->enable());
    server->setRequestHandler([&](AutomationRequest) { server->deleteLater(); });
    SessionChild deleted;
    QVERIFY(deleted.launch(*server, clientDirectory, QStringLiteral("disabled-in-handler")));
    QTRY_COMPARE_WITH_TIMEOUT(deleted.state(), QProcess::NotRunning, 5000);
    QCOMPARE(deleted.exitCode(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(server.isNull(), 3000);
}

#ifdef QKM_AUTOMATION_TESTING
void McpIpcTest::storeMalformedProtectedData()
{
    QTemporaryDir temporary(QDir::currentPath() + QStringLiteral("/pairs-XXXXXX"));
    QVERIFY(temporary.isValid());
    const QByteArray binding("test-application");
    PairCredentialStore store(temporary.path() + QStringLiteral("/private"), binding);
    PairStoreError error;
    const PairCredential credential = storeCredential();
    QVERIFY(store.save({credential}, error));
    const QJsonObject record{{QStringLiteral("pair_id"), credential.pairId},
        {QStringLiteral("client_name"), credential.clientName},
        {QStringLiteral("epoch"), QString::number(credential.permissionEpoch)},
        {QStringLiteral("scopes"), int(credential.scopes)},
        {QStringLiteral("secret"), QString::fromLatin1(credential.secret.toHex())}};
    const auto writeRoot = [&](const QJsonObject &root, bool badChecksum = false) {
        FrameError frameError;
        const QByteArray frame = encodeFrame(root, 32768, frameError);
        QByteArray plain("QKM-PAIR-STORE-1", sizeof("QKM-PAIR-STORE-1"));
        plain.append(badChecksum ? QByteArray(32, 'x') : QCryptographicHash::hash(frame, QCryptographicHash::Sha256));
        plain.append(frame);
        const QByteArray encrypted = protectPairStoreTestPlain(plain, binding);
        return frameError == FrameError::None && !encrypted.isEmpty() && replaceTestFile(store.filePath(), encrypted);
    };
    const QJsonObject root{{QStringLiteral("format"), 1}, {QStringLiteral("pairs"), QJsonArray{record}}};
    QVERIFY(writeRoot(root));
    QList<PairCredential> records;
    QVERIFY(store.load(records, error));
    QCOMPARE(records.size(), 1);
    QList<QJsonObject> badRecords;
    QJsonObject changed = record; changed.insert(QStringLiteral("epoch"), 1); badRecords.append(changed);
    changed = record; changed.insert(QStringLiteral("epoch"), QStringLiteral("01")); badRecords.append(changed);
    changed = record; changed.insert(QStringLiteral("epoch"), QStringLiteral("18446744073709551616")); badRecords.append(changed);
    changed = record; changed.insert(QStringLiteral("scopes"), 128); badRecords.append(changed);
    changed = record; changed.insert(QStringLiteral("scopes"), 1.5); badRecords.append(changed);
    changed = record; changed.insert(QStringLiteral("secret"), QString(63, QLatin1Char('a'))); badRecords.append(changed);
    changed = record; changed.insert(QStringLiteral("unknown"), true); badRecords.append(changed);
    QList<QJsonObject> invalid;
    for (const QJsonObject &item : badRecords)
        invalid.append({{QStringLiteral("format"), 1}, {QStringLiteral("pairs"), QJsonArray{item}}});
    invalid.append({{QStringLiteral("format"), 2}, {QStringLiteral("pairs"), QJsonArray{record}}});
    invalid.append({{QStringLiteral("format"), 1}, {QStringLiteral("pairs"), QJsonArray{record, record}}});
    for (const QJsonObject &value : invalid) {
        QVERIFY(writeRoot(value));
        records = {credential};
        QVERIFY(!store.load(records, error));
        QVERIFY(records.isEmpty());
        QCOMPARE(error, PairStoreError::InvalidData);
    }
    QVERIFY(writeRoot(root, true));
    QVERIFY(!store.load(records, error));
    QCOMPARE(error, PairStoreError::InvalidData);
}

void McpIpcTest::listenerVerifierStopAndRestart()
{
    HANDLE gate = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    QVERIFY(gate);
    const auto cleanup = qScopeGuard([gate]() {
        setRequestIdentityTestHooks(nullptr, false);
        SetEvent(gate);
        CloseHandle(gate);
    });
    setRequestIdentityTestHooks(gate, false);
    LocalPipeListener listener;
    int received = 0;
    connect(&listener, &LocalPipeListener::handshakeReceived, this, [&]() { ++received; });
    QVERIFY(listener.start());
    const QString oldInstance = listener.instanceId();
    ListenerChild child;
    QVERIFY(child.launch(listener, QStringLiteral("hello")));
    QTRY_COMPARE_WITH_TIMEOUT(requestIdentityTestWorkerCount(), 1, 1000);
    QCOMPARE(received, 0);
    QElapsedTimer elapsed;
    elapsed.start();
    listener.stop();
    QVERIFY(elapsed.elapsed() < 1000);
    QVERIFY(!listener.start());
    QCOMPARE(requestIdentityTestWorkerCount(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
    setRequestIdentityTestHooks(nullptr, false);
    QVERIFY(SetEvent(gate));
    QTRY_VERIFY_WITH_TIMEOUT(listener.start(), 3000);
    QVERIFY(listener.instanceId() != oldInstance);
    QCOMPARE(received, 0);
    ListenerChild next;
    QVERIFY(next.launch(listener, QStringLiteral("hello")));
    QTRY_COMPARE_WITH_TIMEOUT(received, 1, 3000);
    listener.closeClient();
    QTRY_COMPARE_WITH_TIMEOUT(next.state(), QProcess::NotRunning, 3000);
    QCOMPARE(next.exitCode(), 0);
    QCOMPARE(requestIdentityTestWorkerCount(), 0);
}

void McpIpcTest::listenerVerifierTimeout_data()
{
    QTest::addColumn<bool>("disconnect");
    QTest::newRow("connected-held") << false;
    QTest::newRow("disconnected-held") << true;
}

void McpIpcTest::listenerVerifierTimeout()
{
    QFETCH(bool, disconnect);
    HANDLE gate = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    QVERIFY(gate);
    const auto cleanup = qScopeGuard([gate]() {
        setRequestIdentityTestHooks(nullptr, false);
        SetEvent(gate);
        CloseHandle(gate);
    });
    setRequestIdentityTestHooks(gate, false);
    LocalPipeListener listener;
    int received = 0, failures = 0;
    connect(&listener, &LocalPipeListener::handshakeReceived, this, [&]() { ++received; });
    connect(&listener, &LocalPipeListener::failed, this, [&]() { ++failures; });
    QVERIFY(listener.start());
    ListenerChild child;
    QVERIFY(child.launch(listener, QStringLiteral("hello")));
    QTRY_COMPARE_WITH_TIMEOUT(requestIdentityTestWorkerCount(), 1, 1000);
    if (disconnect) {
        child.kill();
        QVERIFY(child.waitForFinished(3000));
    }
    QTRY_COMPARE_WITH_TIMEOUT(failures, 1, PreauthTimeoutMs + 1500);
    QVERIFY(!listener.isListening());
    QVERIFY(!listener.start());
    QCOMPARE(received, 0);
    QCOMPARE(requestIdentityTestWorkerCount(), 1);
    if (!disconnect) {
        QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
        QCOMPARE(child.exitCode(), 0);
    }
    setRequestIdentityTestHooks(nullptr, false);
    QVERIFY(SetEvent(gate));
    QTRY_VERIFY_WITH_TIMEOUT(listener.start(), 3000);
    QCOMPARE(received, 0);
    QCOMPARE(failures, 1);
    QCOMPARE(requestIdentityTestWorkerCount(), 0);
}

void McpIpcTest::listenerRestoreFailure()
{
    setRequestIdentityTestHooks(nullptr, true);
    const auto resetHooks = qScopeGuard([]() { setRequestIdentityTestHooks(nullptr, false); });
    LocalPipeListener listener;
    int received = 0, failures = 0;
    connect(&listener, &LocalPipeListener::handshakeReceived, this, [&]() { ++received; });
    connect(&listener, &LocalPipeListener::failed, this, [&]() { ++failures; });
    QVERIFY(listener.start());
    ListenerChild child;
    QVERIFY(child.launch(listener, QStringLiteral("hello")));
    QTRY_COMPARE_WITH_TIMEOUT(failures, 1, 3000);
    QVERIFY(!listener.isListening());
    QCOMPARE(received, 0);
    QCOMPARE(requestIdentityTestWorkerCount(), 0);
    HANDLE token = nullptr;
    QVERIFY(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token));
    QCOMPARE(GetLastError(), DWORD(ERROR_NO_TOKEN));
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 3000);
    QCOMPARE(child.exitCode(), 0);
    setRequestIdentityTestHooks(nullptr, false);
    QVERIFY(listener.start());
    ListenerChild next;
    QVERIFY(next.launch(listener, QStringLiteral("hello")));
    QTRY_COMPARE_WITH_TIMEOUT(received, 1, 3000);
    listener.closeClient();
    QTRY_COMPARE_WITH_TIMEOUT(next.state(), QProcess::NotRunning, 3000);
    QCOMPARE(next.exitCode(), 0);
}
#endif

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().value(1) == QStringLiteral("--pipe-client"))
        return runPipeClient(app.arguments());
    if (app.arguments().value(1) == QStringLiteral("--listener-client"))
        return runListenerClient(app.arguments());
    if (app.arguments().value(1) == QStringLiteral("--store-reader"))
        return runStoreReader(app.arguments());
    if (app.arguments().value(1) == QStringLiteral("--session-client"))
        return runSessionClient(app.arguments());
    McpIpcTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mcp_ipc.moc"
