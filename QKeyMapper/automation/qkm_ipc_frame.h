#ifndef QKEYMAPPER_AUTOMATION_IPC_FRAME_H
#define QKEYMAPPER_AUTOMATION_IPC_FRAME_H

#include <QByteArray>
#include <QJsonObject>

class QIODevice;

namespace QkmAutomation {

constexpr int MaxFramePayload = 256 * 1024;
constexpr int MaxHandshakePayload = 4 * 1024;
constexpr int MaxJsonDepth = 32;

enum class FrameError {
    None, InvalidLimit, InvalidLength, TooLarge, InvalidUtf8, InvalidJson,
    InvalidUnicode, TooDeep, IoError, Truncated, Closed
};

enum class FrameReadResult { Incomplete, Message, Failed };

QByteArray encodeFrame(const QJsonObject &message, int maximumPayload,
                       FrameError &error);

class FrameReader {
public:
    explicit FrameReader(int maximumPayload = MaxFramePayload);
    FrameReadResult read(QIODevice &device, QJsonObject &message);
    bool finish();
    FrameError error() const { return m_error; }

private:
    FrameReadResult fail(FrameError error);
    int m_maximumPayload;
    char m_header[4] = {};
    int m_headerBytes = 0;
    int m_payloadBytes = 0;
    QByteArray m_payload;
    FrameError m_error = FrameError::None;
    bool m_closed = false;
};

} // namespace QkmAutomation
#endif
