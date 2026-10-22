#include "qkm_ipc_frame.h"

#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtEndian>
#include <cmath>

namespace QkmAutomation {
namespace {

bool validUnicode(const QString &text)
{
    const QByteArray utf8 = text.toUtf8();
    // The explicit length preserves embedded NUL on Qt5.
    return QString::fromUtf8(utf8.constData(), utf8.size()) == text;
}

FrameError validateValue(const QJsonValue &value, int depth)
{
    if (value.isDouble() && !std::isfinite(value.toDouble())) return FrameError::InvalidJson;
    if (value.isString()) {
        const QString text = value.toString();
        return validUnicode(text) ? FrameError::None : FrameError::InvalidUnicode;
    }
    if (!value.isArray() && !value.isObject()) return FrameError::None;
    if (depth > MaxJsonDepth) return FrameError::TooDeep;
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &child : array) {
            const FrameError error = validateValue(child, depth + 1);
            if (error != FrameError::None) return error;
        }
    } else {
        const QJsonObject object = value.toObject();
        for (auto it = object.begin(); it != object.end(); ++it) {
            const QString key = it.key();
            if (!validUnicode(key)) return FrameError::InvalidUnicode;
            const FrameError error = validateValue(it.value(), depth + 1);
            if (error != FrameError::None) return error;
        }
    }
    return FrameError::None;
}

bool validLimit(int value) { return value > 0 && value <= MaxFramePayload; }

} // namespace

QByteArray encodeFrame(const QJsonObject &message, int maximumPayload,
                       FrameError &error)
{
    error = FrameError::None;
    if (!validLimit(maximumPayload)) {
        error = FrameError::InvalidLimit;
        return {};
    }
    error = validateValue(message, 1);
    if (error != FrameError::None) return {};
    const QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    if (payload.size() > maximumPayload) {
        error = FrameError::TooLarge;
        return {};
    }
    QByteArray frame(4, '\0');
    qToBigEndian<quint32>(static_cast<quint32>(payload.size()), frame.data());
    frame.append(payload);
    return frame;
}

FrameReader::FrameReader(int maximumPayload) : m_maximumPayload(maximumPayload)
{
    if (!validLimit(maximumPayload)) m_error = FrameError::InvalidLimit;
}

FrameReadResult FrameReader::fail(FrameError error)
{
    m_error = error;
    m_payload.clear();
    return FrameReadResult::Failed;
}

FrameReadResult FrameReader::read(QIODevice &device, QJsonObject &message)
{
    message = {};
    if (m_error != FrameError::None) return FrameReadResult::Failed;
    if (m_closed) return fail(FrameError::Closed);
    while (m_headerBytes < 4) {
        const qint64 count = device.read(m_header + m_headerBytes, 4 - m_headerBytes);
        if (count < 0) return fail(FrameError::IoError);
        if (!count) return FrameReadResult::Incomplete;
        m_headerBytes += static_cast<int>(count);
    }
    if (m_payload.isEmpty()) {
        const quint32 length = qFromBigEndian<quint32>(m_header);
        if (!length) return fail(FrameError::InvalidLength);
        if (length > static_cast<quint32>(m_maximumPayload)) return fail(FrameError::TooLarge);
        m_payload.resize(static_cast<int>(length));
    }
    while (m_payloadBytes < m_payload.size()) {
        const qint64 count = device.read(m_payload.data() + m_payloadBytes,
                                        m_payload.size() - m_payloadBytes);
        if (count < 0) return fail(FrameError::IoError);
        if (!count) return FrameReadResult::Incomplete;
        m_payloadBytes += static_cast<int>(count);
    }
    if (m_payload.startsWith("\xef\xbb\xbf")
        || QString::fromUtf8(m_payload.constData(), m_payload.size()).toUtf8() != m_payload)
        return fail(FrameError::InvalidUtf8);
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(m_payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return fail(FrameError::InvalidJson);
    const FrameError valueError = validateValue(document.object(), 1);
    if (valueError != FrameError::None) return fail(valueError);
    message = document.object();
    m_headerBytes = 0;
    m_payloadBytes = 0;
    m_payload.clear();
    return FrameReadResult::Message;
}

bool FrameReader::finish()
{
    m_closed = true;
    if (m_error != FrameError::None) return false;
    if (m_headerBytes || m_payloadBytes) {
        fail(FrameError::Truncated);
        return false;
    }
    return true;
}

} // namespace QkmAutomation
