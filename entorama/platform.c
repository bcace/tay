#include "platform.h"
#include <stdio.h>
#include <assert.h>

#ifdef PLATFORM_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif


void platform_sleep(int milliseconds) {
#ifdef PLATFORM_WIN
	Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

void *platform_fopen(const char *path, const char *mode) {
    FILE *f = 0;
#ifdef PLATFORM_WIN
    fopen_s(&f, path, mode);
#else
    f = fopen(path, mode);
#endif
    assert(f != 0);
    return f;
}

void platform_fclose(void *file) {
    fclose(file);
}

void *platform_load_library(const char *path) {
    void *lib = 0;
#ifdef PLATFORM_WIN
    lib = LoadLibraryA(path);
#else
#endif
    return lib;
}

void *platform_load_library_function(void *library, const char *name) {
    void *func = 0;
#ifdef PLATFORM_WIN
    func = GetProcAddress(library, name);
#else
#endif
    return func;
}

