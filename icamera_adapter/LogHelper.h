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

#ifndef _LOGHELPER_H_
#define _LOGHELPER_H_

#include <inttypes.h> // For PRId64 used to print int64_t for both 64bits and 32bits
#include <utils/Log.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>
#include <utils/KeyedVector.h>
#include <utils/Timers.h>
#include <cutils/trace.h>

#define UNUSED(x) (void)(x)
#define UNUSED1(a)                                                  (void)(a)
#define UNUSED2(a,b)                                                (void)(a),UNUSED1(b)
#define UNUSED3(a,b,c)                                              (void)(a),UNUSED2(b,c)
#define UNUSED4(a,b,c,d)                                            (void)(a),UNUSED3(b,c,d)
#define UNUSED5(a,b,c,d,e)                                          (void)(a),UNUSED4(b,c,d,e)
#define UNUSED6(a,b,c,d,e,f)                                        (void)(a),UNUSED5(b,c,d,e,f)
#define UNUSED7(a,b,c,d,e,f,g)                                      (void)(a),UNUSED6(b,c,d,e,f,g)
#define UNUSED8(a,b,c,d,e,f,g,h)                                    (void)(a),UNUSED7(b,c,d,e,f,g,h)
#define UNUSED9(a,b,c,d,e,f,g,h,i)                                  (void)(a),UNUSED8(b,c,d,e,f,g,h,i)
#define UNUSED10(a,b,c,d,e,f,g,h,i,j)                               (void)(a),UNUSED9(b,c,d,e,f,g,h,i,j)
#define UNUSED11(a,b,c,d,e,f,g,h,i,j,k)                             (void)(a),UNUSED10(b,c,d,e,f,g,h,i,j,k)
#define UNUSED12(a,b,c,d,e,f,g,h,i,j,k,l)                           (void)(a),UNUSED11(b,c,d,e,f,g,h,i,j,k,l)
#define UNUSED13(a,b,c,d,e,f,g,h,i,j,k,l,m)                         (void)(a),UNUSED12(b,c,d,e,f,g,h,i,j,k,l,m)
#define UNUSED14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)                       (void)(a),UNUSED13(b,c,d,e,f,g,h,i,j,k,l,m,n)
#define UNUSED15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)                     (void)(a),UNUSED14(b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#define UNUSED16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)                   (void)(a),UNUSED15(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
#define UNUSED17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)                 (void)(a),UNUSED16(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)
#define UNUSED18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)               (void)(a),UNUSED17(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)
#define UNUSED19(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)             (void)(a),UNUSED18(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)
#define UNUSED20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)           (void)(a),UNUSED19(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)
#define UNUSED21(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)         (void)(a),UNUSED20(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)
#define UNUSED22(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)       (void)(a),UNUSED21(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)
#define UNUSED23(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w)     (void)(a),UNUSED22(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w)
#define UNUSED24(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x)   (void)(a),UNUSED23(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x)
#define VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, N,...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8 ,7 ,6, 5, 4, 3, 2, 1)
#define ALL_UNUSED_IMPL_(nargs) UNUSED ## nargs
#define ALL_UNUSED_IMPL(nargs) ALL_UNUSED_IMPL_(nargs)
#define ALL_UNUSED(...) ALL_UNUSED_IMPL( VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ )

/**
 * global log level
 * This global variable is set from system properties
 * It is used to control the level of verbosity of the traces in logcat
 * It is also used to store the status of certain RD features
 */
extern int32_t gIcameraLogLevel;

/**
 * LOG levels
 *
 * LEVEL 1 is used to track events in the that are relevant during
 * the operation of the camera, but are not happening on a per frame basis.
 * this ensures that the level of logging is not too verbose
 *
 * LEVEL 2 is used to track information on a per request basis
 *
 */
enum  {
    /* verbosity level of general traces */
    CAMERA_DEBUG_LOG_LEVEL1 = 1,
    CAMERA_DEBUG_LOG_LEVEL2 = 1 << 1,

    /* space reserved for new log levels */

    /* Make logs persistent, retrying if logcat is busy */
    CAMERA_DEBUG_LOG_PERSISTENT = 1 << 12 /* 4096 */
};

// Dedicated namespace section for function declarations
namespace icamera {

void __icamera_log(bool condition, int prio, const char *tag,
                      const char *fmt, ...);

void cca_print_error(const char *fmt, va_list ap);

void cca_print_debug(const char *fmt, va_list ap);

void cca_print_info(const char *fmt, va_list ap);

} // namespace icamera

// enfore same prefix for all ICamera log tags
#define ICAMERA_PREFIX "ICAMERA_"
#define ICAMERA_TAG ICAMERA_PREFIX LOG_TAG

// Take the namespaced function to be used for unqualified
// lookup  in the LOG* macros below
#ifdef ICAMERA_DEBUG
using icamera::__icamera_log;

#define LOG1(...) __icamera_log(gIcameraLogLevel & CAMERA_DEBUG_LOG_LEVEL1, ANDROID_LOG_DEBUG, ICAMERA_TAG, __VA_ARGS__)
#define LOG2(...) __icamera_log(gIcameraLogLevel & CAMERA_DEBUG_LOG_LEVEL2, ANDROID_LOG_DEBUG, ICAMERA_TAG, __VA_ARGS__)

#define LOGE(...) __icamera_log(true, ANDROID_LOG_ERROR, ICAMERA_TAG, __VA_ARGS__)
#define LOGD(...) __icamera_log(true, ANDROID_LOG_DEBUG, ICAMERA_TAG, __VA_ARGS__)
#define LOGW(...) __icamera_log(true, ANDROID_LOG_WARN, ICAMERA_TAG, __VA_ARGS__)
#define LOGV(...) __icamera_log(true, ANDROID_LOG_VERBOSE, ICAMERA_TAG, __VA_ARGS__)

// CAMTRACE_NAME traces the beginning and end of the current scope.  To trace
// the correct start and end times this macro should be declared first in the
// scope body.
#define HAL_TRACE_NAME(level, name) icamera::ScopedTrace ___tracer(level, name )
#define HAL_TRACE_CALL(level) HAL_TRACE_NAME(level, __FUNCTION__)
#define HAL_TRACE_CALL_PRETTY(level) HAL_TRACE_NAME(level, __PRETTY_FUNCTION__)
#else
#define LOG1(...) ALL_UNUSED_IMPL( VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ )
#define LOG2(...) ALL_UNUSED_IMPL( VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ )

#define LOGE    ALOGE
#define LOGD    ALOGD
#define LOGW    ALOGW
#define LOGV    ALOGV

#define HAL_TRACE_NAME(level, name)
#define HAL_TRACE_CALL(level)
#define HAL_TRACE_CALL_PRETTY(level)
#endif

namespace icamera {

class ScopedTrace {
public:
inline ScopedTrace(int level, const char* name) :
        mLevel(level),
        mName(name) {
    __icamera_log(gIcameraLogLevel & mLevel, ANDROID_LOG_DEBUG, ICAMERA_TAG, "ENTER-%s", mName);
}

inline ~ScopedTrace() {
    __icamera_log(gIcameraLogLevel & mLevel, ANDROID_LOG_DEBUG, ICAMERA_TAG, "EXIT-%s", mName);
}

private:
    int mLevel;
    const char* mName;
};

namespace LogHelper {
/**
 * Runtime selection of debugging level.
 */
void setDebugLevel(void);

} // namespace LogHelper
} // namespace icamera
#endif // _LOGHELPER_H_
