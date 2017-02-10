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
// this file implements the libcamhal Parameters API with stubs, except for
// ev shift which is implemented as an example
#define LOG_TAG "Camera_Parameters"

#include "RWLock.h"
#include "Parameters.h"
#include "camera/CameraMetadata.h"
#include "LogHelper.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

namespace icamera {

Parameters::Parameters() :
        mMetadata(new CameraMetadata()),
        mRwLock(new RWLock()),
        mEv(0),
        mFps(30),
        mDvsMode(VIDEO_STABILIZATION_MODE_OFF),
        mAeMode(AE_MODE_AUTO),
        mExposureTime(0),
        mBandingMode(ANTIBANDING_MODE_AUTO)
{}

Parameters::Parameters(const Parameters& other) :
        mMetadata(new CameraMetadata(*other.mMetadata)),
        mRwLock(new RWLock()),
        mEv(other.mEv),
        mFps(other.mFps),
        mDvsMode(other.mDvsMode),
        mAeMode(other.mAeMode),
        mExposureTime(other.mExposureTime),
        mBandingMode(other.mBandingMode)
{}

Parameters& Parameters::operator=(const Parameters& other)
{
    // Release the old buffer and assign to a new one
    AutoWMutex wl(*mRwLock);
    if (&other != this) {
        *mMetadata = *other.mMetadata;
        mEv = other.mEv;
        mFps = other.mFps;
        mDvsMode = other.mDvsMode;
        mAeMode = other.mAeMode;
        mExposureTime = other.mExposureTime;
        mBandingMode = other.mBandingMode;
    }
    return *this;
}

Parameters::~Parameters()
{
    if (mMetadata) {
        delete mMetadata;
        mMetadata = NULL;
    }
    delete mRwLock;
    mRwLock = NULL;
}

int Parameters::setAeMode(camera_ae_mode_t aeMode)
{
    AutoWMutex wl(*mRwLock);
    mAeMode = aeMode;
    return OK;
}

int Parameters::getAeMode(camera_ae_mode_t& aeMode) const
{
    AutoRMutex rl(*mRwLock);
    aeMode = mAeMode;
    return OK;
}

int Parameters::setAeRegions(camera_window_list_t aeRegions)
{
    return OK;
}

int Parameters::getAeRegions(camera_window_list_t& aeRegions) const
{
    return OK;
}

int Parameters::setExposureTime(int64_t exposureTime)
{
    AutoWMutex wl(*mRwLock);
    mExposureTime = exposureTime;
    return OK;
}

int Parameters::getExposureTime(int64_t& exposureTime) const
{
    AutoRMutex rl(*mRwLock);
    exposureTime = mExposureTime;
    return OK;
}

int Parameters::setSensitivityGain(float gain)
{
    return OK;
}

int Parameters::getSensitivityGain(float& gain) const
{
    return OK;
}

int Parameters::setAeCompensation(int ev)
{
    AutoWMutex wl(*mRwLock);
    mEv = ev;
    return OK;
}

int Parameters::getAeCompensation(int& ev) const
{
    AutoRMutex rl(*mRwLock);
    ev = mEv;
    return OK;
}

int Parameters::setFrameRate(int fps)
{
    AutoWMutex wl(*mRwLock);
    mFps = fps;
    return OK;
}

int Parameters::getFrameRate(int& fps) const
{
    AutoRMutex rl(*mRwLock);
    fps = mFps;
    return OK;
}

int Parameters::setAntiBandingMode(camera_antibanding_mode_t bandingMode)
{
    AutoWMutex wl(*mRwLock);
    mBandingMode = bandingMode;
    return OK;
}

int Parameters::getAntiBandingMode(camera_antibanding_mode_t& bandingMode) const
{
    AutoRMutex rl(*mRwLock);
    bandingMode = mBandingMode;
    return OK;
}

int Parameters::setAwbMode(camera_awb_mode_t awbMode)
{
    return OK;
}

int Parameters::getAwbMode(camera_awb_mode_t& awbMode) const
{
    return OK;
}

int Parameters::setAwbCctRange(camera_range_t cct)
{
    return OK;
}

int Parameters::getAwbCctRange(camera_range_t& cct) const
{
    return OK;
}

int Parameters::setAwbGains(camera_awb_gains_t awbGains)
{
    return OK;
}

int Parameters::getAwbGains(camera_awb_gains_t& awbGains) const
{
    return OK;
}

int Parameters::setAwbGainShift(camera_awb_gains_t awbGainShift)
{
    return OK;
}

int Parameters::getAwbGainShift(camera_awb_gains_t& awbGainShift) const
{
    return OK;
}

int Parameters::setAwbWhitePoint(camera_coordinate_t whitePoint)
{
    return OK;
}

int Parameters::getAwbWhitePoint(camera_coordinate_t& whitePoint) const
{
    return OK;
}

int Parameters::setColorTransform(camera_color_transform_t colorTransform)
{
    return OK;
}

int Parameters::getColorTransform(camera_color_transform_t& colorTransform) const
{
    return OK;
}

int Parameters::setAwbRegions(camera_window_list_t awbRegions)
{
    return OK;
}

int Parameters::getAwbRegions(camera_window_list_t& awbRegions) const
{
    return OK;
}

int Parameters::setNrMode(camera_nr_mode_t nrMode)
{
    return OK;
}

int Parameters::getNrMode(camera_nr_mode_t& nrMode) const
{
    return OK;
}

int Parameters::setNrLevel(camera_nr_level_t level)
{
    return OK;
}

int Parameters::getNrLevel(camera_nr_level_t& level) const
{
    return OK;
}

int Parameters::setIrisMode(camera_iris_mode_t irisMode)
{
    return OK;
}

int Parameters::getIrisMode(camera_iris_mode_t& irisMode)
{
    return OK;
}

int Parameters::setIrisLevel(int level)
{
    return OK;
}

int Parameters::getIrisLevel(int& level)
{
    return OK;
}

int Parameters::setWdrMode(camera_wdr_mode_t wdrMode)
{
    return OK;
}

int Parameters::getWdrMode(camera_wdr_mode_t& wdrMode) const
{
    return OK;
}

int Parameters::setWdrLevel(int level)
{
    return OK;
}

int Parameters::getWdrLevel(int& level) const
{
    return OK;
}

int Parameters::setSceneMode(camera_scene_mode_t sceneMode)
{
    return OK;
}

int Parameters::getSceneMode(camera_scene_mode_t& sceneMode) const
{
    return OK;
}

int Parameters::setWeightGridMode(camera_weight_grid_mode_t weightGridMode)
{
    return OK;
}

int Parameters::getWeightGridMode(camera_weight_grid_mode_t& weightGridMode) const
{
    return OK;
}

int Parameters::setBlcAreaMode(camera_blc_area_mode_t blcAreaMode)
{
    return OK;
}

int Parameters::getBlcAreaMode(camera_blc_area_mode_t& blcAreaMode) const
{
    return OK;
}

int Parameters::setFpsRange(camera_range_t fpsRange)
{
    return OK;
}

int Parameters::getFpsRange(camera_range_t& fpsRange) const
{
    return OK;
}

int Parameters::setImageEnhancement(camera_image_enhancement_t effects)
{
    return OK;
}

int Parameters::getImageEnhancement(camera_image_enhancement_t& effects) const
{
    return OK;
}

int Parameters::setDeinterlaceMode(camera_deinterlace_mode_t deinterlaceMode)
{
    return OK;
}

int Parameters::getDeinterlaceMode(camera_deinterlace_mode_t &deinterlaceMode) const
{
    return OK;
}

int Parameters::getSupportedFpsRange(camera_range_array_t& ranges) const
{
    return OK;
}

int Parameters::getSupportedStreamConfig(stream_array_t& config) const
{
    config.clear();
    AutoRMutex rl(*mRwLock);
    camera_metadata_ro_entry_t entry = getConstPtr()->find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    const int streamConfMemberNum = STREAM_MEMBER_NUM;
    if (entry.count == 0 || entry.count % streamConfMemberNum != 0) {
        return NAME_NOT_FOUND;
    }

    stream_t s;
    CLEAR(s);

    for (size_t i = 0; i < entry.count; i += streamConfMemberNum) {
        s.format = entry.data.i32[i];
        s.width = entry.data.i32[i + 1];
        s.height = entry.data.i32[i + 2];
        s.field = entry.data.i32[i + 3];
        s.stride = s.width; // fixme
        //s.stride = CameraUtils::getStride(s.format, s.width);
        s.size = s.stride * s.height * 3 / 2; // fixme
        //s.size = CameraUtils::getFrameSize(s.format, s.width, s.height, s.field);
        config.push_back(s);
    }
    return OK;
}

int Parameters::getSupportedFeatures(camera_features_list_t& features) const
{
    return OK;
}

int Parameters::getAeCompensationRange(camera_range_t& evRange) const
{
    return OK;
}

int Parameters::getAeCompensationStep(camera_rational_t& evStep) const
{
    return OK;
}

int Parameters::getSupportedAeExposureTimeRange(std::vector<camera_ae_exposure_time_range_t> & etRanges) const
{
    return OK;
}

int Parameters::getSupportedAeGainRange(std::vector<camera_ae_gain_range_t>& gainRanges) const
{
    return OK;
}

int Parameters::setAeConvergeSpeed(camera_converge_speed_t speed)
{
    return OK;
}

int Parameters::getAeConvergeSpeed(camera_converge_speed_t& speed) const
{
    return OK;
}

int Parameters::setAwbConvergeSpeed(camera_converge_speed_t speed)
{
    return OK;
}

int Parameters::getAwbConvergeSpeed(camera_converge_speed_t& speed) const
{
    return OK;
}

int Parameters::setAeConvergeSpeedMode(camera_converge_speed_mode_t mode)
{
    return OK;
}

int Parameters::getAeConvergeSpeedMode(camera_converge_speed_mode_t& mode) const
{
    return OK;
}

int Parameters::setAwbConvergeSpeedMode(camera_converge_speed_mode_t mode)
{
    return OK;
}

int Parameters::getAwbConvergeSpeedMode(camera_converge_speed_mode_t& mode) const
{
    return OK;
}

int Parameters::setCustomAicParam(const void* data, unsigned int length)
{
    return OK;
}

int Parameters::getCustomAicParam(void* data, unsigned int* length) const
{
    return OK;
}

int Parameters::setAeDistributionPriority(camera_ae_distribution_priority_t priority)
{
    return OK;
}

int Parameters::getAeDistributionPriority(camera_ae_distribution_priority_t& priority) const
{
    return OK;
}

int Parameters::setYuvColorRangeMode(camera_yuv_color_range_mode_t colorRange)
{
    return OK;
}

int Parameters::getYuvColorRangeMode(camera_yuv_color_range_mode_t& colorRange) const
{
    return OK;
}

void Parameters::merge(const Parameters& other)
{
    merge(other.mMetadata);
}

void Parameters::merge(const CameraMetadata* metadata)
{
    HAL_TRACE_CALL(2);
    if (metadata == NULL || metadata->isEmpty()) {
        // Nothing needs to be merged
        return;
    }

    AutoWMutex wl(*mRwLock);
    const camera_metadata_t* src = const_cast<CameraMetadata*>(metadata)->getAndLock();
    size_t count = metadata->entryCount();
    camera_metadata_ro_entry_t entry;
    for (size_t i = 0; i < count; i++) {
        CLEAR(entry);
        if (get_camera_metadata_ro_entry(src, i, &entry) != OK) {
            continue;
        }
        switch (entry.type) {
        case TYPE_BYTE:
            mMetadata->update(entry.tag, entry.data.u8, entry.count);
            break;
        case TYPE_INT32:
            mMetadata->update(entry.tag, entry.data.i32, entry.count);
            break;
        case TYPE_FLOAT:
            mMetadata->update(entry.tag, entry.data.f, entry.count);
            break;
        case TYPE_INT64:
            mMetadata->update(entry.tag, entry.data.i64, entry.count);
            break;
        case TYPE_DOUBLE:
            mMetadata->update(entry.tag, entry.data.d, entry.count);
            break;
        case TYPE_RATIONAL:
            mMetadata->update(entry.tag, entry.data.r, entry.count);
            break;
        default:
            LOGW("Invalid entry type, should never happen");
            break;
        }
    }
    const_cast<CameraMetadata*>(metadata)->unlock(src);
}

int Parameters::setRegions(camera_window_list_t regions, int tag)
{
    return OK;
}

int Parameters::getRegions(camera_window_list_t& regions, int tag) const
{
    return OK;
}

int Parameters::setJpegQuality(int quality)
{
    return OK;
}

int Parameters::getJpegQuality(int &quality) const
{
    return OK;
}

int Parameters::setJpegThumbnailQuality(int quality)
{
    return OK;
}

int Parameters::getJpegThumbnailQuality(int &quality) const
{
    return OK;
}

int Parameters::setJpegRotation(int rotation)
{
    return OK;
}

int Parameters::getJpegRotation(int &rotation) const
{
    return OK;
}

int Parameters::setJpegGpsCoordinates(double *coordinates)
{
    return OK;
}

int Parameters::getJpegGpsLatitude(double &latitude) const
{
    return OK;
}

int Parameters::getJpegGpsLongitude(double &longitude) const
{
    return OK;
}

int Parameters::getJpegGpsAltitude(double &altitude) const
{
    return OK;
}

int Parameters::setJpegGpsTimeStamp(int64_t  timestamp)
{
    return OK;
}

int Parameters::getJpegGpsTimeStamp(int64_t &timestamp) const
{
    return OK;
}

int Parameters::setJpegGpsProcessingMethod(int processMethod)
{
    return OK;
}

int Parameters::getJpegGpsProcessingMethod(int &processMethod) const
{
    return OK;
}

int Parameters::setImageEffect(camera_effect_mode_t  effect)
{
    return OK;
}

int Parameters::getImageEffect(camera_effect_mode_t &effect) const
{
    return OK;
}

int Parameters::setVideoStabilizationMode(camera_video_stabilization_mode_t mode)
{
    AutoWMutex wl(*mRwLock);
    mDvsMode = mode;
    return OK;
}

int Parameters::getVideoStabilizationMode(camera_video_stabilization_mode_t &mode) const
{
    AutoRMutex rl(*mRwLock);
    mode = mDvsMode;
    return OK;
}

//User can set environment and then call api to update the debug level.
int Parameters::updateDebugLevel()
{
    icamera::LogHelper::setDebugLevel();
    return OK;
}

} // end of namespace icamera
