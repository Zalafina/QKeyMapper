// Tests compile the fixed upstream implementation with recording WinAPI substitutes.
#define IB_INPUT_STATIC
#include <Windows.h>
#include <cfgmgr32.h>
#include <winternl.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <IbInputSimulator/common.hpp>

extern "C" NTSTATUS WINAPI QkmTestNtOpenDirectoryObject(PHANDLE handle, ACCESS_MASK, POBJECT_ATTRIBUTES);
extern "C" NTSTATUS WINAPI QkmTestNtQueryDirectoryObject(HANDLE, PVOID buffer, ULONG, BOOLEAN, BOOLEAN, PULONG, PULONG);
BOOL WINAPI QkmTestCloseHandle(HANDLE);
BOOL WINAPI QkmTestIo(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
HANDLE WINAPI QkmTestCreateFile(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
FILE *QkmTestOpen(const wchar_t *, const wchar_t *, int);
CONFIGRET WINAPI QkmTestLocate(PDEVINST,DEVINSTID_W,ULONG);
CONFIGRET WINAPI QkmTestChild(PDEVINST,DEVINST,ULONG);
CONFIGRET WINAPI QkmTestSibling(PDEVINST,DEVINST,ULONG);
CONFIGRET WINAPI QkmTestStatus(PULONG,PULONG,DEVINST,ULONG);
CONFIGRET WINAPI QkmTestProperty(DEVINST,ULONG,PULONG,PVOID,PULONG,ULONG);
#define NtOpenDirectoryObject QkmTestNtOpenDirectoryObject
#define NtQueryDirectoryObject QkmTestNtQueryDirectoryObject
#define CloseHandle QkmTestCloseHandle
#include <IbInputSimulator/SendTypes/base.hpp>
#undef NtOpenDirectoryObject
#undef NtQueryDirectoryObject
#define DeviceIoControl QkmTestIo
#define CreateFileW QkmTestCreateFile
#define _wfsopen QkmTestOpen
#define CM_Locate_DevNodeW QkmTestLocate
#define CM_Get_Child QkmTestChild
#define CM_Get_Sibling QkmTestSibling
#define CM_Get_DevNode_Status QkmTestStatus
#define CM_Get_DevNode_Registry_PropertyW QkmTestProperty
#include <IbInputSimulator/SendTypes/Logitech.hpp>
#include <IbInputSimulator/SendTypes/Razer.hpp>
#undef DeviceIoControl
#undef CreateFileW
#undef _wfsopen
#undef CloseHandle
#undef CM_Locate_DevNodeW
#undef CM_Get_Child
#undef CM_Get_Sibling
#undef CM_Get_DevNode_Status
#undef CM_Get_DevNode_Registry_PropertyW

namespace {
struct Call { DWORD code; std::vector<unsigned char> bytes; };
std::vector<Call> calls;
int failAt = 0;
DWORD failCode = 0;
int closes = 0;
bool acceleration = false;
bool razerDevice=false;
bool mouseExists=false,mouseStarted=true,matchingBus=true,queryFails=false,creationRace=false,mouseProblem=false;
DWORD activationFailure=ERROR_GEN_FAILURE;
struct Mouse8 { unsigned char buttons; signed char x, y, wheel; unsigned char unknown; };
struct Mouse16 { unsigned char buttons; short x, y; signed char wheel; unsigned char unknown; };
struct Keyboard { unsigned char modifiers, reserved; std::array<unsigned char,6> keys; };
static_assert(sizeof(Mouse8) == 5 && sizeof(Mouse16) == 8 && sizeof(Keyboard) == 8);
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class T> T report(const Call &call) {
    check(call.bytes.size() == sizeof(T), "report ABI mismatch");
    T result{};
    std::memcpy(&result, call.bytes.data(), sizeof result);
    return result;
}
void reset() { calls.clear(); failAt=0; failCode=0; closes=0; acceleration=false; razerDevice=false; mouseExists=false; mouseStarted=true; matchingBus=true; queryFails=false; creationRace=false; mouseProblem=false; activationFailure=ERROR_GEN_FAILURE; }
MOUSEINPUT mouse(DWORD flags, LONG x=0, LONG y=0, DWORD data=0) { return {x,y,data,flags,0,0}; }
KEYBDINPUT key(WORD vk, bool up=false) { return {vk,0,up ? KEYEVENTF_KEYUP : DWORD{0},0,0}; }
void mouseCase() {
    Send::Type::Internal::Logitech logi;
    check(logi.create() == Send::Error::Success, "fake create failed");
    check(logi.send_mouse_input(mouse(MOUSEEVENTF_LEFTDOWN)), "left down failed");
    calls.clear();
    check(logi.send_mouse_input(mouse(MOUSEEVENTF_MOVE,300,-260)), "split move failed");
    int x=0,y=0;
    for (const auto &call : calls) { const auto r=report<Mouse8>(call); check(r.buttons==1,"button lost during split move"); x+=r.x; y+=r.y; }
    check(x==300 && y==-260 && calls.size()==3,"split displacement changed");
    check(logi.send_mouse_input(mouse(MOUSEEVENTF_XDOWN,0,0,XBUTTON2)),"X2 down failed");
    check(report<Mouse8>(calls.back()).buttons==17,"multiple buttons lost");
    calls.clear();
    check(logi.send_mouse_input(mouse(MOUSEEVENTF_WHEEL,0,0,3*WHEEL_DELTA)),"wheel failed");
    check(calls.size()==3,"wheel magnitude lost");
    for (const auto &call:calls) { const auto r=report<Mouse8>(call); check(r.buttons==17 && r.wheel==1,"wheel changed button state"); }
    const auto count=calls.size();
    check(!logi.send_mouse_input(mouse(MOUSEEVENTF_HWHEEL,0,0,WHEEL_DELTA)),"unsupported horizontal wheel accepted");
    check(!logi.send_mouse_input(mouse(MOUSEEVENTF_WHEEL,0,0,1)),"fractional wheel accepted");
    check(calls.size()==count,"unsupported input reached I/O");
    check(logi.send_mouse_input(mouse(MOUSEEVENTF_LEFTUP)),"left up failed");
    check(report<Mouse8>(calls.back()).buttons==16,"left up released X2");
    check(logi.send_mouse_input(mouse(MOUSEEVENTF_XUP,0,0,XBUTTON2)),"X2 up failed");
    check(report<Mouse8>(calls.back()).buttons==0,"buttons remain held");
    logi.destroy();
    Send::Type::Internal::LogitechGHubNew ghub;
    check(ghub.create()==Send::Error::Success,"fake G HUB create failed");
    calls.clear();
    check(ghub.send_mouse_input(mouse(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_MOVE,40000,-40000)),"new report split failed");
    x=0; y=0;
    for (const auto &call:calls) { const auto r=report<Mouse16>(call); check(r.buttons==1,"G HUB split lost button"); x+=r.x; y+=r.y; }
    check(x==40000 && y==-40000 && calls.size()==2,"G HUB split displacement changed");
    struct Button { DWORD down,up,data; unsigned char bit; };
    const Button buttons[]={
        {MOUSEEVENTF_LEFTDOWN,MOUSEEVENTF_LEFTUP,0,1},
        {MOUSEEVENTF_RIGHTDOWN,MOUSEEVENTF_RIGHTUP,0,2},
        {MOUSEEVENTF_MIDDLEDOWN,MOUSEEVENTF_MIDDLEUP,0,4},
        {MOUSEEVENTF_XDOWN,MOUSEEVENTF_XUP,XBUTTON1,8},
        {MOUSEEVENTF_XDOWN,MOUSEEVENTF_XUP,XBUTTON2,16}
    };
    check(ghub.send_mouse_input(mouse(MOUSEEVENTF_LEFTUP)),"G HUB initial release failed");
    unsigned char held=0;
    for (const auto &button:buttons) {
        check(ghub.send_mouse_input(mouse(button.down,0,0,button.data)),"G HUB button DOWN failed");
        held|=button.bit;
        check(report<Mouse16>(calls.back()).buttons==held,"G HUB button bit/holding changed");
    }
    calls.clear();
    check(ghub.send_mouse_input(mouse(MOUSEEVENTF_MOVE,-5,7)),"G HUB held movement failed");
    check(ghub.send_mouse_input(mouse(MOUSEEVENTF_WHEEL,0,0,2*WHEEL_DELTA)),"G HUB positive wheel failed");
    check(ghub.send_mouse_input(mouse(MOUSEEVENTF_WHEEL,0,0,static_cast<DWORD>(-2*WHEEL_DELTA))),"G HUB negative wheel failed");
    check(calls.size()==5,"G HUB wheel unit count changed");
    check(report<Mouse16>(calls[0]).x==-5 && report<Mouse16>(calls[0]).y==7,"G HUB held displacement changed");
    for (size_t index=0;index<calls.size();++index) {
        const auto r=report<Mouse16>(calls[index]);
        check(r.buttons==31,"G HUB move/wheel lost held buttons");
        check(r.wheel==(index==0 ? 0 : index<3 ? 1 : -1),"G HUB signed wheel unit changed");
    }
    for (const auto &button:buttons) {
        check(ghub.send_mouse_input(mouse(button.up,0,0,button.data)),"G HUB button UP failed");
        held&=static_cast<unsigned char>(~button.bit);
        check(report<Mouse16>(calls.back()).buttons==held,"G HUB UP released another button");
    }
    ghub.destroy();
}
void keyboardCase() {
    Send::Type::Internal::Logitech logi;
    check(logi.create()==Send::Error::Success,"fake keyboard create failed");
    for (WORD vk='A';vk<='F';++vk) check(logi.send_keyboard_input(key(vk)),"six-key report rejected");
    check(logi.send_keyboard_input(key('A')),"duplicate down rejected");
    const auto r=report<Keyboard>(calls.back());
    check(std::count(r.keys.begin(),r.keys.end(),4)==1,"duplicate down consumed a slot");
    const auto count=calls.size();
    check(!logi.send_keyboard_input(key('G')),"seventh key falsely accepted");
    check(!logi.send_keyboard_input(key(0)),"invalid key falsely accepted");
    check(calls.size()==count,"invalid keyboard event reached I/O");
    check(logi.send_keyboard_input(key('A',true)),"key release failed");
    check(logi.send_keyboard_input(key('G')),"released slot not reusable");
    check(logi.send_keyboard_input(key(VK_RCONTROL)),"right modifier failed");
    check(report<Keyboard>(calls.back()).modifiers==16,"right modifier changed side");
    logi.destroy();
    Send::Type::Internal::LogitechGHubNew ghub;
    check(ghub.create()==Send::Error::Success,"fake standard keyboard create failed");
    const std::pair<WORD,unsigned char> standard[]={{VK_BACK,0x2a},{VK_TAB,0x2b},{VK_RETURN,0x28},
        {VK_SPACE,0x2c},{VK_LEFT,0x50},{VK_HOME,0x4a},{VK_NUMPAD1,0x59},{VK_DIVIDE,0x54},
        {VK_OEM_1,0x33},{VK_OEM_5,0x31},{VK_APPS,0x65},{VK_PAUSE,0x48}};
    for (const auto &entry:standard) {
        check(ghub.send_keyboard_input(key(entry.first)),"standard keyboard DOWN failed");
        const auto value=report<Keyboard>(calls.back());
        check(value.keys[0]==entry.second && std::count(value.keys.begin(),value.keys.end(),entry.second)==1,
            "standard VK/HID mapping differs from FakerInput");
        check(ghub.send_keyboard_input(key(entry.first,true)),"standard keyboard UP failed");
        check(report<Keyboard>(calls.back()).keys==std::array<unsigned char,6>{},"standard key remains in report");
    }
    auto keypadEnter=key(VK_RETURN); keypadEnter.dwFlags|=KEYEVENTF_EXTENDEDKEY;
    check(ghub.send_keyboard_input(keypadEnter) && report<Keyboard>(calls.back()).keys[0]==0x28,
        "VK-only Enter behavior differs from existing FakerInput");
    check(ghub.send_keyboard_input(key(VK_RETURN,true)),"Enter cleanup failed");
    const std::pair<WORD,unsigned char> modifiers[]={{VK_LCONTROL,1},{VK_LSHIFT,2},{VK_LMENU,4},{VK_LWIN,8},
        {VK_RCONTROL,16},{VK_RSHIFT,32},{VK_RMENU,64},{VK_RWIN,128}};
    for (WORD vk='A';vk<='F';++vk) check(ghub.send_keyboard_input(key(vk)),"ordinary slots rejected before modifiers");
    unsigned char mask=0;
    for (const auto &entry:modifiers) {
        mask|=entry.second;
        check(ghub.send_keyboard_input(key(entry.first)) && report<Keyboard>(calls.back()).modifiers==mask,
            "modifier bit or side changed");
    }
    check(!ghub.send_keyboard_input(key('G')),"modifier report admitted a seventh ordinary key");
    for (const auto &entry:modifiers) check(ghub.send_keyboard_input(key(entry.first,true)),"modifier UP failed");
    for (WORD vk='A';vk<='F';++vk) check(ghub.send_keyboard_input(key(vk,true)),"ordinary UP failed");
    check(report<Keyboard>(calls.back()).modifiers==0 && report<Keyboard>(calls.back()).keys==std::array<unsigned char,6>{},
        "standard keyboard report did not clear");
    ghub.destroy();
}
void partialCase() {
    Send::Type::Internal::Logitech logi;
    check(logi.create()==Send::Error::Success,"fake partial create failed");
    failAt=2;
    check(!logi.send_mouse_input(mouse(MOUSEEVENTF_MOVE,500)),"partial split failure hidden");
    check(calls.size()==2,"I/O continued after failure");
    calls.clear(); failAt=1;
    check(!logi.send_mouse_input(mouse(MOUSEEVENTF_MOVE,LONG_MIN,LONG_MAX)),"extreme displacement failure hidden");
    check(calls.size()==1,"extreme displacement continued after failure");
    logi.destroy();
    reset();
    Send::Type::Internal::LogitechGHubNew ghub;
    check(ghub.create()==Send::Error::Success,"fake G HUB partial create failed");
    check(ghub.send_mouse_input(mouse(MOUSEEVENTF_XDOWN,0,0,XBUTTON2)),"G HUB held X2 failed");
    calls.clear(); failAt=2;
    check(!ghub.send_mouse_input(mouse(MOUSEEVENTF_MOVE,100000)),"G HUB partial split failure hidden");
    check(calls.size()==2,"G HUB split continued after failure");
    for (const auto &call:calls) check(report<Mouse16>(call).buttons==16,"G HUB failed split lost held X2");
    failAt=0;
    check(ghub.send_mouse_input(mouse(MOUSEEVENTF_XUP,0,0,XBUTTON2)),"G HUB partial failure release failed");
    check(report<Mouse16>(calls.back()).buttons==0,"G HUB partial failure left X2 held");
    ghub.destroy();
}
void activationCase() {
    Send::Type::Internal::LogitechGHubNew ghub;
    failCode=0x2A2040;
    check(ghub.create()==Send::Error::DeviceCreateFailed,"activation failure hidden");
    const DWORD activationError=GetLastError();
    check(closes==2,"failed activation leaked fake device");
    check(activationError==ERROR_GEN_FAILURE,"activation error was overwritten by cleanup");
}
void existingMouseCase() {
    reset(); mouseExists=true; failCode=0x2A2040; activationFailure=ERROR_INVALID_PARAMETER;
    {
        Send::Type::Internal::LogitechGHubNew ghub;
        check(ghub.create()==Send::Error::Success,"already-started matching mouse was rejected");
        check(calls.empty(),"existing mouse caused another activation request");
        ghub.destroy();
    }
    for (int condition=0;condition<5;++condition) {
        reset(); mouseExists=condition!=0; failCode=0x2A2040; activationFailure=ERROR_INVALID_PARAMETER;
        if (condition==1) mouseStarted=false;
        if (condition==2) matchingBus=false;
        if (condition==3) queryFails=true;
        if (condition==4) mouseProblem=true;
        Send::Type::Internal::LogitechGHubNew ghub;
        check(ghub.create()==Send::Error::DeviceCreateFailed,"unproven mouse accepted error87");
        const DWORD error=GetLastError();
        check(calls.size()==1 && closes==2 && error==ERROR_INVALID_PARAMETER,"error87 failure or cleanup changed");
    }
    reset(); failCode=0x2A2040; activationFailure=ERROR_ACCESS_DENIED; creationRace=true;
    {
        Send::Type::Internal::LogitechGHubNew ghub;
        check(ghub.create()==Send::Error::DeviceCreateFailed,"real activation failure was ignored");
        check(GetLastError()==ERROR_ACCESS_DENIED,"real activation error changed");
    }
    reset(); failCode=0x2A2040; activationFailure=ERROR_INVALID_PARAMETER; creationRace=true;
    {
        Send::Type::Internal::LogitechGHubNew ghub;
        check(ghub.create()==Send::Error::Success && calls.size()==1,"proven creation race was rejected");
        ghub.destroy();
    }
}
void compensationCase() {
    Send::Type::Internal::Logitech logi;
    acceleration=true;
    check(logi.create()==Send::Error::Success,"fake acceleration create failed");
    failAt=1;
    check(!logi.send_mouse_input(mouse(MOUSEEVENTF_MOVE,20)),"compensation failure hidden");
    check(calls.size()==1,"I/O continued after compensation failure");
    logi.destroy();
}
void razerBasicCase() {
    razerDevice=true;
    Send::Type::Internal::Razer razer;
    check(razer.create()==Send::Error::Success && calls.empty(),"Razer fake connection sent input");
    using Rz=Send::Type::Internal::Razer::RzControl;
    const std::pair<WORD,USHORT> keys[]={{'A',0x1e},{'Z',0x2c},{'0',0x0b},{'9',0x0a},
        {VK_F1,0x3b},{VK_F12,0x58},{VK_F13,0x64},{VK_F24,0x76},{VK_RETURN,0x1c},{VK_TAB,0x0f}};
    for (const auto &[vk,scan]:keys) for (bool up:{false,true}) {
        check(razer.send_keyboard_input(key(vk,up)),"Razer keyboard report failed");
        const auto value=report<Rz>(calls.back());
        check(calls.back().code==0x88883020 && value.type==Rz::Type::Keyboard
            && value.ki.MakeCode==scan && value.ki.Flags==(up ? KEY_BREAK : KEY_MAKE),"Razer basic keyboard encoding differs");
    }
    const DWORD down[]={MOUSEEVENTF_LEFTDOWN,MOUSEEVENTF_RIGHTDOWN,MOUSEEVENTF_MIDDLEDOWN,MOUSEEVENTF_XDOWN,MOUSEEVENTF_XDOWN};
    const DWORD up[]={MOUSEEVENTF_LEFTUP,MOUSEEVENTF_RIGHTUP,MOUSEEVENTF_MIDDLEUP,MOUSEEVENTF_XUP,MOUSEEVENTF_XUP};
    const USHORT expected[]={MOUSE_LEFT_BUTTON_DOWN,MOUSE_RIGHT_BUTTON_DOWN,MOUSE_MIDDLE_BUTTON_DOWN,MOUSE_BUTTON_4_DOWN,MOUSE_BUTTON_5_DOWN};
    for (int i=0;i<5;++i) {
        const DWORD data=i==3 ? XBUTTON1 : i==4 ? XBUTTON2 : 0;
        check(razer.send_mouse_input(mouse(down[i],0,0,data)),"Razer button DOWN failed");
        check(report<Rz>(calls.back()).mi.ButtonFlags==expected[i],"Razer button DOWN encoding differs");
        check(razer.send_mouse_input(mouse(up[i],0,0,data)),"Razer button UP failed");
        check(report<Rz>(calls.back()).mi.ButtonFlags==static_cast<USHORT>(expected[i]<<1),"Razer button UP encoding differs");
    }
    for (DWORD axis:{DWORD(MOUSEEVENTF_WHEEL),DWORD(MOUSEEVENTF_HWHEEL)}) for (LONG delta:{LONG(WHEEL_DELTA),LONG(-WHEEL_DELTA)}) {
        check(razer.send_mouse_input(mouse(axis,0,0,static_cast<DWORD>(delta))),"Razer wheel report failed");
        const auto value=report<Rz>(calls.back());
        check(value.mi.ButtonFlags==(axis==MOUSEEVENTF_WHEEL ? MOUSE_WHEEL : MOUSE_HWHEEL)
            && static_cast<SHORT>(value.mi.ButtonData)==delta && value.mi.LastX==0 && value.mi.LastY==0,
            "Razer wheel axis/sign/unit changed");
    }
    check(razer.send_mouse_input(mouse(MOUSEEVENTF_MOVE,8,-12)),"Razer move failed");
    const auto move=report<Rz>(calls.back());
    check(move.mi.Flags==MOUSE_MOVE_RELATIVE && move.mi.LastX==8 && move.mi.LastY==-10
        && move.mi.ButtonFlags==0,"Razer relative move encoding changed");
    failAt=static_cast<int>(calls.size()+1);
    check(!razer.send_mouse_input(mouse(MOUSEEVENTF_LEFTUP)),"Razer IO failure was hidden");
    razer.destroy();
}
void razerExtendedCase() {
    razerDevice=true;
    Send::Type::Internal::Razer razer;
    check(razer.create()==Send::Error::Success,"Razer extended fake connection failed");
    using Rz=Send::Type::Internal::Razer::RzControl;
    const std::pair<WORD,USHORT> keys[]={{VK_RCONTROL,0x1d},{VK_RMENU,0x38},{VK_DIVIDE,0x35},{VK_APPS,0x5d}};
    for (const auto &[vk,scan]:keys) for (bool up:{false,true}) {
        check(razer.send_keyboard_input(key(vk,up)),"Razer extended send failed");
        const auto value=report<Rz>(calls.back());
        check(value.ki.MakeCode==scan && value.ki.Flags==(KEY_E0 | (up ? KEY_BREAK : KEY_MAKE)),
            "Razer right modifier/divide/menu lacks E0");
    }
    razer.destroy();
}
void razerPauseCase() {
    razerDevice=true;
    Send::Type::Internal::Razer razer;
    check(razer.create()==Send::Error::Success,"Razer Pause fake connection failed");
    using Rz=Send::Type::Internal::Razer::RzControl;
    for (bool up:{false,true}) {
        calls.clear();
        check(razer.send_keyboard_input(key(VK_PAUSE,up)) && calls.size()==2,"Razer Pause sequence is incomplete");
        const auto prefix=report<Rz>(calls.at(0)),tail=report<Rz>(calls.at(1));
        check(calls.at(0).code==0x88883020 && calls.at(1).code==0x88883020
            && prefix.type==Rz::Type::Keyboard && tail.type==Rz::Type::Keyboard
            && prefix.ki.MakeCode==0x1d && prefix.ki.Flags==(KEY_E1 | (up ? KEY_BREAK : KEY_MAKE))
            && tail.ki.MakeCode==0x45 && tail.ki.Flags==(up ? KEY_BREAK : KEY_MAKE),
            "Razer Pause differs from the observed FakerInput sequence");
        for (int failure:{1,2}) {
            calls.clear(); failAt=failure;
            check(!razer.send_keyboard_input(key(VK_PAUSE,up)) && calls.size()==static_cast<size_t>(failure),
                "Razer Pause failure was hidden or continued sending");
            failAt=0;
        }
    }
    razer.destroy();
}
}
extern "C" NTSTATUS WINAPI QkmTestNtOpenDirectoryObject(PHANDLE handle,ACCESS_MASK,POBJECT_ATTRIBUTES) { *handle=reinterpret_cast<HANDLE>(1); return 0; }
extern "C" NTSTATUS WINAPI QkmTestNtQueryDirectoryObject(HANDLE,PVOID buffer,ULONG,BOOLEAN,BOOLEAN,PULONG,PULONG) {
    static wchar_t logitechName[]=L"ROOT#SYSTEM#0001#{1abc05c0-c378-41b9-9cef-df1aba82b015}";
    static wchar_t razerName[]=L"RZCONTROL#VID_1532&PID_0306&MI_00#test#{e3be005d-d130-4910-88ff-09ae02f680e9}";
    wchar_t *name=razerDevice ? razerName : logitechName;
    auto *info=static_cast<OBJECT_DIRECTORY_INFORMATION *>(buffer);
    std::memset(info,0,2*sizeof(*info));
    info[0].Name.Buffer=name; info[0].Name.Length=static_cast<USHORT>(std::wcslen(name)*sizeof(wchar_t));
    return 0;
}
BOOL WINAPI QkmTestCloseHandle(HANDLE) { ++closes; SetLastError(ERROR_INVALID_HANDLE); return TRUE; }
HANDLE WINAPI QkmTestCreateFile(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE) { return reinterpret_cast<HANDLE>(2); }
BOOL WINAPI QkmTestIo(HANDLE,DWORD code,LPVOID data,DWORD size,LPVOID,DWORD,LPDWORD,LPOVERLAPPED) {
    const auto *bytes=static_cast<unsigned char *>(data);
    calls.push_back({code,{bytes,bytes+size}});
    if ((failAt && calls.size()==static_cast<size_t>(failAt)) || failCode==code) { SetLastError(activationFailure); return FALSE; }
    return TRUE;
}
FILE *QkmTestOpen(const wchar_t *,const wchar_t *,int) {
    if (!acceleration) return nullptr;
    FILE *file=nullptr;
    if (tmpfile_s(&file)!=0) return nullptr;
    std::fputs("{\"pointer\":{\"hasAcceleration\":true}}",file); std::rewind(file); return file;
}
CONFIGRET WINAPI QkmTestLocate(PDEVINST node,DEVINSTID_W,ULONG) {
    if (queryFails) return CR_FAILURE;
    *node=10; return CR_SUCCESS;
}
CONFIGRET WINAPI QkmTestChild(PDEVINST child,DEVINST parent,ULONG) {
    if (queryFails) return CR_FAILURE;
    if (parent==10 && (mouseExists || (creationRace && !calls.empty()))) { *child=11; return CR_SUCCESS; }
    if (parent==11) { *child=12; return CR_SUCCESS; }
    return CR_NO_SUCH_DEVNODE;
}
CONFIGRET WINAPI QkmTestSibling(PDEVINST,DEVINST,ULONG) { return CR_NO_SUCH_DEVNODE; }
CONFIGRET WINAPI QkmTestStatus(PULONG status,PULONG problem,DEVINST node,ULONG) {
    if (queryFails) return CR_FAILURE;
    *status=(node!=12 || mouseStarted) ? DN_STARTED : 0;
    if (node==12 && mouseProblem) *status|=DN_HAS_PROBLEM;
    *problem=42; return CR_SUCCESS;
}
CONFIGRET WINAPI QkmTestProperty(DEVINST node,ULONG property,PULONG type,PVOID buffer,PULONG bytes,ULONG) {
    if (queryFails) return CR_FAILURE;
    const wchar_t *value=L"";
    if (node==10 && property==CM_DRP_HARDWAREID) value=matchingBus ? LR"(root\LGHUBVirtualBus)" : LR"(root\otherbus)";
    if (node==11 && property==CM_DRP_CLASSGUID) value=L"{745a17a0-74d3-11d0-b6fe-00a0c90f57da}";
    if (node==12 && property==CM_DRP_CLASSGUID) value=L"{4d36e96f-e325-11ce-bfc1-08002be10318}";
    const ULONG length=static_cast<ULONG>((std::wcslen(value)+1)*sizeof(wchar_t));
    if (*bytes<length) { *bytes=length; return CR_BUFFER_SMALL; }
    std::memcpy(buffer,value,length); *bytes=length;
    *type=property==CM_DRP_HARDWAREID ? REG_MULTI_SZ : REG_SZ; return CR_SUCCESS;
}
int main(int argc,char **argv) {
    const std::string selected=argc>1 ? argv[1] : "all";
    try {
        const std::array<std::pair<const char *,void(*)()>,9> cases{{{"mouse",mouseCase},{"keyboard",keyboardCase},{"partial",partialCase},{"activation",activationCase},{"compensation",compensationCase},{"existing_mouse",existingMouseCase},{"razer_basic",razerBasicCase},{"razer_extended",razerExtendedCase},{"razer_pause",razerPauseCase}}};
        bool ran=false;
        for (const auto &test:cases) if (selected=="all" || selected==test.first) { reset(); test.second(); ran=true; std::cout<<"PASS simulated "<<test.first<<'\n'; }
        check(ran,"unknown case");
        std::cout<<"No real device initialization or input; WinAPI boundaries substituted.\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<"FAIL simulated "<<selected<<": "<<e.what()<<'\n'; return 2; }
}
