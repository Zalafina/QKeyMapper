#include "qkm_input_owner_fence.h"
#include "qkeymapper_worker.h"
#include "interception_worker.h"
#include <QAtomicInteger>
#include <QThread>
#include <limits>

namespace {
QAtomicInteger<quint64> lastFenceGeneration{0};
}

InputOwnerFence::InputOwnerFence(QObject *parent) : QObject(parent)
{
    auto *worker = QKeyMapper_Worker::getInstance();
    auto *hook = QKeyMapper_Hook_Proc::getInstance();
    auto *interception = Interception_Worker::getInstance();
    connect(this, &InputOwnerFence::requestFence_Signal, worker, &QKeyMapper_Worker::reachInputFence, Qt::QueuedConnection);
    connect(this, &InputOwnerFence::requestFence_Signal, hook, &QKeyMapper_Hook_Proc::reachInputFence, Qt::QueuedConnection);
    connect(this, &InputOwnerFence::requestFence_Signal, interception, &Interception_Worker::requestInputFence, Qt::QueuedConnection);
    connect(worker, &QKeyMapper_Worker::inputFenceReached_Signal, this,
        [this](quint64 generation) { observe(generation, Worker); }, Qt::QueuedConnection);
    connect(hook, &QKeyMapper_Hook_Proc::inputFenceReached_Signal, this,
        [this](quint64 generation) { observe(generation, Hook); }, Qt::QueuedConnection);
    connect(interception, &Interception_Worker::inputFenceReached_Signal, this,
        [this](quint64 generation) { observe(generation, Interception); }, Qt::QueuedConnection);
}

quint64 InputOwnerFence::request()
{
    Q_ASSERT(QThread::currentThread() == thread());
    quint64 previous = lastFenceGeneration.loadRelaxed();
    do {
        if (previous == std::numeric_limits<quint64>::max()) { return 0; }
    } while (!lastFenceGeneration.testAndSetRelaxed(previous, previous + 1, previous));
    m_generation = previous + 1;
    m_observedOwners = 0;
    m_pending = true;
    emit requestFence_Signal(m_generation);
    return m_generation;
}

void InputOwnerFence::observe(quint64 generation, Owner owner)
{
    if (!m_pending || generation != m_generation || (m_observedOwners & owner)) { return; }
    m_observedOwners |= owner;
    emit ownerObserved(generation, m_observedOwners);
    if (m_pending && generation == m_generation && m_observedOwners == All) {
        m_pending = false;
        emit reached(generation);
    }
}
