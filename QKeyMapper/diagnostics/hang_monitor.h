#ifndef QKM_HANG_MONITOR_H
#define QKM_HANG_MONITOR_H
#include "crash_protocol.h"

namespace QkmDiagnostics {
struct Observation {
    LONG phase = Starting;
    LONG progress = 0;
    LONG expected = 0;
    LONG counts[SlotCount] = {};
    DWORD threadIds[SlotCount] = {};
    DWORD timeoutSeconds = DefaultHangSeconds;
};
inline Observation observe(SharedState &state)
{
    Observation value;
    value.phase = readAtomic(state.phase);
    value.progress = readAtomic(state.progress);
    value.expected = readAtomic(state.expectedThreads);
    value.timeoutSeconds = static_cast<DWORD>(readAtomic(state.hangSeconds));
    for (unsigned i = 0; i < SlotCount; ++i) {
        value.counts[i] = readAtomic(state.pulses[i].count);
        value.threadIds[i] = static_cast<DWORD>(readAtomic(state.pulses[i].threadId));
    }
    return value;
}
class HangMonitor {
public:
    bool sample(const Observation &value, ULONGLONG now, bool paused, LONG &stalledMask)
    {
        stalledMask = 0;
        if (!initialized || paused || value.phase != previous.phase) {
            reset(value, now);
            return false;
        }
        if (value.progress != previous.progress) progressAt = now;
        for (unsigned i = 0; i < SlotCount; ++i) {
            if (value.counts[i] != previous.counts[i]) {
                lastBeat[i] = now;
                recoverySeen |= 1L << i;
            } else if (!(previous.expected & (1L << i))) lastBeat[i] = now;
        }
        if (value.phase == Running) {
            const DWORD seconds = value.timeoutSeconds >= 10 && value.timeoutSeconds <= 600
                ? value.timeoutSeconds : DefaultHangSeconds;
            for (unsigned i = 0; i < SlotCount; ++i)
                if ((value.expected & (1L << i)) && now - lastBeat[i] >= seconds * 1000ULL)
                    stalledMask |= 1L << i;
        } else if (now - progressAt >= (value.phase == Starting ? 120000ULL : 60000ULL)) {
            stalledMask = -1;
        }
        previous = value;
        if (stalledMask) {
            recovering = false;
            if (!latched && attempts < 3 && (!attempts || now - lastCapture >= 600000)) {
                latched = true;
                recoverySeen = 0;
                lastCapture = now;
                ++attempts;
                return true;
            }
        } else if (latched) {
            // A grace period alone is not evidence of recovery, especially with long thresholds.
            bool healthy = value.phase == Running && value.expected
                && (recoverySeen & value.expected) == value.expected;
            for (unsigned i = 0; i < SlotCount; ++i)
                if ((value.expected & (1L << i)) && now - lastBeat[i] > 3 * HeartbeatIntervalMs) healthy = false;
            if (!healthy) recovering = false;
            else {
                if (!recovering) { recovering = true; healthySince = now; }
                if (now - healthySince >= 60000) latched = false;
            }
        }
        return false;
    }
    void reset(const Observation &value, ULONGLONG now)
    {
        initialized = true;
        previous = value;
        progressAt = now;
        for (auto &time : lastBeat) time = now;
        recovering = false;
        recoverySeen = 0;
    }
    ULONGLONG lastHeartbeat(unsigned slot) const { return lastBeat[slot]; }
private:
    Observation previous;
    bool initialized = false, latched = false, recovering = false;
    unsigned attempts = 0;
    LONG recoverySeen = 0;
    ULONGLONG progressAt = 0, lastBeat[SlotCount] = {}, lastCapture = 0, healthySince = 0;
};
}
#endif
