#ifndef QKM_IBINPUT_CLIENT_H
#define QKM_IBINPUT_CLIENT_H

#include <Windows.h>
#include <QDateTime>
#include <QMutex>
#include <QSet>
#include <QStringList>
#include <atomic>

class QSettings;

class QkmIbInputClient
{
public:
    enum Capability { RelativeMove=1, Keyboard=2, Buttons=4, VerticalWheel=8, HorizontalWheel=16, AbsoluteMove=32 };
    struct Preferences { QString backend=QStringLiteral("auto"); QString unavailablePolicy=QStringLiteral("block"); };
    struct Requirements { unsigned capabilities=0; QSet<quint16> keys; QSet<quint16> mouseButtons; bool buttonStateOverlap=false; QString rejection; };
    struct Status {
        bool connected=false;
        bool stopping=false;
        bool fallback=false;
        bool releaseIncomplete=false;
        quint64 generation=0;
        QString backend;
        QString detail;
        QDateTime sampledAt;
    };
    static QkmIbInputClient &instance();
    static Preferences readPreferences(QSettings &settings, QString *diagnostic=nullptr);
    static void writePreferences(QSettings &settings, const Preferences &preferences);
    static QStringList candidates(const QString &preference);
    static bool isSupportedKey(quint16 key);
    static bool isSupportedButton(quint16 button);
    static unsigned capabilities(const QString &backend);
    Preferences preferences() const;
    bool setPreferences(const Preferences &preferences);
    Status statusSnapshot() const;
    quint64 requestRun();
    void cancelRun();
    quint64 requestedGeneration() const { return m_requested.load(); }
    bool acceptingInput() const { return !m_cancelled.load(); }
    bool prepareRun(quint64 generation, const Requirements &requirements);
    bool sendEvent(const INPUT &input, quint64 generation);
    bool releaseOwnedInputs(quint64 generation);
    bool closeAfterDrain(quint64 generation);
    bool checkConnection();
    bool busy() const;

private:
    void publish();
    bool fail(const QString &detail);
    bool sendOne(const INPUT &input);
    bool releaseLocked();
    bool candidateSupports(const QString &backend, const Requirements &requirements) const;
    mutable QMutex m_statusMutex;
    QMutex m_callMutex;
    Preferences m_preferences;
    Status m_status;
    Status m_published;
    unsigned m_runCapabilities=0;
    QSet<quint16> m_ownedKeys;
    QSet<quint16> m_ownedButtons;
    std::atomic<quint64> m_requested{0};
    std::atomic_bool m_cancelled{true};
};

#endif
