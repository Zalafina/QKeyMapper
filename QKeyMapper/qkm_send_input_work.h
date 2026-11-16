#ifndef QKM_SEND_INPUT_WORK_H
#define QKM_SEND_INPUT_WORK_H

#include <QAtomicInteger>
#include <QMetaType>
#include <QSharedPointer>

struct KeySequenceContext;

// Includes lifecycle/send queues and runnable destruction; zero alone is not a stop fence.
class SendInputWorkItem final {
public:
    explicit SendInputWorkItem(const QSharedPointer<KeySequenceContext> &context = {}) : sequence(context) { s_outstanding.fetchAndAddRelaxed(1); }
    ~SendInputWorkItem() { s_outstanding.fetchAndSubRelease(1); }
    QSharedPointer<KeySequenceContext> sequence;
    static quint64 outstanding() { return s_outstanding.loadAcquire(); }

private:
    Q_DISABLE_COPY(SendInputWorkItem)
    inline static QAtomicInteger<quint64> s_outstanding{0};
};

Q_DECLARE_METATYPE(QSharedPointer<SendInputWorkItem>)

#endif
