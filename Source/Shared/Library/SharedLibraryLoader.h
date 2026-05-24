/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "Shared/Common/Types.h"
#include <stdexcept>

#ifdef _WIN32
#   include <windows.h>
using LibHandle = HMODULE;
#   define LIB_LOAD(path)         LoadLibraryA(path)
#   define LIB_SYMBOL(h, name)    GetProcAddress(h, name)
#   define LIB_FREE(h)            FreeLibrary(h)
#else
#   include <dlfcn.h>
using LibHandle = void*;
#   define LIB_LOAD(path)         dlopen(path, RTLD_LAZY)
#   define LIB_SYMBOL(h, name)    dlsym(h, name)
#   define LIB_FREE(h)            dlclose(h)
#endif

template <typename Base>
class SharedLibraryLoader
{
public:
    using FactoryFn = Base * (*)();
    using DestroyFn = void  (*)(Base*);

    SharedLibraryLoader() = default;
    ~SharedLibraryLoader()
    {
        if (_hLib)
            LIB_FREE(_hLib);
    }

    void loadLibrary(const string& libName)
    {
#ifdef _WIN32
        string path = libName + ".dll";
#else
        string path = "lib" + libName + ".so";
#endif
        _hLib = LIB_LOAD(path.c_str());
        if (!_hLib)
            throw std::runtime_error("SharedLibraryLoader: cannot load " + path);

        _factory = reinterpret_cast<FactoryFn>(LIB_SYMBOL(_hLib, "createInstance"));
        _destroy = reinterpret_cast<DestroyFn>(LIB_SYMBOL(_hLib, "destroyInstance"));
        if (!_factory || !_destroy)
            throw std::runtime_error("SharedLibraryLoader: missing createInstance/destroyInstance in " + path);
    }

    Base* create() const
    {
        if (!_factory)
            throw std::runtime_error("SharedLibraryLoader: library not loaded");
        return _factory();
    }

    void destroy(Base* obj) const
    {
        if (_destroy && obj)
            _destroy(obj);
    }

    bool isLoaded() const { return _hLib != nullptr; }

    SharedLibraryLoader(const SharedLibraryLoader&) = delete;
    SharedLibraryLoader& operator=(const SharedLibraryLoader&) = delete;

private:
    LibHandle _hLib = nullptr;
    FactoryFn _factory = nullptr;
    DestroyFn _destroy = nullptr;
};
