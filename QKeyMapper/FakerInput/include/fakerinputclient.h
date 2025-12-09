#ifndef FAKERINPUTCLIENT_H
#define FAKERINPUTCLIENT_H

#include "fakerinputcommon.h"

#include <initguid.h>

typedef struct _fakerinput_client_t* pfakerinput_client;

#ifdef FAKERINPUTLIB_EXPORTS
#define FAKERINPUTLIB_API __declspec(dllexport)
#else
#define FAKERINPUTLIB_API __declspec(dllimport)
#endif

EXTERN_C_START

    FAKERINPUTLIB_API pfakerinput_client fakerinput_alloc();

    FAKERINPUTLIB_API void fakerinput_free(pfakerinput_client clientHandle);

    FAKERINPUTLIB_API bool fakerinput_connect(pfakerinput_client clientHandle);

    FAKERINPUTLIB_API void fakerinput_disconnect(pfakerinput_client clientHandle);

    FAKERINPUTLIB_API bool fakerinput_update_keyboard(pfakerinput_client clientHandle, BYTE shiftKeyFlags, BYTE keyCodes[KBD_KEY_CODES]);

    FAKERINPUTLIB_API bool fakerinput_update_keyboard_enhanced(pfakerinput_client clientHandle, BYTE multiKeys, BYTE extraKeys);

    FAKERINPUTLIB_API bool fakerinput_update_absolute_mouse(pfakerinput_client clientHandle, BYTE button, USHORT x, USHORT y,
        BYTE wheelPosition, BYTE hWheelPosition);

    FAKERINPUTLIB_API bool fakerinput_update_relative_mouse(pfakerinput_client clientHandle, BYTE button,
        SHORT x, SHORT y, BYTE wheelPosition, BYTE hWheelPosition);

    FAKERINPUTLIB_API UINT32 fakerinput_versionAPINumber(pfakerinput_client clientHandle);

EXTERN_C_END

//// {ab67b0fa-d0f5-4f60-81f4-346e18fd0805}
DEFINE_GUID(GUID_DEVINTERFACE_FakerInput,
    0xab67b0fa, 0xd0f5, 0x4f60, 0x81, 0xf4, 0x34, 0x6e, 0x18, 0xfd, 0x08, 0x05);


#endif // FAKERINPUTCLIENT_H