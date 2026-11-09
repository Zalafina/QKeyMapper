#ifndef QKM_NATIVE_OUTPUT_PROBE_H
#define QKM_NATIVE_OUTPUT_PROBE_H
#include <Windows.h>
#include <IbInputSimulator/InputSimulator.hpp>
#include <QJsonArray>
#include <QJsonObject>

// Only the opt-in maintenance build replaces these final native boundaries.
namespace QkmNativeOutputProbe {
void reset();
QJsonObject counts();
QJsonArray exercise(bool demo);
}
Send::Error __stdcall qkmProbeIbSendInit(Send::SendType, Send::InitFlags, void *);
void __stdcall qkmProbeIbSendDestroy();
UINT WINAPI qkmProbeIbSendInput(UINT, LPINPUT, int);
UINT WINAPI qkmProbeSendInput(UINT, LPINPUT, int);
BOOL WINAPI qkmProbeSetCursorPos(int, int);
void qkmProbeWorkerIbOutput(int scenario, quint64 generation);

#ifndef QKM_OUTPUT_PROBE_IMPLEMENTATION
#define IbSendInit(...) qkmProbeIbSendInit(__VA_ARGS__)
#define IbSendDestroy(...) qkmProbeIbSendDestroy(__VA_ARGS__)
#define IbSendInput(...) qkmProbeIbSendInput(__VA_ARGS__)
#define SendInput(...) qkmProbeSendInput(__VA_ARGS__)
#define SetCursorPos(...) qkmProbeSetCursorPos(__VA_ARGS__)
#endif
#endif
