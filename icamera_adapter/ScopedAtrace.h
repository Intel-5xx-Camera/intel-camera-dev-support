/*
 * Copyright (C) 2015-2016 Intel Corporation.
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
// this file is from libcamhal and ideally should be shared with it somehow
#ifndef _SCOPEDATRACE_H_
#define _SCOPEDATRACE_H_
/* This file exists to satisfy the rather surprising dependency between gst
 * icamerasrc and libcamhal, which is outside of the official icamera API.
 * Namely, gstcamerasrc uses the ScopedAtrace object directly from the HAL
 * library. Therefore we must provide such a header, even if an empty
 * implementation such as this.
 *
 * icamerasrc should be fixed to use its own tracing facilities, or a common
 * utility library.
 */
namespace icamera {
class ScopedAtrace {
      public:
          ScopedAtrace(const char*, const char*, const char* = (char*)0, int = -1, \
                       const char* = (char*)0, int = -1, const char* = (char*)0, int = -1) {}
          ~ScopedAtrace() {}
          static void setTraceLevel(int) {}
};
#define PERF_CAMERA_ATRACE() ScopedAtrace atrace(__func__, LOG_TAG);
#define PERF_CAMERA_ATRACE_PARAM1(note, value) ScopedAtrace atrace(__func__, LOG_TAG, note, value);
#define PERF_CAMERA_ATRACE_PARAM2(note, value, note2, value2) \
        ScopedAtrace atrace(__func__, LOG_TAG, note, value, note2, value2);
#define PERF_CAMERA_ATRACE_PARAM3(note, value, note2, value2, note3, value3) \
        ScopedAtrace atrace(__func__, LOG_TAG, note, value, note2, value2, note3, value3);

} // namespace icamera
#endif //_SCOPEDATRACE_H_
