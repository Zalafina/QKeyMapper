#ifndef QKM_MAPPING_SOURCE_ACCESS_H
#define QKM_MAPPING_SOURCE_ACCESS_H

#include <QAtomicInteger>
#include <QMutex>
#include <limits>

class MappingSourceAccess {
public:
    QMutex &mutex() { return m_mutex; }
    bool runtimeObserved() const { return runtimeRevision() != 0; }
    quint64 runtimeRevision() const { return m_runtimeRevision.loadAcquire(); }

    void observeRuntimeLifecycle()
    {
        QMutexLocker locker(&m_mutex);
        const quint64 previous = m_runtimeRevision.loadRelaxed();
        // Never reuse a revision if the counter is exhausted.
        if (previous != (std::numeric_limits<quint64>::max)()) {
            m_runtimeRevision.storeRelease(previous + 1);
        }
    }

private:
    QMutex m_mutex;
    QAtomicInteger<quint64> m_runtimeRevision{0};
};

#endif
