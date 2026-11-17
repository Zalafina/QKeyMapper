#ifndef QKM_NATIVE_OUTPUT_PROBE_H
#define QKM_NATIVE_OUTPUT_PROBE_H
#include <Windows.h>
#include <IbInputSimulator/InputSimulator.hpp>
#ifdef FAKERINPUT_SUPPORT
#include <fakerinputclient.h>
#endif
#include <QJsonArray>
#include <QAtomicInt>
#include <QJsonObject>
#include <QList>

// Only the opt-in maintenance build replaces these final native boundaries.
namespace QkmNativeOutputProbe {
void reset();
void breakAfterUp(QAtomicInt *flag, WORD virtualKey, int stopValue);
void breakAfterDown(QAtomicInt *flag, WORD virtualKey, int stopValue);
void failNextSendInputUp(WORD virtualKey);
int sendInputDownCount(WORD virtualKey);
int sendInputUpCount(WORD virtualKey);
QJsonObject counts();
QJsonArray exercise(bool demo);
#ifdef FAKERINPUT_SUPPORT
using FakerInputKeyboardObserver = void (*)(BYTE shiftKeyFlags, const BYTE keyCodes[KBD_KEY_CODES]);
using FakerInputMouseObserver = void (*)(BYTE button);
void resetFakerInput();
void setFakerInputObservers(FakerInputKeyboardObserver keyboardObserver, FakerInputMouseObserver mouseObserver);
QJsonObject fakerInputCounts();
BYTE lastFakerInputShiftFlags();
QList<BYTE> lastFakerInputKeyCodes();
BYTE lastFakerInputMouseButtons();
#endif
}
Send::Error __stdcall qkmProbeIbSendInit(Send::SendType, Send::InitFlags, void *);
void __stdcall qkmProbeIbSendDestroy();
UINT WINAPI qkmProbeIbSendInput(UINT, LPINPUT, int);
UINT WINAPI qkmProbeSendInput(UINT, LPINPUT, int);
BOOL WINAPI qkmProbeSetCursorPos(int, int);
void qkmProbeWorkerIbOutput(int scenario, quint64 generation);
#ifdef FAKERINPUT_SUPPORT
pfakerinput_client qkmProbeFakerInputAlloc();
void qkmProbeFakerInputFree(pfakerinput_client clientHandle);
bool qkmProbeFakerInputConnect(pfakerinput_client clientHandle);
void qkmProbeFakerInputDisconnect(pfakerinput_client clientHandle);
bool qkmProbeFakerInputUpdateKeyboard(pfakerinput_client clientHandle, BYTE shiftKeyFlags, BYTE keyCodes[KBD_KEY_CODES]);
bool qkmProbeFakerInputUpdateRelativeMouse(pfakerinput_client clientHandle, BYTE button,
    SHORT x, SHORT y, BYTE wheelPosition, BYTE hWheelPosition);
bool qkmProbeFakerInputUpdateAbsoluteMouse(pfakerinput_client clientHandle, BYTE button,
    USHORT x, USHORT y, BYTE wheelPosition, BYTE hWheelPosition);
#endif

#ifndef QKM_OUTPUT_PROBE_IMPLEMENTATION
#define IbSendInit(...) qkmProbeIbSendInit(__VA_ARGS__)
#define IbSendDestroy(...) qkmProbeIbSendDestroy(__VA_ARGS__)
#define IbSendInput(...) qkmProbeIbSendInput(__VA_ARGS__)
#define SendInput(...) qkmProbeSendInput(__VA_ARGS__)
#define SetCursorPos(...) qkmProbeSetCursorPos(__VA_ARGS__)
#ifdef FAKERINPUT_SUPPORT
#define fakerinput_alloc(...) qkmProbeFakerInputAlloc(__VA_ARGS__)
#define fakerinput_free(...) qkmProbeFakerInputFree(__VA_ARGS__)
#define fakerinput_connect(...) qkmProbeFakerInputConnect(__VA_ARGS__)
#define fakerinput_disconnect(...) qkmProbeFakerInputDisconnect(__VA_ARGS__)
#define fakerinput_update_keyboard(...) qkmProbeFakerInputUpdateKeyboard(__VA_ARGS__)
#define fakerinput_update_relative_mouse(...) qkmProbeFakerInputUpdateRelativeMouse(__VA_ARGS__)
#define fakerinput_update_absolute_mouse(...) qkmProbeFakerInputUpdateAbsoluteMouse(__VA_ARGS__)
#endif
#endif
#endif
