#ifndef QKM_INPUT_OWNER_FENCE_H
#define QKM_INPUT_OWNER_FENCE_H

#include <QObject>

// Observes owner queue boundaries only; admission, task pools and GUI cleanup are separate.
class InputOwnerFence final : public QObject {
    Q_OBJECT
public:
    enum Owner { Worker = 1, Hook = 2, Interception = 4, All = Worker | Hook | Interception };
    explicit InputOwnerFence(QObject *parent = nullptr);
    quint64 request();
    void cancel() { m_pending = false; }
    bool pending() const { return m_pending; }
    int observedOwners() const { return m_observedOwners; }

signals:
    void requestFence_Signal(quint64 generation);
    void ownerObserved(quint64 generation, int owners);
    void reached(quint64 generation);

private:
    void observe(quint64 generation, Owner owner);
    quint64 m_generation = 0;
    int m_observedOwners = 0;
    bool m_pending = false;
};

#endif
