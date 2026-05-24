/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include <format>
#include "ArmaConsoleChannel.h"

#ifdef _WIN32
#include <Richedit.h>
#include <CommDlg.h>
#include <cstring>
#include <detours.h>

namespace
{
    typedef void (WINAPI* SleepFunc)(DWORD);
    SleepFunc RealSleep = nullptr;

    void WINAPI HookedSleep(DWORD millis)
    {
        if (millis == 50)
            MsgWaitForMultipleObjects(0, nullptr, false, millis, QS_ALLINPUT);
        else
            RealSleep(millis);
    }

    struct SleepHook
    {
        SleepHook()
        {
            RealSleep = Sleep;
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID&)RealSleep, HookedSleep);
            DetourTransactionCommit();
        }
        ~SleepHook()
        {
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(&(PVOID&)RealSleep, HookedSleep);
            DetourTransactionCommit();
        }
    } gSleepHook;

    struct FindConsoleData { DWORD pid; HWND result; };

    BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM param)
    {
        auto* data = reinterpret_cast<FindConsoleData*>(param);
        DWORD pid = 0;
        GetWindowThreadProcessId(hWnd, &pid);
        if (pid != data->pid) return TRUE;

        char cls[256] = {};
        GetClassNameA(hWnd, cls, sizeof(cls) - 1);
        if (!_stricmp(cls, "#32770"))
        {
            data->result = hWnd;
            return FALSE;
        }
        return TRUE;
    }

    BOOL CALLBACK enumChildProc(HWND hWnd, LPARAM param)
    {
        char cls[256] = {};
        GetClassNameA(hWnd, cls, sizeof(cls) - 1);
        if (!_stricmp(cls, "RichEdit20A"))
        {
            *reinterpret_cast<HWND*>(param) = hWnd;
            return FALSE;
        }
        return TRUE;
    }
}

HWND ArmaConsoleChannel::findRichEdit()
{
    FindConsoleData data{ GetProcessId(GetCurrentProcess()), nullptr };
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));
    if (!data.result) return nullptr;

    HWND richEdit = nullptr;
    EnumChildWindows(data.result, enumChildProc, reinterpret_cast<LPARAM>(&richEdit));
    return richEdit;
}

ArmaConsoleChannel::ArmaConsoleChannel() : _wndRich(nullptr) {}

void ArmaConsoleChannel::log(Log::Level level, std::string_view msg)
{
    if (!_wndRich)
        _wndRich = findRichEdit();
    if (!_wndRich) return;

    std::string text = std::string(msg) + "\r\n";
    if (!text.empty() && text[0] == '0')
        text = " " + text.substr(1);

    static const size_t MAX_LINES = 500;
    size_t lineCount = SendMessageW(_wndRich, EM_GETLINECOUNT, 0, 0);
    int lastPoint = -1;
    while (lineCount > MAX_LINES)
    {
        FINDTEXTEXA fi{};
        fi.chrg.cpMin = lastPoint + 1;
        fi.chrg.cpMax = -1;
        fi.lpstrText = "\r";
        fi.chrgText.cpMin = fi.chrgText.cpMax = -1;

        if (SendMessageW(_wndRich, EM_FINDTEXTEX, FR_DOWN | FR_MATCHCASE,
            reinterpret_cast<LPARAM>(&fi)) == -1)
            break;

        lastPoint = fi.chrgText.cpMax;
        lineCount--;
    }

    SendMessage(_wndRich, WM_SETREDRAW, FALSE, 0);

    if (lastPoint > 0)
    {
        SendMessageW(_wndRich, EM_SETSEL, 0, lastPoint);
        SendMessageW(_wndRich, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L""));
    }

    SendMessageW(_wndRich, EM_SETSEL, -1, -1);

    COLORREF color = RGB(0x33, 0x99, 0x33);
    if (level == Log::Level::Trace || level == Log::Level::Debug)
        color = RGB(0xBB, 0xBB, 0xBB);
    else if (level == Log::Level::Warning)
        color = RGB(0xFF, 0x99, 0x33);
    else if (level >= Log::Level::Error)
        color = RGB(0xFF, 0x00, 0x00);

    CHARFORMATW cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    if (level >= Log::Level::Error)
    {
        cf.dwMask |= CFM_BOLD;
        cf.dwEffects = CFE_BOLD;
    }
    cf.crTextColor = color;
    SendMessageW(_wndRich, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));

    SETTEXTEX st{ ST_SELECTION, 65001 };
    SendMessageW(_wndRich, EM_SETTEXTEX,
        reinterpret_cast<WPARAM>(&st),
        reinterpret_cast<LPARAM>(text.c_str()));

    SendMessage(_wndRich, WM_SETREDRAW, TRUE, 0);
    SendMessageW(_wndRich, EM_SETSEL, -1, -1);
    SendMessageW(_wndRich, EM_SCROLLCARET, 0, 0);
    RedrawWindow(_wndRich, nullptr, nullptr,
        RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

#else
#include <iostream>
ArmaConsoleChannel::ArmaConsoleChannel() {}
void ArmaConsoleChannel::log(Log::Level level, std::string_view msg)
{
    (level >= Log::Level::Error ? std::cerr : std::cout) << msg << "\n";
}
#endif