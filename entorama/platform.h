#ifndef platform_h
#define platform_h

#define PLATFORM_WIN


void platform_sleep(int milliseconds);
void *platform_fopen(const char *path, const char *mode);
void platform_fclose(void *file);
void *platform_load_library(const char *path);
void *platform_load_library_function(void *library, const char *name);

#endif
