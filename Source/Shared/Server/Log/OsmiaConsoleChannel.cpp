/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#include "OsmiaConsoleChannel.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <ios>

namespace
{
    // Convert UTF-8 string to UTF-16
    std::wstring toUTF16(std::string_view str)
    {
        if (str.empty()) return {};
        int size = MultiByteToWideChar(CP_UTF8, 0,
            str.data(), static_cast<int>(str.size()),
            nullptr, 0);
        std::wstring result(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0,
            str.data(), static_cast<int>(str.size()),
            result.data(), size);
        return result;
    }

    bool redirectIOToConsole()
    {
        if (!AllocConsole()) return false;

        // Set buffer size
        {
            static const WORD MAX_LINES = 500;
            CONSOLE_SCREEN_BUFFER_INFO info{};
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
            info.dwSize.Y = MAX_LINES;
            SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), info.dwSize);
        }

        // Redirect stdout
        {
            auto h = reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE));
            FILE* fp = _fdopen(_open_osfhandle(h, _O_TEXT), "w");
            *stdout = *fp;
            setvbuf(stdout, nullptr, _IONBF, 0);
        }

        // Redirect stdin
        {
            auto h = reinterpret_cast<intptr_t>(GetStdHandle(STD_INPUT_HANDLE));
            FILE* fp = _fdopen(_open_osfhandle(h, _O_TEXT), "r");
            *stdin = *fp;
            setvbuf(stdin, nullptr, _IONBF, 0);
        }

        // Redirect stderr
        {
            auto h = reinterpret_cast<intptr_t>(GetStdHandle(STD_ERROR_HANDLE));
            FILE* fp = _fdopen(_open_osfhandle(h, _O_TEXT), "w");
            *stderr = *fp;
            setvbuf(stderr, nullptr, _IONBF, 0);
        }

        std::ios::sync_with_stdio();
        return true;
    }
}

OsmiaConsoleChannel::OsmiaConsoleChannel(std::string windowTitle)
    : _consoleCreated(redirectIOToConsole())
    , _hConsole(GetStdHandle(STD_OUTPUT_HANDLE))
{
    if (!windowTitle.empty())
        SetConsoleTitleW(toUTF16(windowTitle).c_str());
}

OsmiaConsoleChannel::~OsmiaConsoleChannel()
{
    if (_consoleCreated)
    {
        FreeConsole();
        _consoleCreated = false;
    }
}

void OsmiaConsoleChannel::log(Log::Level level, std::string_view msg)
{
    std::wstring wtext = toUTF16(std::string(msg) + "\r\n");
    DWORD written = 0;
    WriteConsoleW(_hConsole, wtext.data(), static_cast<DWORD>(wtext.size()), &written, nullptr);
}

#else
OsmiaConsoleChannel::OsmiaConsoleChannel(std::string windowTitle)
    : _consoleCreated(false) {}
OsmiaConsoleChannel::~OsmiaConsoleChannel() {}
void OsmiaConsoleChannel::log(Log::Level level, std::string_view msg) {}
#endif
