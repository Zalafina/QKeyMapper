#define IB_INPUT_STATIC
#include <Windows.h>
#include <bit>
#include <iostream>
#include <IbInputSimulator/InputSimulator.hpp>
static int sent=0;
DLLAPI UINT WINAPI IbSendInput(UINT count,LPINPUT,int) { sent+=static_cast<int>(count); return count; }
#include "API 3.cpp"
int main() {
    if (IbSendMouseMove(0,0,static_cast<Send::MoveMode>(99)) || sent!=0) {
        std::cerr<<"FAIL simulated: invalid move mode reached send\n"; return 2;
    }
    if (!IbSendMouseMove(10,0,Send::MoveMode::Relative) || sent!=1) return 2;
    std::cout<<"PASS simulated argument bounds; no device or input API\n"; return 0;
}
