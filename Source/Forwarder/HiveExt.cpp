/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   define LIB_LOAD(p)      LoadLibraryA(p)
#   define LIB_SYMBOL(h, n) GetProcAddress(h, n)
using LibHandle = HMODULE;
#else
#   include <dlfcn.h>
#   include <cstring>
#   include <cstdlib>
#   define LIB_LOAD(p)      dlopen(p, RTLD_LAZY)
#   define LIB_SYMBOL(h, n) dlsym(h, n)
#   define __stdcall
using LibHandle = void*;
#endif
#include <cstdio>

using RVExtensionFn = void(__stdcall*)(char*, int, const char*);

static LibHandle     hOsmia = nullptr;
static RVExtensionFn fnRVExtension = nullptr;

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif

static bool loadOsmia()
{
    if (fnRVExtension) return true;

#ifdef _WIN32
    char path[MAX_PATH] = {};
    HMODULE hSelf = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&loadOsmia, &hSelf);
    GetModuleFileNameA(hSelf, path, MAX_PATH);
    char* slash = strrchr(path, '\\');
    if (slash) strcpy_s(slash + 1, MAX_PATH - (slash - path) - 1, "Osmia.dll");
#else
    Dl_info info{};
    if (!dladdr(reinterpret_cast<void*>(&loadOsmia), &info) || !info.dli_fname)
        return false;
    char path[4096] = {};
    strncpy(path, info.dli_fname, sizeof(path) - 1);
    char* slash = strrchr(path, '/');
    if (slash) strcpy(slash + 1, "Osmia.so");
#endif

    hOsmia = LIB_LOAD(path);
    if (!hOsmia) return false;

#ifdef _WIN32
    fnRVExtension = reinterpret_cast<RVExtensionFn>(LIB_SYMBOL(hOsmia, "_RVExtension@12"));
#else
    fnRVExtension = reinterpret_cast<RVExtensionFn>(LIB_SYMBOL(hOsmia, "RVExtension"));
#endif
    return fnRVExtension != nullptr;
}

extern "C"
{
#ifdef _WIN32
    __declspec(dllexport)
#else
    __attribute__((visibility("default")))
#endif
        void __stdcall RVExtension(char* output, int outputSize, const char* function)
    {
        if (!loadOsmia())
        {
            snprintf(output, outputSize, "[\"ERROR\",\"Cannot load Osmia\"]");
            return;
        }
        fnRVExtension(output, outputSize, function);
    }
}