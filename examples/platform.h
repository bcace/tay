#ifndef platform_h
#define platform_h

#define PLATFORM_WIN


void platform_sleep(int milliseconds);
void *platform_fopen(const char *path, const char *mode);

#endif
