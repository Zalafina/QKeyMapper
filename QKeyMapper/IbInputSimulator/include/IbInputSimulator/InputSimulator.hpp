#pragma once
#include <Windows.h>
#include <stdint.h>

#ifdef IB_INPUT_STATIC
    #define DLLAPI extern "C"
#elif defined(IB_INPUT_DLLEXPORT)
    #define DLLAPI extern "C" __declspec(dllexport)
#else
    #define DLLAPI extern "C" __declspec(dllimport)
#endif

namespace Send {
    enum class Error : uint32_t {
        Success,
        InvalidArgument,
        LibraryNotFound,
        LibraryLoadFailed,
        LibraryError,
        DeviceCreateFailed,
        DeviceNotFound,
        DeviceOpenFailed
    };

    enum class SendType : uint32_t {
        AnyDriver = 0,
        SendInput = 1,
        Logitech = 2,
        LogitechGHubNew = 6,
        Razer = 3,
        DD = 4,
        MouClassInputInjection = 5
    };

    using InitFlags = const uint32_t;
    struct Init {
        using T = InitFlags;
    };

    enum class HookCode : uint32_t {
        Off,
        On,
        InitOnly,
        Destroy
    };
}

DLLAPI Send::Error __stdcall IbSendInit(Send::SendType type, Send::InitFlags flags, void* argument);
DLLAPI void __stdcall IbSendDestroy();

DLLAPI void __stdcall IbSendSyncKeyStates();

#pragma region API 1

DLLAPI UINT WINAPI IbSendInput(
    _In_ UINT cInputs,                    // number of input in the array
    _In_reads_(cInputs) LPINPUT pInputs,  // array of inputs
    _In_ int cbSize                       // sizeof(INPUT)
);

DLLAPI void __stdcall IbSendInputHook(Send::HookCode code);

#pragma endregion


#pragma region API 2

DLLAPI VOID WINAPI IbSend_mouse_event(
    _In_ DWORD dwFlags,         //MOUSEEVENTF_
    _In_ DWORD dx,
    _In_ DWORD dy,
    _In_ DWORD dwData,
    _In_ ULONG_PTR dwExtraInfo
);

DLLAPI VOID WINAPI IbSend_keybd_event(
    _In_ BYTE bVk,
    _In_ BYTE bScan,
    _In_ DWORD dwFlags,         //KEYEVENTF_
    _In_ ULONG_PTR dwExtraInfo
);

#pragma endregion


#pragma region API 3

namespace Send {
    enum class MoveMode : uint32_t {
        Absolute,  //0
        Relative   //1
    };

    enum class MouseButton : uint32_t {
        LeftDown = MOUSEEVENTF_LEFTDOWN,              //0x02
        LeftUp = MOUSEEVENTF_LEFTUP,                  //0x04
        Left = LeftDown | LeftUp,                     //0x06

        RightDown = MOUSEEVENTF_RIGHTDOWN,            //0x08
        RightUp = MOUSEEVENTF_RIGHTUP,                //0x10
        Right = RightDown | RightUp,                  //0x18

        MiddleDown = MOUSEEVENTF_MIDDLEDOWN,          //0x20
        MiddleUp = MOUSEEVENTF_MIDDLEUP,              //0x40
        Middle = MiddleDown | MiddleUp,               //0x60

        XButton1Down = MOUSEEVENTF_XDOWN | XBUTTON1,  //0x81
        XButton1Up = MOUSEEVENTF_XUP | XBUTTON1,      //0x101
        XButton1 = XButton1Down | XButton1Up,         //0x181

        XButton2Down = MOUSEEVENTF_XDOWN | XBUTTON2,  //0x82
        XButton2Up = MOUSEEVENTF_XUP | XBUTTON2,      //0x102
        XButton2 = XButton2Down | XButton2Up,         //0x182
    };

    struct KeyboardModifiers {
        bool LCtrl : 1;   //0x01
        bool LShift : 1;  //0x02
        bool LAlt : 1;    //0x04
        bool LWin : 1;    //0x08
        bool RCtrl : 1;   //0x10
        bool RShift : 1;  //0x20
        bool RAlt : 1;    //0x40
        bool RWin : 1;    //0x80
    };
}

DLLAPI bool __stdcall IbSendMouseMove(uint32_t x, uint32_t y, Send::MoveMode mode);
DLLAPI bool __stdcall IbSendMouseClick(Send::MouseButton button);
DLLAPI bool __stdcall IbSendMouseWheel(int32_t movement);

DLLAPI bool __stdcall IbSendKeybdDown(uint16_t vk);
DLLAPI bool __stdcall IbSendKeybdUp(uint16_t vk);
DLLAPI bool __stdcall IbSendKeybdDownUp(uint16_t vk, Send::KeyboardModifiers modifiers);

#pragma endregion