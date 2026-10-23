#ifndef QKM_INPUT_PRODUCER_WORK_H
#define QKM_INPUT_PRODUCER_WORK_H

#include <QAtomicInteger>

// Counts nested synchronous input scopes, not queued work or completed stops.
class InputProducerWorkItem final {
public:
    InputProducerWorkItem() { s_outstanding.fetchAndAddRelaxed(1); }
    ~InputProducerWorkItem() { s_outstanding.fetchAndSubRelease(1); }
    static quint64 outstanding() { return s_outstanding.loadAcquire(); }

private:
    Q_DISABLE_COPY(InputProducerWorkItem)
    inline static QAtomicInteger<quint64> s_outstanding{0};
};

#endif
