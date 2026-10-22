#include "../../automation/qkm_ipc_frame.h"
#include "../../automation/qkm_local_identity.h"
#include "../../automation/qkm_local_listener.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QProcess>
#include <QScopeGuard>
#include <QTest>
#include <QTimer>
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

bool receiveMessage(QLocalSocket &socket, FrameReader &reader, QJsonObject &message)
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
        timer.start(3000);
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
#ifdef QKM_AUTOMATION_TESTING
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

#ifdef QKM_AUTOMATION_TESTING
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
    McpIpcTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mcp_ipc.moc"
