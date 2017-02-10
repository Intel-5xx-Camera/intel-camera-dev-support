/*
 * Copyright (C) 2007-2016 The Android Open Source Project
 * Copyright (C) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modified by Intel Corporation.
 * - this is an adaptation of the Android logging API to vsyslog
 */

#include "log/log.h"
#include <stdlib.h>
#include <syslog.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <string>

#define LOG_BUF_SIZE 1024

int __android_log_is_loggable(int prio, const char *tag, int def)
{
    // todo implementation.
    (void) prio;
    (void) tag;
    (void) def;
    return 1;
}

void __android_log_assert(const char *cond, const char *tag,
                          const char *fmt, ...)
{
    std::string buf;

    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        __android_log_vprint(ANDROID_LOG_FATAL, tag, "%s", ap);
        va_end(ap);
        abort();
    } else {
        if (cond) {
            buf += "Assertion failed: ";
            buf += cond;
        }
        else {
            buf += "Assertion failed with NULL condition message.";
        }
    }

    __android_log_print(ANDROID_LOG_FATAL, tag, "%s", buf.c_str());
    abort();
}

void __attribute__ ((constructor)) start_logging(void);
void __attribute__ ((destructor)) end_logging(void);

void start_logging(void)
{
    openlog("camerahal", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
}

void end_logging(void)
{
    closelog();
}

int __android_log_vprint(int prio,
                         const char *tag,
                         const char *fmt,
                         va_list args)
{
    int priority;
    std::string buf(tag);

    switch (prio) {
    case ANDROID_LOG_INFO:
        priority = LOG_INFO;
        break;
    case ANDROID_LOG_WARN:
        priority = LOG_WARNING;
        break;
    case ANDROID_LOG_FATAL:
        priority = LOG_ALERT;
        break;
    case ANDROID_LOG_ERROR:
        priority = LOG_ERR;
        break;
    default:
        priority = LOG_DEBUG;
        break;
    }

    buf += ":";
    buf += fmt;

    vsyslog(priority, buf.c_str(), args);

    return 0;
}

int __android_log_print(int prio,
                        const char *tag,
                        const char *fmt,
                        ...)
{
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(prio, tag, fmt, args);
    va_end(args);

    return 0;
}

