/*
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
 */

#ifndef _ICAMERAADAPTER_H_
#define _ICAMERAADAPTER_H_

#include "Parameters.h"
#include "hardware/camera3.h"
#include "ui/GraphicBuffer.h"
#include "ui/GraphicBufferMapper.h"
#include "Errors.h"
#include <vector>
#include <map>

namespace icamera {

class ICameraAdapter : private camera3_callback_ops {
public:
    ICameraAdapter(int cameraId);
    virtual ~ICameraAdapter();

    // camera3_callback_ops static functions for function pointers
    static void s_notify(const struct camera3_callback_ops *ops,
                         const camera3_notify_msg_t *msg);
    static void s_process_capture_result(const struct camera3_callback_ops *ops,
                const camera3_capture_result_t *result);
    // camera3_callback_ops instance implementation(s)
    void processCaptureResult(const camera3_capture_result_t *result);

    status_t open();
    status_t close();
    status_t start();
    status_t stop();
    status_t setParameters(const icamera::Parameters& param);
    status_t getParameters(icamera::Parameters& param);
    status_t configStreams(icamera::stream_config_t *stream_list);
    status_t allocateMemory(icamera::camera_buffer_t *buffer);
    status_t dqBuf(int stream_id, icamera::camera_buffer_t **buffer);
    status_t qBuf(int stream_id, icamera::camera_buffer_t *buffer);

private: // types
    // operation modes used in stream config
    enum {
        OP_MODE_DEFAULT =   0,      // default
        OP_MODE_30_FPS =    1 << 0, // 1
        OP_MODE_60_FPS =    1 << 1, // 2
        OP_MODE_90_FPS =    1 << 2, // 4
        OP_MODE_120_FPS =   1 << 3, // 8
        OP_MODE_DVS =       1 << 8, // 256
    };

    struct BufferWrapper {
        int stream_id;
        icamera::camera_buffer_t *buffer;
    };

    struct Result {
        bool metadataDone;
        bool buffersDone;
        uint64_t timestamp; /**< buffer timestamp, for storing metadata value before buffer arrives */
    };


private: // functions
    status_t capture(int stream_id, icamera::camera_buffer_t *buffer);
    status_t constructDefaultRequest();
    status_t mapMemory(icamera::camera_buffer_t *buffer);

private: // members
    hw_device_t *mDevice;
    int mCameraId;
    bool mStarted;
    std::vector<BufferWrapper> mPendingBuffers;       /**< buffers which are queued in HAL before calling start() */
    std::vector<android::sp<android::GraphicBuffer>> mAllocatedBuffers; /**< buffer destruction storage */
    std::vector<BufferWrapper> mQueuedBuffers;        /**< buffers which are queued for capture */
    std::vector<BufferWrapper> mCapturedBuffers;      /**< buffers which are waiting for dqbuf */
    std::vector<buffer_handle_t *> mMappedBuffers;    /**< mmapped buffers, storage for destruction */
    std::map<uint32_t, Result> mResults;              /**< camera3hal capture results */
    std::map<void *, camera3_stream_buffer> mBufferMapping; /**< for finding the cam3 buffer struct with a pointer */
    camera3_stream_t mStream;
    android::Mutex mLock;
    android::Condition mCondition;
    camera_metadata_t *mRequestSettings;
    int mOperationMode; /**< used to pass fps to HAL */
};

} // namespace icamera

#endif /* _ICAMERAADAPTER_H_ */
