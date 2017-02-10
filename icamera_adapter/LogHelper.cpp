/*
 * Copyright (C) 2012-2015 Intel Corporation
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
 */

#define LOG_TAG "LogHelper"

#include <stdint.h>
#include <limits.h> // INT_MAX, INT_MIN
#include <stdlib.h> // atoi.h
#include <utils/Log.h>
#include <cutils/properties.h>
#include <stdarg.h>

#include "LogHelper.h"

int32_t gIcameraLogLevel = 0;

namespace icamera {

void __icamera_log(bool condition, int prio, const char *tag,
                      const char *fmt, ...)
{
    if (condition) {
        va_list ap;
        va_start(ap, fmt);
        if (gIcameraLogLevel & CAMERA_DEBUG_LOG_PERSISTENT) {
            int errnoCopy;
            unsigned int maxTries = 20;
            do {
                errno = 0;
                __android_log_vprint(prio, tag, fmt, ap);
                errnoCopy = errno;
                if (errnoCopy == EAGAIN)
                    usleep(2000); /* sleep 2ms */
            } while(errnoCopy == EAGAIN && maxTries--);
        } else {
            __android_log_vprint(prio, tag, fmt, ap);
        }
        va_end(ap);
    }
}

} // namespace icamera

using namespace icamera;

void LogHelper::setDebugLevel(void)
{
    // for now, use the same debug property with camera hal
    const char* PROP_ICAMERA_DEBUG = "camera.hal.debug";

    char debugPropTmp [PROPERTY_VALUE_MAX];

    //update atrace tags status.
    atrace_set_tracing_enabled(true);

    if (property_get(PROP_ICAMERA_DEBUG, debugPropTmp, NULL)) {
        gIcameraLogLevel = atoi(debugPropTmp);
        LOGD("Debug level is 0x%x", gIcameraLogLevel);

        // Check that the property value is a valid integer
        if (gIcameraLogLevel >= INT_MAX || gIcameraLogLevel <= INT_MIN) {
            LOGD("Invalid %s property integer value: %s", PROP_ICAMERA_DEBUG, debugPropTmp);
            gIcameraLogLevel = 0;
        }

        // legacy support: "setprop camera.hal.debug 2" is expected
        // to enable both LOG1 and LOG2 traces
        if (gIcameraLogLevel & CAMERA_DEBUG_LOG_LEVEL2)
            gIcameraLogLevel |= CAMERA_DEBUG_LOG_LEVEL1;
    }
}


