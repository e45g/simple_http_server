#ifndef UTILS_H
#define UTILS_H

void logg(long line, const char *file, const char *func, const char *format, ...);
#define LOG(format, ...) logg(__LINE__, __FILE__, __PRETTY_FUNCTION__, format, ##__VA_ARGS__)

#endif
