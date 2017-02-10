/*
 * Copyright (C) 2007 The Android Open Source Project
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
 * - made a stub out of CallStack
 */

#define LOG_TAG "CallStack"

#include <utils/CallStack.h>
#include <utils/Printer.h>
#include <utils/Errors.h>
#include <utils/Log.h>

#include <backtrace/Backtrace.h>

/* a CallStack implementation for Linux should be written here.
 * currently just a stub */

namespace android {

CallStack::CallStack() {
}

CallStack::CallStack(const char*, int32_t) {
}

CallStack::~CallStack() {
}

void CallStack::update(int32_t, pid_t) {
}

void CallStack::log(const char*, android_LogPriority, const char*) const {
}

void CallStack::dump(int, int, const char*) const {
}

String8 CallStack::toString(const char*) const {
    String8 str;
    return str;
}

void CallStack::print(Printer&) const {
}

}; // namespace android
