#ifndef QKM_MAPPING_SOURCE_ACCESS_H
#define QKM_MAPPING_SOURCE_ACCESS_H

#include <QAtomicInteger>
#include <QMutex>

class MappingSourceAccess {
public:
    QMutex &mutex() { return m_mutex; }
    bool runtimeObserved() const { return m_runtimeObserved.loadAcquire(); }

    void observeRuntimeLifecycle()
    {
        QMutexLocker locker(&m_mutex);
        m_runtimeObserved.storeRelease(true);
    }

private:
    QMutex m_mutex;
    QAtomicInteger<bool> m_runtimeObserved{false};
};

#endif
