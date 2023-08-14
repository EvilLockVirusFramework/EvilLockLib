#define _USE_MATH_DEFINES 1
#define M_PI   3.1415
#include <Windows.h>
#include <cmath>
#include <time.h>
#include <thread>
#include <random>
#pragma comment(lib, "winmm.lib")

typedef union _RGBQUAD
{
    COLORREF rgb;
    struct
    {
        BYTE r;
        BYTE g;
        BYTE b;
        BYTE Reserved;
    };
} _RGBQUAD, * PRGBQUAD;

extern "C"
{
   // __declspec(dllexport) COLORREF Hue(int length);
    __declspec(dllexport) void HuaPing1(int executionTimes, int interval);
    __declspec(dllexport) void HuaPing2(int executionTimes, int interval);
    __declspec(dllexport) void HuaPing3(int executionTimes, int interval);
    __declspec(dllexport) void MoveScreen(int executionTimes, int interval);
    __declspec(dllexport) void ScreenZoom(int executionTimes, int interval);
    __declspec(dllexport) void ScreenFreeze(int executionTimes, int interval);
    __declspec(dllexport) void RandomTextOut(const char* text, int fontSize, int executionTimes, int interval);
    __declspec(dllexport) void ScreenShake(int executionTimes, int interval);
    __declspec(dllexport) void HuaPing4(int executionTimes, int interval);
    __declspec(dllexport) void ScreenFlicker(int executionTimes, int interval);
    __declspec(dllexport) void RandomBlocks(int executionTimes, int interval);
    __declspec(dllexport) void DrawIconOnMouse(int executionTimes, int interval);
    __declspec(dllexport) void ScreenChannel(int executionTimes, int interval);
    __declspec(dllexport) void RandIconOnScreen(int executionTimes, int interval);
    __declspec(dllexport) void ScreenRotation(int executionTimes, int interval, double angle);
    __declspec(dllexport)  int MessageBoxPos(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, int X, int Y);
    __declspec(dllexport) void CreateRandomWindows(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, int num);
}
