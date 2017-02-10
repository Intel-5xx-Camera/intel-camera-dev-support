/*
 * Copyright (C) 2012 The Android Open Source Project
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
 * - reduced Fence into a stub to allow compiling in linux
 *
 */

#include <ui/Fence.h>
namespace android {

Fence::Fence() :
    mFenceFd(-1) {
}

Fence::Fence(int fenceFd) :
    mFenceFd(fenceFd) {
}

Fence::~Fence() {
}

status_t Fence::wait(int timeout) {
    (void) timeout;
    return OK;
}

status_t Fence::waitForever(const char* logname) {
    (void) logname;
    return OK;
}

sp<Fence> Fence::merge(const String8& name, const sp<Fence>& f1,
        const sp<Fence>& f2) {
    (void) name;
    (void) f1;
    (void) f2;
    return NULL;
}

int Fence::dup() const {
    return 0;
}

nsecs_t Fence::getSignalTime() const {
    return 0;
}

size_t Fence::getFlattenedSize() const {
    return 4;
}

size_t Fence::getFdCount() const {
    return 0;
}

status_t Fence::flatten(void*& buffer, size_t& size, int*& fds, size_t& count) const {
    (void) buffer;
    (void) size;
    (void) fds;
    (void) count;
    return OK;
}

status_t Fence::unflatten(void const*& buffer, size_t& size, int const*& fds, size_t& count) {
    (void) buffer;
    (void) size;
    (void) fds;
    (void) count;
    return OK;
}

} // namespace android
