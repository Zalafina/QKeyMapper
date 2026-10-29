// Tests compile the fixed upstream implementation with recording WinAPI substitutes.
#define IB_INPUT_STATIC
#include <Windows.h>
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
#define NtOpenDirectoryObject QkmTestNtOpenDirectoryObject
#define NtQueryDirectoryObject QkmTestNtQueryDirectoryObject
#define CloseHandle QkmTestCloseHandle
#include <IbInputSimulator/SendTypes/base.hpp>
#undef NtOpenDirectoryObject
#undef NtQueryDirectoryObject
#define DeviceIoControl QkmTestIo
#define CreateFileW QkmTestCreateFile
#define _wfsopen QkmTestOpen
#include <IbInputSimulator/SendTypes/Logitech.hpp>
#undef DeviceIoControl
#undef CreateFileW
#undef _wfsopen
#undef CloseHandle

namespace {
struct Call { DWORD code; std::vector<unsigned char> bytes; };
std::vector<Call> calls;
int failAt = 0;
DWORD failCode = 0;
int closes = 0;
bool acceleration = false;
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
void reset() { calls.clear(); failAt=0; failCode=0; closes=0; acceleration=false; }
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
}
void activationCase() {
    Send::Type::Internal::LogitechGHubNew ghub;
    failCode=0x2A2040;
    check(ghub.create()==Send::Error::DeviceCreateFailed,"activation failure hidden");
    check(closes==2,"failed activation leaked fake device");
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
}
extern "C" NTSTATUS WINAPI QkmTestNtOpenDirectoryObject(PHANDLE handle,ACCESS_MASK,POBJECT_ATTRIBUTES) { *handle=reinterpret_cast<HANDLE>(1); return 0; }
extern "C" NTSTATUS WINAPI QkmTestNtQueryDirectoryObject(HANDLE,PVOID buffer,ULONG,BOOLEAN,BOOLEAN,PULONG,PULONG) {
    static wchar_t name[]=L"ROOT#SYSTEM#0001#{1abc05c0-c378-41b9-9cef-df1aba82b015}";
    auto *info=static_cast<OBJECT_DIRECTORY_INFORMATION *>(buffer);
    std::memset(info,0,2*sizeof(*info));
    info[0].Name.Buffer=name; info[0].Name.Length=static_cast<USHORT>((std::size(name)-1)*sizeof(wchar_t));
    return 0;
}
BOOL WINAPI QkmTestCloseHandle(HANDLE) { ++closes; return TRUE; }
HANDLE WINAPI QkmTestCreateFile(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE) { return reinterpret_cast<HANDLE>(2); }
BOOL WINAPI QkmTestIo(HANDLE,DWORD code,LPVOID data,DWORD size,LPVOID,DWORD,LPDWORD,LPOVERLAPPED) {
    const auto *bytes=static_cast<unsigned char *>(data);
    calls.push_back({code,{bytes,bytes+size}});
    if ((failAt && calls.size()==static_cast<size_t>(failAt)) || failCode==code) { SetLastError(ERROR_GEN_FAILURE); return FALSE; }
    return TRUE;
}
FILE *QkmTestOpen(const wchar_t *,const wchar_t *,int) {
    if (!acceleration) return nullptr;
    FILE *file=nullptr;
    if (tmpfile_s(&file)!=0) return nullptr;
    std::fputs("{\"pointer\":{\"hasAcceleration\":true}}",file); std::rewind(file); return file;
}
int main(int argc,char **argv) {
    const std::string selected=argc>1 ? argv[1] : "all";
    try {
        const std::array<std::pair<const char *,void(*)()>,5> cases{{{"mouse",mouseCase},{"keyboard",keyboardCase},{"partial",partialCase},{"activation",activationCase},{"compensation",compensationCase}}};
        bool ran=false;
        for (const auto &test:cases) if (selected=="all" || selected==test.first) { reset(); test.second(); ran=true; std::cout<<"PASS simulated "<<test.first<<'\n'; }
        check(ran,"unknown case");
        std::cout<<"No real device initialization or input; WinAPI boundaries substituted.\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<"FAIL simulated "<<selected<<": "<<e.what()<<'\n'; return 2; }
}
