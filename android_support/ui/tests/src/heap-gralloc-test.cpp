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

#include "gtest/gtest.h"
#define LOG_TAG "libui-test"

#include <utils/Log.h>
#include "ui/GraphicBufferAllocator.h"
#include "ui/GraphicBufferMapper.h"
#include "ui/GraphicBuffer.h"
#include "ui/Rect.h"

using namespace android;

TEST(HeapGrallocTest, allocateGraphicBuffer) {

    GraphicBufferAllocator &gba = GraphicBufferAllocator::get();

    buffer_handle_t handle;
    uint32_t stride;

    status_t status = gba.alloc(32,32,PIXEL_FORMAT_RGB_888,0,&handle, &stride);

    ASSERT_EQ(status, OK);
    ALOGD(" Allocation succeed stride = %d", stride);

    status = gba.free(handle);
    ASSERT_EQ(status, OK);
    ALOGD(" free succeed");
}


TEST(HeapGrallocTest, mapNativeHandle) {

    GraphicBufferAllocator &gba = GraphicBufferAllocator::get();
    GraphicBufferMapper &gbm = GraphicBufferMapper::get();
    buffer_handle_t handle;
    Rect bounds(32,32);
    uint32_t stride;
    void* address;


    status_t status = gba.alloc(32,32,PIXEL_FORMAT_RGB_888,0,&handle, &stride);

    ASSERT_EQ(status, OK);
    ALOGD(" Allocation succeed");

    status = gbm.lock(static_cast<const native_handle*>(handle),0,bounds,&address);
    ASSERT_EQ(status, OK);
    ALOGD(" mapping succeed %p", address);

    /*
     * Check that there is actual memory backing up the address
     */
    uint8_t *dest = (uint8_t*)address;
    for (size_t i = 0; i < 32; i++) {
        dest[i] = i;
    }

    for (size_t i = 0; i < 32; i++) {
        ASSERT_EQ(dest[i], i);
    }
    ALOGD(" stored 32 bytes in buffer ");
    status = gba.free(handle);
    ASSERT_EQ(status, OK);
    ALOGD(" free succeed");
}

TEST(HeapGrallocTest, GraphicBufferTest) {

    ALOGD("test GraphicBufferTest START-----");
    buffer_handle_t handle;
    Rect bounds(32,32);
    status_t status = OK;
    void* address = NULL;

    GraphicBuffer *buffer = new GraphicBuffer(32, 32,PIXEL_FORMAT_RGB_888, 0);
    status = buffer->initCheck();

    ASSERT_TRUE(status == OK) << "Failed to allocate graphic Buffer";
    ALOGD(" Allocation succeed stride %d", buffer->getStride());

    status = buffer->lock(0,bounds,&address);

    ASSERT_TRUE(status == OK);
    ALOGD(" mapping succeed %p", address);

    /*
     * Check that there is actual memory backing up the address
     */
    uint8_t *dest = (uint8_t*)address;
    for (size_t i = 0; i < 32; i++) {
        dest[i] = i;
    }

    for (size_t i = 0; i < 32; i++) {
        ASSERT_EQ(dest[i], i);
    }
    ALOGD(" stored 32 bytes in buffer ");
    ALOGD("test GraphicBufferTest  END-----");
}

