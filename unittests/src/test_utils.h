/*
 * Copyright (C) 2016 Intel Corporation
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

#ifndef IPU4_UNITTESTS_SRC_TEST_UTILS_H_
#define IPU4_UNITTESTS_SRC_TEST_UTILS_H_

#include <gtest/gtest.h>

#define CDEV(dev) ((camera3_device_t *) dev)
#define DOPS(dev) (CDEV(dev)->ops)
#define DCOMMON(dev) (CDEV(dev)->common)

namespace t_i = testing::internal;

namespace testing {
namespace internal {
enum GTestColor {
    COLOR_DEFAULT, COLOR_RED, COLOR_GREEN, COLOR_YELLOW
};

extern void ColoredPrintf(GTestColor color, const char* fmt, ...);
} // namespace internal
} // namespace testing

#define PRINTF(...) \
do { \
    t_i::ColoredPrintf(t_i::COLOR_GREEN, "[          ] "); \
    t_i::ColoredPrintf(t_i::COLOR_YELLOW, __VA_ARGS__); \
} while(0)

#define PRINTLN(...) \
do { \
    PRINTF(__VA_ARGS__); \
    t_i::ColoredPrintf(t_i::COLOR_YELLOW, "\n"); \
} while(0)

#define BAIL(exittag, progress) \
    do { \
        /* add these, as needed */ \
        if (progress == 0) goto exittag##0; \
        if (progress == 1) goto exittag##1; \
        if (progress == 2) goto exittag##2; \
        ASSERT_TRUE(progress < 3); \
    } while (0)

#endif /* IPU4_UNITTESTS_SRC_TEST_UTILS_H_ */
