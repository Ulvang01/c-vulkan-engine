#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static const char *levelStrings[] = { "DEBUG", "INFO", "WARN", "ERROR" };

// ANSI color codes, one per level
static const char *levelColors[] = {
    "\x1b[36m", // DEBUG: cyan
    "\x1b[32m", // INFO: green
    "\x1b[33m", // WARN: yellow
    "\x1b[31m", // ERROR: red
};
static const char *colorReset = "\x1b[0m";
static const char *colorGray = "\x1b[90m"; // bright black, reads as gray

static LogLevel minLevel = LOG_INFO;

void logSetMinLevel(LogLevel level) { minLevel = level; }

void logMessage(LogLevel level, const char *file, int line, const char *fmt, ...)
{
    if (level < minLevel)
        return;

    FILE *out = (level >= LOG_WARN) ? stderr : stdout;

    fprintf(out, "%s[%s]%s %s%s:%d:%s ", levelColors[level], levelStrings[level], colorReset,
            colorGray, file, line, colorReset);

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    if (level >= LOG_ERROR)
    {
        fprintf(out, "\n\n");
    }
    else
    {
        fprintf(out, "\n");
    }
}