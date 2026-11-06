/*
 * Copyright (C) 2026 Kenneth Looney
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "kace/loader.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

Loader::~Loader() {
    close();
}

void Loader::close() {
    if (instance && destroy) destroy(instance);
#ifdef _WIN32
    if (library) FreeLibrary(static_cast<HMODULE>(library));
#else
    if (library) dlclose(library);
#endif
    instance = nullptr;
    destroy = nullptr;
    library = nullptr;
}

bool Loader::open(const std::string& path, std::string& error) {
    error.clear();
    if (library) { error = "Plugin already open"; return false; }
#ifdef _WIN32
    library = LoadLibraryA(path.c_str());
    if (!library) { error = "Could not load plugin (Windows error " + std::to_string(GetLastError()) + ")"; return false; }
    auto create = reinterpret_cast<Cpu* (*)()>(GetProcAddress(static_cast<HMODULE>(library), "create_cpu"));
    destroy = reinterpret_cast<void (*)(Cpu*)>(GetProcAddress(static_cast<HMODULE>(library), "destroy_cpu"));
#else
    library = dlopen(path.c_str(), RTLD_NOW);
    if (!library) { error = dlerror(); return false; }
    auto create = reinterpret_cast<Cpu* (*)()>(dlsym(library, "create_cpu"));
    destroy = reinterpret_cast<void (*)(Cpu*)>(dlsym(library, "destroy_cpu"));
#endif
    if (!create || !destroy) { error = "Plugin is missing its factory functions"; close(); return false; }
    instance = create();
    if (!instance) { error = "Plugin did not create a CPU"; close(); return false; }
    return true;
}
