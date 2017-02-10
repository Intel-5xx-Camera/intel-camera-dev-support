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

#define LOG_TAG "ICameraAdapter"

#include "ICamera.h"
#include "ICameraAdapter.h"
#include "ParameterAdapter.h"
#include "camera/CameraMetadata.h"
#include "camera_metadata_hidden.h"
#include "Errors.h"
#include "LogHelper.h"
#include <linux/videodev2.h>
#include <hardware/gralloc.h>
#include <string>

using std::pair;
using std::vector;
using std::map;
using std::string;
using std::to_string;
using android::Rect;
using android::GraphicBufferMapper;
using android::GraphicBuffer;
using android::sp;
using android::Mutex;
using android::Condition;
const uint64_t ONE_SECOND = 1000000000;
extern camera_module_t HAL_MODULE_INFO_SYM;
extern gralloc_module_t GRALLOC_HAL_MODULE_INFO_SYM;

/* icamera C-api implementation.
 * Mostly a wrapper which calls into ICameraAdapter. */
namespace icamera {

#define MAX_CAMERAS 2
#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define ALIGN16(x) (((x) + 15) & ~15)

#define CDEV(dev) ((camera3_device_t *) dev)
// DOPS means Device OPS
#define DOPS(dev) (CDEV(dev)->ops)
#define DCOMMON(dev) (CDEV(dev)->common)

#define CAMERA_ID_CHECK(camera_id) \
do { \
    int numCameras = HAL_MODULE_INFO_SYM.get_number_of_cameras(); \
    if (camera_id < 0 || camera_id >= numCameras) \
        return BAD_VALUE; \
} while (0)

#define CALL_ADAPTOR_AND_RETURN(camera_id, function) \
do { \
    if (sCamAdapters[camera_id] != NULL) { \
        return sCamAdapters[camera_id]->function; \
    } \
    return UNKNOWN_ERROR; \
} while (0)

__attribute__ ((init_priority (101))) static Parameters sParameters[MAX_CAMERAS];
static ICameraAdapter* sCamAdapters[MAX_CAMERAS];
static const char *sCameraNames[MAX_CAMERAS] = {
        "camera0",
        "camera1"
};

int get_number_of_cameras()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    return HAL_MODULE_INFO_SYM.get_number_of_cameras();
}

int get_camera_info(int camera_id, camera_info_t& info)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    CAMERA_ID_CHECK(camera_id);

    struct camera_info ac2info;
    HAL_MODULE_INFO_SYM.get_camera_info(camera_id, &ac2info);

    info.orientation = ac2info.orientation;
    info.facing = ac2info.facing;
    // todo vendor metadata could be added to contain camera (and/or sensor)
    // names, but for now we use hardcoded rather anonymous names
    info.name = sCameraNames[camera_id];
    info.description = sCameraNames[camera_id];
    info.device_version = 1; // as good as in libcamhal and not used by gst src

    // we need to support the getSupportedStreamConfig API of the Parameters.h
    // in the info.capability pointer. The simplest way is to write stream
    // configs into an array of Parameters at around library load time, and then
    // assign the param ptr to info like this:
    info.capability = &sParameters[camera_id];

    return OK;
}

/* This function sets up the static metadata Parameters objects, in practice
 * only the stream configs. This is called from the HAL library constructor.
 */
int camera_hal_init()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    static bool initDone = false;

    if (initDone)
        return OK;

    if (HAL_MODULE_INFO_SYM.get_vendor_tag_ops != NULL) {
        static vendor_tag_ops_t ops;
        HAL_MODULE_INFO_SYM.get_vendor_tag_ops(&ops);
        set_camera_metadata_vendor_ops(&ops);
    }

    struct camera_info ac2info;
    for (int cameraId = 0; cameraId < MAX_CAMERAS; cameraId++) {
        int count = 0;
        HAL_MODULE_INFO_SYM.get_camera_info(cameraId, &ac2info);
        const camera_metadata_t *meta = ac2info.static_camera_characteristics;
        if (!meta) {
            LOGE("@%s: Cannot get static metadata.", __FUNCTION__);
            return UNKNOWN_ERROR;
        }

        camera_metadata_ro_entry_t entry;
        CLEAR(entry);
        int ret = find_camera_metadata_ro_entry(meta,
                                 ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                 &entry);

        if (ret != OK) {
            LOGE("@%s: Cannot get stream configuration from static metadata.",
                 __FUNCTION__);
            return UNKNOWN_ERROR;
        }

        count = entry.count;
        const int32_t *availStreamConfig = entry.data.i32;

        if (availStreamConfig == NULL || count < 4) {
            LOGE("@%s: Empty stream configuration in static metadata.",
                 __FUNCTION__);
            return UNKNOWN_ERROR;
        }

        // figure out suitable stream configs, push them to vector
        vector<int> streamConfigVec;
        for (uint32_t j = 0; j < (uint32_t)count; j += 4) {
            // only process the nv12 outputs, for now
            if (availStreamConfig[j] == HAL_PIXEL_FORMAT_YCbCr_420_888 &&
                availStreamConfig[j+3] == CAMERA3_STREAM_OUTPUT) {
                // for some strange reason, Parameters.cpp expects to read
                // width, height & field from the metadata, and it invents
                // stride and size. Parameters.cpp also expects the metadata to
                // have as many ints as struct stream_t which is 8. So we need
                // dummy ints in there.
                streamConfigVec.push_back(V4L2_PIX_FMT_NV12);        // format
                streamConfigVec.push_back(availStreamConfig[j + 1]); // width
                streamConfigVec.push_back(availStreamConfig[j + 2]); // height
                streamConfigVec.push_back(0); // field
                streamConfigVec.push_back(0); // dummy
                streamConfigVec.push_back(0); // dummy
                streamConfigVec.push_back(0); // dummy
                streamConfigVec.push_back(0); // dummy
            }
        }

        // pull the stream configs from the vector as an array
        count = streamConfigVec.size();
        if (count == 0) {
            LOGE("no valid output stream configs");
            return UNKNOWN_ERROR;
        }
        // take a pointer to the array of ints
        const int *data = &streamConfigVec[0];
        // write configs into a metadata entry; allocate some excess space
        camera_metadata_t *streamConfigs = allocate_camera_metadata(20, 32768);
        ret = add_camera_metadata_entry(streamConfigs,
                ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                data,
                count);

        // wrap them up as a CameraMetadata object for the Parameters class API
        CameraMetadata cameraMetadata;
        // cloning requires const
        cameraMetadata = const_cast<const camera_metadata_t *>(streamConfigs);

        // now finally, merge it to the static Parameters instance
        sParameters[cameraId].merge(&cameraMetadata);

        // free the metadata
        free_camera_metadata(streamConfigs);
    }
    initDone = true;
    return OK;
}

int camera_hal_deinit()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    // the Parameters class implementation from libcamhal does not have an API
    // to dynamically release and re-create the metadata object, so we have to
    // leave the static instances dangling.
    // todo - we could either refactor the Parameters class or we could use
    // static pointers and dynamically allocated instances, if these objects
    // are to be mopped up in this function.
    return OK;
}

int camera_device_open(int camera_id, int vc_num)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    CAMERA_ID_CHECK(camera_id);
    sCamAdapters[camera_id] = new ICameraAdapter(camera_id);
    CALL_ADAPTOR_AND_RETURN(camera_id, open());
}

void camera_device_close(int camera_id)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    int numCameras = HAL_MODULE_INFO_SYM.get_number_of_cameras();
    if (camera_id < 0 || camera_id >= numCameras)
        return;

    if (sCamAdapters[camera_id] != NULL) {
        sCamAdapters[camera_id]->close();

        delete sCamAdapters[camera_id];
        sCamAdapters[camera_id] = NULL;
    }
}

int get_frame_size(int format, int width, int height, int field, int *bpp)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    // currently just NV12 supported, so this is easy to do:
    *bpp = 12;
    return width * height * 3 / 2;
}

int camera_device_start(int camera_id)
{
    CAMERA_ID_CHECK(camera_id);
    CALL_ADAPTOR_AND_RETURN(camera_id, start());
}
int camera_device_stop(int camera_id)
{
    CAMERA_ID_CHECK(camera_id);
    CALL_ADAPTOR_AND_RETURN(camera_id, stop());
}
int camera_set_parameters(int camera_id, const Parameters& param)
{
    CAMERA_ID_CHECK(camera_id);
    CALL_ADAPTOR_AND_RETURN(camera_id, setParameters(param));
}
int camera_get_parameters(int camera_id, Parameters& param)
{
    CAMERA_ID_CHECK(camera_id);
    CALL_ADAPTOR_AND_RETURN(camera_id, getParameters(param));
}
int camera_device_config_streams(int camera_id, stream_config_t *stream_list, int /*input_fmt*/)
{
    CAMERA_ID_CHECK(camera_id);
    CALL_ADAPTOR_AND_RETURN(camera_id, configStreams(stream_list));
}
int camera_device_allocate_memory(int camera_id, camera_buffer_t *buffer)
{
    CAMERA_ID_CHECK(camera_id);
    CALL_ADAPTOR_AND_RETURN(camera_id, allocateMemory(buffer));
}
int camera_stream_dqbuf(int camera_id, int stream_id, camera_buffer_t **buffer)
{
    CALL_ADAPTOR_AND_RETURN(camera_id, dqBuf(stream_id, buffer));
}
int camera_stream_qbuf(int camera_id, int stream_id, camera_buffer_t *buffer)
{
    CALL_ADAPTOR_AND_RETURN(camera_id, qBuf(stream_id, buffer));
}

ICameraAdapter::ICameraAdapter(int cameraId) :
    mCameraId(cameraId),
    mStarted(false),
    mRequestSettings(NULL),
    mOperationMode(0)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Mutex::Autolock lock(mLock);
    CLEAR(mStream);
    string sName = to_string(cameraId);
    HAL_MODULE_INFO_SYM.common.methods->
        open((hw_module_t *)&HAL_MODULE_INFO_SYM, sName.c_str(), &mDevice);
}

ICameraAdapter::~ICameraAdapter()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
}

status_t ICameraAdapter::open()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Mutex::Autolock lock(mLock);
    // set callbacks
    camera3_callback_ops::notify = &s_notify;
    camera3_callback_ops::process_capture_result = &s_process_capture_result;
    status_t status = DOPS(mDevice)->initialize((camera3_device_t *)mDevice, this);

    if (status != OK)
        return status;

    return constructDefaultRequest();
}

status_t ICameraAdapter::close()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    // intentionally left unlocked during flush.
    // Fix if this proves to be an issue.
    DOPS(mDevice)->flush((camera3_device_t *)mDevice);
    Mutex::Autolock lock(mLock);
    mAllocatedBuffers.clear();
    mBufferMapping.clear();
    for (auto buffer : mMappedBuffers) {
        if (buffer != NULL) {
            GRALLOC_HAL_MODULE_INFO_SYM.perform(&GRALLOC_HAL_MODULE_INFO_SYM,
                    GRALLOC_UNMAP_FD,
                    buffer);
            delete (buffer);
        }
    }
    mMappedBuffers.clear();
    free_camera_metadata(mRequestSettings);
    DCOMMON(mDevice).close(mDevice);
    return OK;
}

status_t ICameraAdapter::start()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Mutex::Autolock lock(mLock);
    mStarted = true;
    while (!mPendingBuffers.empty()) {
        BufferWrapper pendingBuffer = mPendingBuffers.at(0);
        mPendingBuffers.erase(mPendingBuffers.begin());
        capture(pendingBuffer.stream_id, pendingBuffer.buffer);
    }
    return OK;
}

status_t ICameraAdapter::stop()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Mutex::Autolock lock(mLock);
    mStarted = false;
    return OK;
}

status_t ICameraAdapter::setParameters(const icamera::Parameters& param)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Mutex::Autolock lock(mLock);
    status_t status = OK;
    if (mRequestSettings == NULL) {
        LOGE("Request settings not ready yet");
        return UNKNOWN_ERROR;
    }

    struct camera_info ac2info;
    HAL_MODULE_INFO_SYM.get_camera_info(mCameraId, &ac2info);
    const camera_metadata_t *staticMeta = ac2info.static_camera_characteristics;
    if (staticMeta == NULL) {
        LOGE("No static metadata");
        return UNKNOWN_ERROR;
    }

    CameraMetadata meta(mRequestSettings); // takes metadata ownership

    // run ParameterAdapter on params to convert supported params to camera3
    // format
    int ev;
    param.getAeCompensation(ev);
    status = ParameterAdapter::convertAeComp(ev, meta, *staticMeta);
    if (status != OK)
        LOGE("ev parameter conversion failed");

    int fps;
    param.getFrameRate(fps);
    status = ParameterAdapter::convertFps(fps, meta, *staticMeta);
    if (status != OK) {
        mOperationMode = OP_MODE_DEFAULT;
        LOGE("Fps parameter conversion failed");
    } else {
        if (fps == 30) {
            mOperationMode = OP_MODE_30_FPS;
        } else if (fps > 30 && fps <= 60) {
            mOperationMode = OP_MODE_60_FPS;
        } else if (fps > 60 && fps <= 90) {
            mOperationMode = OP_MODE_90_FPS;
        } else if (fps > 90 && fps <= 120) {
            mOperationMode = OP_MODE_120_FPS;
        } else {
            mOperationMode = OP_MODE_DEFAULT;
        }
    }

    camera_video_stabilization_mode_t dvsMode;
    param.getVideoStabilizationMode(dvsMode);
    status = ParameterAdapter::convertDvs(dvsMode, meta, *staticMeta);
    if (status != OK)
        LOGE("DVS parameter conversion failed");
    if (dvsMode) {
        mOperationMode += OP_MODE_DVS;
    }

    camera_ae_mode_t aeMode;
    param.getAeMode(aeMode);
    status = ParameterAdapter::convertAeMode(aeMode, meta, *staticMeta);
    if (status != OK)
        LOGE("AE mode parameter conversion failed");

    int64_t exposureTime;
    param.getExposureTime(exposureTime);
    status = ParameterAdapter::convertExposureTime(exposureTime, meta, *staticMeta);
    if (status != OK)
        LOGE("Exposure time conversion failed");

    camera_antibanding_mode_t bandingMode;
    param.getAntiBandingMode(bandingMode);
    status = ParameterAdapter::convertBandingMode(bandingMode, meta, *staticMeta);
    if (status != OK)
        LOGE("Antibanding mode conversion failed");

    mRequestSettings = meta.release(); // restore metadata ownership (new ptr)

    return status;
}

status_t ICameraAdapter::getParameters(icamera::Parameters& param)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    // left empty intentionally since not used by gst camera src
    return OK;
}

status_t ICameraAdapter::configStreams(icamera::stream_config_t *stream_list)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Mutex::Autolock lock(mLock);

    // only support one stream for now - that's hardcoded in gstcamerasrc anyway
    if (stream_list == NULL || stream_list->num_streams != 1) {
        LOGE("bad stream config");
        return UNKNOWN_ERROR;
    }

    camera3_stream_configuration_t streamConfig;
    camera3_stream_t *streamPtrs[1];

    streamPtrs[0] = &mStream;

    streamConfig.num_streams = 1;
    streamConfig.operation_mode = mOperationMode;
    streamConfig.streams = streamPtrs;

    mStream.format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
    mStream.width = stream_list->streams[0].width;
    mStream.height = stream_list->streams[0].height;
    mStream.stream_type = CAMERA3_STREAM_OUTPUT;
    if (stream_list->streams[0].memType == V4L2_MEMORY_DMABUF) {
        mStream.usage = GRALLOC_USAGE_HW_VIDEO_ENCODER; // to force dma handle usage
        // height needs to be multiple of 16 with dma buffers
        if (mStream.height % 16 != 0)
            mStream.height = ALIGN16(mStream.height);
    } else {
        mStream.usage = GRALLOC_USAGE_HW_COMPOSER; // to force video
    }
    mStream.priv = NULL;
    mStream.max_buffers = mAllocatedBuffers.size() > 0 ?
                          mAllocatedBuffers.size() : 2;

    return DOPS(mDevice)->
            configure_streams((camera3_device_t *)mDevice, &streamConfig);
}

/* this function must be called with the mLock locked already */
status_t ICameraAdapter::mapMemory(icamera::camera_buffer_t *buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    if (buffer == NULL)
        return BAD_VALUE;

    if (buffer->s.format != V4L2_PIX_FMT_NV12) {
        LOGE("Only NV12 is supported at the moment");
        return BAD_VALUE;
    }

    camera3_stream_buffer streamBuffer;
    CLEAR(streamBuffer);

    int width = buffer->s.width;
    int height = buffer->s.height;
    int stride = width;
    size_t size = width * height * 3 / 2; /* nv12 */
    buffer_handle_t *pHandle = new buffer_handle_t;

    // map the FD
    GRALLOC_HAL_MODULE_INFO_SYM.perform(&GRALLOC_HAL_MODULE_INFO_SYM,
            GRALLOC_MAP_FD,
            pHandle,
            &buffer->addr,
            buffer->dmafd,
            size,
            width,
            height,
            stride,
            HAL_PIXEL_FORMAT_YCrCb_420_SP,
            GRALLOC_USAGE_HW_VIDEO_ENCODER);

    streamBuffer.stream = &mStream;
    streamBuffer.acquire_fence = 0;
    streamBuffer.release_fence = 0;
    streamBuffer.status = CAMERA3_BUFFER_STATUS_OK;
    streamBuffer.buffer = pHandle;

    // create a mapping for the buffers
    // so that they can be easily found during capture
    mBufferMapping[reinterpret_cast<void*>(buffer->dmafd)] = streamBuffer;

    mMappedBuffers.push_back(pHandle);

    return OK;
}

status_t ICameraAdapter::allocateMemory(icamera::camera_buffer_t *buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Mutex::Autolock lock(mLock);

    if (buffer == NULL)
        return BAD_VALUE;

    if (buffer->s.format != V4L2_PIX_FMT_NV12) {
        LOGE("Only NV12 is supported at the moment");
        return BAD_VALUE;
    }

    camera3_stream_buffer streamBuffer;
    CLEAR(streamBuffer);

    int width = buffer->s.width;
    int height = buffer->s.height;

    GraphicBuffer *gb = new GraphicBuffer(width,
                                          height,
                                          HAL_PIXEL_FORMAT_YCbCr_420_888,
                                          GRALLOC_USAGE_HW_COMPOSER);
    if (gb == NULL)
        return NO_MEMORY;

    sp<GraphicBuffer> spBuf = gb;

    status_t status = gb->initCheck();
    if (status != OK)
        return status;

    streamBuffer.stream = &mStream;
    streamBuffer.acquire_fence = 0;
    streamBuffer.release_fence = 0;
    streamBuffer.status = CAMERA3_BUFFER_STATUS_OK;
    streamBuffer.buffer = &gb->getNativeBuffer()->handle;

    GraphicBufferMapper &gbm = GraphicBufferMapper::get();

    Rect bounds(width, height);
    void *address;
    status = gbm.lock(static_cast<const native_handle*>(*streamBuffer.buffer),
                               0,
                               bounds,
                               &address);
    if (status != OK) {
        LOGE("Could not lock buffer");
        return status;
    }

    // fill the address for caller
    buffer->addr = address;

    // done, so unlock the graphic buf
    gb->unlock();
    if (status != OK) {
        LOGE("Could not unlock buffer");
        return status;
    }

    // create a mapping for the buffers
    // so that they can be easily found during capture
    mBufferMapping[address] = streamBuffer;
    // store the buffer with strong pointer for destruction later
    mAllocatedBuffers.push_back(spBuf);
    return OK;
}

status_t ICameraAdapter::dqBuf(int stream_id, icamera::camera_buffer_t **buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    Mutex::Autolock lock(mLock);

    if (mCapturedBuffers.empty()) {
        mCondition.waitRelative(mLock, ONE_SECOND);
        if (mCapturedBuffers.empty()) {
            LOGE("capture timed out");
            return UNKNOWN_ERROR;
        }
    }

    BufferWrapper buf = mCapturedBuffers.at(0);
    mCapturedBuffers.erase(mCapturedBuffers.begin());

    *buffer = buf.buffer;

    return OK;
}

status_t ICameraAdapter::qBuf(int stream_id, icamera::camera_buffer_t *buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    Mutex::Autolock lock(mLock);
    if (buffer == NULL) {
        LOGE("null buffer");
        return BAD_VALUE;
    }

    if (mStarted) {
        return capture(stream_id, buffer);
    } else {
        BufferWrapper pendingBuffer;
        pendingBuffer.stream_id = stream_id;
        pendingBuffer.buffer = buffer;
        mPendingBuffers.push_back(pendingBuffer);
    }
    return OK;
}

/*
 * Constructs the default request settings and stores them into
 * mRequestSettings. Does nothing if the default request is non-null.
 *
 * this function must be called with the mLock locked already
 */
status_t ICameraAdapter::constructDefaultRequest()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = OK;
    if (mRequestSettings == NULL) {
        const camera_metadata_t *metadata = DOPS(mDevice)->
                construct_default_request_settings((camera3_device_t *)mDevice,
                                                   CAMERA3_TEMPLATE_PREVIEW);
        if (metadata == NULL) {
            LOGE("couldn't create default request");
            return UNKNOWN_ERROR;
        }
        CameraMetadata cameraMetadata;
        cameraMetadata = metadata; // clone
        int32_t requestId = CAMERA3_TEMPLATE_PREVIEW;
        cameraMetadata.update(ANDROID_REQUEST_ID, &requestId, 1);
        mRequestSettings = cameraMetadata.release(); // assign the clone to member
    }
    return status;
}

/* this function must be called with the mLock locked already */
status_t ICameraAdapter::capture(int stream_id, icamera::camera_buffer_t *buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = OK;

    map<void *, camera3_stream_buffer>::iterator mapping;

    // try handling dma buffers first
    if (buffer->dmafd > 0) {
        // try to find the mapped address, if any
        mapping = mBufferMapping.find(reinterpret_cast<void*>(buffer->dmafd));
        if (mapping == mBufferMapping.end()) {
            // mapping not found -> do mmap for the buffer
            status = mapMemory(buffer);
            mapping = mBufferMapping.find(reinterpret_cast<void*>(buffer->dmafd));
        }
    } else {
        // we should always have a buffer here
        mapping = mBufferMapping.find(buffer->addr);
    }

    if (status == OK && mapping != mBufferMapping.end()) {
        static int frame_number = 0;
        camera3_capture_request_t request;
        camera3_stream_buffer streamBuffer = mapping->second;
        // just one stream supported ATM in camerasrc,
        // and the qbuf API of icamera isn't much better as it does not offer
        // possibility to send more than one buffer at a time, so num out = 1
        request.num_output_buffers = 1;
        request.input_buffer = NULL;
        request.settings = mRequestSettings;
        request.frame_number = frame_number++;
        request.output_buffers = &streamBuffer;

        BufferWrapper queuedBuffer;
        CLEAR(queuedBuffer);
        queuedBuffer.stream_id = stream_id;
        queuedBuffer.buffer = buffer;
        mQueuedBuffers.push_back(queuedBuffer);

        mLock.unlock(); // process_capture_request may block, so we must unlock

        status = DOPS(mDevice)->
                process_capture_request((camera3_device_t *)mDevice, &request);
        if (status != OK) {
            LOGE("capture failed");
        }
        mLock.lock(); // lock was held when this function is called, so lock again
    } else {
        LOGE("Capture error. Buffer fd is this: %d addr: %x", buffer->dmafd, buffer->addr);
        return UNKNOWN_ERROR;
    }

    return status;
}

void ICameraAdapter::processCaptureResult(const camera3_capture_result_t *result)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    Mutex::Autolock lock(mLock);

    // first find the result structure or create a new one, if it is not found
    map<uint32_t, Result>::iterator captureResult =
            mResults.find(result->frame_number);
    if (captureResult == mResults.end()) {
        Result resultStruct;
        CLEAR(resultStruct);
        pair<map<uint32_t, Result>::iterator,bool> ret;
        mResults[result->frame_number] = resultStruct;
        captureResult = mResults.find(result->frame_number);
    }

    if (result->num_output_buffers == 0) {
        // this is the metadata, so fetch the necessary parts from it
        // (just timestamp, for now)
        camera_metadata_ro_entry entry;
        find_camera_metadata_ro_entry(result->result,
                                      ANDROID_SENSOR_TIMESTAMP,
                                      &entry);
        if (entry.count == 1) {
            captureResult->second.timestamp = entry.data.i64[0];
        } else {
            LOGW("No shutter timestamp in result metadata");
        }

        captureResult->second.metadataDone = true;
    }

    // just one stream supported at the moment, hence checking for
    // output buffer count of one
    if (result->num_output_buffers == 1) {
        const camera3_stream_buffer_t &c3Buf = result->output_buffers[0];
        int width = c3Buf.stream->width;
        int height = c3Buf.stream->height;
        buffer_handle_t *pHandle = c3Buf.buffer;

        GraphicBufferMapper &gbm = GraphicBufferMapper::get();
        Rect bounds(width,height);
        void *address;
        status_t status = gbm.lock(static_cast<const native_handle*>(*pHandle),
                                                                     0,
                                                                     bounds,
                                                                     &address);

        BufferWrapper queuedBuffer = mQueuedBuffers.at(0);
        if (queuedBuffer.buffer->addr != address) {
            LOGE("wrong address %p in result buffer, expected %p - "
                 "sequence mismatch maybe?",
                 address, queuedBuffer.buffer->addr);
        }
        queuedBuffer.buffer->addr = address;
        queuedBuffer.buffer->sequence = result->frame_number;

        captureResult->second.buffersDone = true;
    }

    // once both metadata and buffers are received, we are done with the results
    // and can timestamp the buffer and return it to icamera user.
    if (captureResult->second.buffersDone && captureResult->second.metadataDone) {
        BufferWrapper queuedBuffer = mQueuedBuffers.at(0);
        mQueuedBuffers.erase(mQueuedBuffers.begin());
        queuedBuffer.buffer->timestamp = captureResult->second.timestamp;
        mCapturedBuffers.push_back(queuedBuffer);
        mResults.erase(captureResult);
        mCondition.signal();
    }
}

/* static functions for callback function pointers */
void ICameraAdapter::s_notify(const struct camera3_callback_ops *ops,
              const camera3_notify_msg_t *msg)
{
    if (msg == NULL) {
        LOGE("Received null notification message");
        return;
    }

    switch (msg->type) {
        case CAMERA3_MSG_ERROR:
            LOGE("Received error code %d for frame %d",
                 msg->message.error.error_code,
                 msg->message.error.frame_number);
            break;
        case CAMERA3_MSG_SHUTTER:
            LOG2("Shutter received for frame %d timestamp %" PRId64 "",
                 msg->message.shutter.frame_number,
                 msg->message.shutter.timestamp);
            break;
        default:
            LOGE("Received unknown message type %d", msg->type);
            break;
    }
}
void ICameraAdapter::s_process_capture_result(const struct camera3_callback_ops *ops,
                                              const camera3_capture_result_t *result)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    ICameraAdapter *adapter =
                const_cast<ICameraAdapter*>(static_cast<const ICameraAdapter*>(ops));

    adapter->processCaptureResult(result);
}

} // namespace icamera
