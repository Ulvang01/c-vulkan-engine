#ifndef LOG_H
#define LOG_H

typedef enum
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

void logMessage(LogLevel level, const char *file, int line, const char *fmt, ...);
void logSetMinLevel(LogLevel level);

#define LOG_DEBUG(fmt, ...) logMessage(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) logMessage(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) logMessage(LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) logMessage(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif