#ifndef QKM_SEND_INPUT_WORK_H
#define QKM_SEND_INPUT_WORK_H

#include <QAtomicInteger>
#include <QMetaType>
#include <QSharedPointer>

// Includes queued delivery and runnable destruction; zero alone is not a stop fence.
class SendInputWorkItem final {
public:
    SendInputWorkItem() { s_outstanding.fetchAndAddRelaxed(1); }
    ~SendInputWorkItem() { s_outstanding.fetchAndSubRelease(1); }
    static quint64 outstanding() { return s_outstanding.loadAcquire(); }

private:
    Q_DISABLE_COPY(SendInputWorkItem)
    inline static QAtomicInteger<quint64> s_outstanding{0};
};

Q_DECLARE_METATYPE(QSharedPointer<SendInputWorkItem>)

#endif
