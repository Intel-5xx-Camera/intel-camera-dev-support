/*
 * Copyright 2016 The Android Open Source Project
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
 * - removed dependency to pid_t
 * - added definition of gettid();
 *
 */

#ifndef ANDROID_ISCHEDULING_POLICY_SERVICE_H
#define ANDROID_ISCHEDULING_POLICY_SERVICE_H

#include <stdint.h>

int32_t gettid();

namespace android {

int requestPriority(/*pid_t*/int32_t pid,
                    /*pid_t*/int32_t tid,
                    int32_t prio,
                    bool asynchronous = false);

}   // namespace android

#endif  // ANDROID_ISCHEDULING_POLICY_SERVICE_H
