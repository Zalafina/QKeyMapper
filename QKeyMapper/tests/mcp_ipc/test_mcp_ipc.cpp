#include "../../automation/qkm_ipc_frame.h"
#include "../../automation/qkm_local_identity.h"

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
        FrameReader small(limit);
        const QByteArray header = rawFrame(QByteArray(limit + 1, ' ')).left(4);
        QCOMPARE(feed(small, header, decoded), FrameReadResult::Failed);
        QCOMPARE(small.error(), FrameError::TooLarge);
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

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().value(1) == QStringLiteral("--pipe-client"))
        return runPipeClient(app.arguments());
    McpIpcTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mcp_ipc.moc"
