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

#define LOG_TAG "ParameterAdapter"

#include "LogHelper.h"
#include "ParameterAdapter.h"
#include "hardware/camera3.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

using android::CameraMetadata;

namespace icamera {
namespace ParameterAdapter {
/**
 * \param [IN]  ev value
 * \param [OUT] outputMetadata where converted value is written
 * \param [IN]  staticMetadata of the current camera in use
 * \return status value. OK if value could be converted and written.
 */
    status_t convertAeComp(int ev,
                           CameraMetadata &outputMetadata,
                           const camera_metadata_t &staticMetadata) {
        camera_metadata_ro_entry stepSizeEntry;
        CLEAR(stepSizeEntry);
        int ret = find_camera_metadata_ro_entry(&staticMetadata,
                                                ANDROID_CONTROL_AE_COMPENSATION_STEP,
                                                &stepSizeEntry);
        if (ret != OK ||
            stepSizeEntry.count != 1 ||
            stepSizeEntry.data.r->denominator == 0 ||
            stepSizeEntry.data.r->numerator == 0) {
            LOGE("No step size in static metadata");
            return UNKNOWN_ERROR;
        }

        camera_metadata_ro_entry evRangeEntry;
        CLEAR(evRangeEntry);
        ret = find_camera_metadata_ro_entry(&staticMetadata,
                                            ANDROID_CONTROL_AE_COMPENSATION_RANGE,
                                            &evRangeEntry);

        if (ret != OK || evRangeEntry.count != 2) {
            LOGE("No ev range in static metadata");
            return UNKNOWN_ERROR;
        }

        int32_t stepCount = ev * stepSizeEntry.data.r->denominator /
                stepSizeEntry.data.r->numerator;

        if (stepCount > evRangeEntry.data.i32[1]) {
            LOGW("ev value above max, capping it");
            stepCount = evRangeEntry.data.i32[1];
        }

        if (stepCount < evRangeEntry.data.i32[0]) {
            LOGW("ev value below min, capping it");
            stepCount = evRangeEntry.data.i32[0];
        }

        status_t status = outputMetadata.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                                &stepCount,
                                                1);

        return status;
    }

/**
 * \param [IN]  fps value
 * \param [OUT] outputMetadata where converted value is written
 * \param [IN]  staticMetadata of the current camera in use
 * \return status value. OK if value could be converted and written.
 */
    status_t convertFps(int fps,
                        CameraMetadata &outputMetadata,
                        const camera_metadata_t &staticMetadata) {

        camera_metadata_ro_entry fpsRangesEntry;
        CLEAR(fpsRangesEntry);
        int ret = find_camera_metadata_ro_entry(&staticMetadata,
                                                ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
                                                &fpsRangesEntry);

        if (ret != OK) {
            LOGE("Error getting FPS ranges from static metadata");
            return UNKNOWN_ERROR;
        }

        if ((fpsRangesEntry.count % 2) || (fpsRangesEntry.count < 2)) {
            LOGE("No valid FPS ranges in static metadata");
            return UNKNOWN_ERROR;
        }

        // supported fps ranges are in (min, max) pairs
        // find the suitable range, prioritize fixed range
        int32_t lowFps = 0;
        int32_t highFps = 0;
        int32_t index = -1;
        for (int i = 0; i < fpsRangesEntry.count / 2; i++) {
            lowFps = fpsRangesEntry.data.i32[2 * i];
            highFps = fpsRangesEntry.data.i32[2 * i + 1];

            // found suitable fixed range, stop searching
            if (fps == lowFps && fps == highFps) {
                index = 2*i;
                break;
            }

            // suitable variable range found
            if (fps >= lowFps && fps <= highFps) {
                index = 2*i;
            }
        }

        if (index == -1) {
            LOGW("Suitable range not found for fps setting %d, using default", fps);
            index = 0;
        }

        // set target range
        int32_t fpsRange[2];
        fpsRange[0] = fpsRangesEntry.data.i32[index];
        fpsRange[1] = fpsRangesEntry.data.i32[index + 1];
        LOG1("Setting target fps range [%d, %d]", fpsRange[0], fpsRange[1]);

        status_t status = outputMetadata.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                                                fpsRange,
                                                2);

        return status;
    }
/**
 * \param [IN]  DVS value
 * \param [OUT] outputMetadata where converted value is written
 * \param [IN]  staticMetadata of the current camera in use
 * \return status value. OK if value could be converted and written.
 */
    status_t convertDvs(camera_video_stabilization_mode_t mode,
                        CameraMetadata &outputMetadata,
                        const camera_metadata_t &staticMetadata) {

        camera_metadata_ro_entry dvsModes;
        CLEAR(dvsModes);
        int ret = find_camera_metadata_ro_entry(&staticMetadata,
                                                ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
                                                &dvsModes);

        if (ret != OK) {
            LOGE("Error getting DVS modes from static metadata");
            return UNKNOWN_ERROR;
        }

        bool dvsSupported = false;
        for (int i = 0; i < dvsModes.count; i++) {
            if (dvsModes.data.u8[i] == ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON) {
                dvsSupported = true;
                break;
            }
        }

        uint8_t dvsMode = VIDEO_STABILIZATION_MODE_OFF;
        if (dvsSupported && (mode == VIDEO_STABILIZATION_MODE_ON)) {
            dvsMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON;
        }
        LOG1("DVS mode: %u", dvsMode);

        status_t status = outputMetadata.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE,
                                                &dvsMode,
                                                1);
        return status;
    }

/**
 * \param [IN]  AE mode
 * \param [OUT] outputMetadata where converted value is written
 * \param [IN]  staticMetadata of the current camera in use
 * \return status value. OK if value could be converted and written.
 */
    status_t convertAeMode(camera_ae_mode_t mode,
                           CameraMetadata &outputMetadata,
                           const camera_metadata_t &staticMetadata) {

        uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
        if (mode == AE_MODE_MANUAL)
            aeMode = ANDROID_CONTROL_AE_MODE_OFF;

        LOG1("AE mode: %d", aeMode);
        status_t status = outputMetadata.update(ANDROID_CONTROL_AE_MODE,
                                                &aeMode,
                                                1);

        return status;
    }

/**
 * \param [IN]  exposure time
 * \param [OUT] outputMetadata where converted value is written
 * \param [IN]  staticMetadata of the current camera in use
 * \return status value. OK if value could be converted and written.
 */
    status_t convertExposureTime(int64_t exposureTime,
                                 CameraMetadata &outputMetadata,
                                 const camera_metadata_t &staticMetadata) {

        camera_metadata_ro_entry etRange;
        CLEAR(etRange);
        int ret = find_camera_metadata_ro_entry(&staticMetadata,
                                                ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
                                                &etRange);
        if (ret != OK) {
            LOGE("Error getting ET range from static metadata");
            return UNKNOWN_ERROR;
        }

        if (etRange.count != 2) {
            LOGE("No valid ET range in static metadata");
            return NAME_NOT_FOUND;
        }

        int64_t etMin = etRange.data.i64[0];
        int64_t etMax = etRange.data.i64[1];

        // HAL expects exposure time in nanoseconds
        int64_t exposureTimeNs = exposureTime * 1000;

        if (exposureTimeNs < etMin) {
            exposureTimeNs = etMin;
            LOGW("ET value below minimum, capping it");
        } else if (exposureTimeNs > etMax) {
            exposureTimeNs = etMax;
            LOGW("ET value above maximum, capping it");
        }

        LOG1("exposureTime (ns): %ld. Supported range [%ld, %ld]", exposureTimeNs, etMin, etMax);

        status_t status = outputMetadata.update(ANDROID_SENSOR_EXPOSURE_TIME,
                                                &exposureTimeNs,
                                                1);

        return status;
    }

/**
 * \param [IN]  antibanding mode
 * \param [OUT] outputMetadata where converted value is written
 * \param [IN]  staticMetadata of the current camera in use
 * \return status value. OK if value could be converted and written.
 */
    status_t convertBandingMode(camera_antibanding_mode_t bandingMode,
                                CameraMetadata &outputMetadata,
                                const camera_metadata_t &staticMetadata) {

        camera_metadata_ro_entry modes;
        CLEAR(modes);
        int ret = find_camera_metadata_ro_entry(&staticMetadata,
                                                ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
                                                &modes);
        if (ret != OK) {
            LOGE("Error getting antibanding modes from static metadata");
            return UNKNOWN_ERROR;
        }

        uint8_t androidMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;

        switch (bandingMode) {
        case ANTIBANDING_MODE_AUTO:
            androidMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
            break;
        case ANTIBANDING_MODE_50HZ:
            androidMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ;
            break;
        case ANTIBANDING_MODE_60HZ:
            androidMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ;
            break;
        case ANTIBANDING_MODE_OFF:
            androidMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF;
            break;
        }

        bool modeSupported = false;
        for (int i = 0; i < modes.count; i++) {
            if (modes.data.u8[i] == androidMode) {
                modeSupported = true;
                break;
            }
        }

        if (!modeSupported) {
            LOGW("Requested antibanding mode (%u) not supported, using AUTO", androidMode);
            androidMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
        }
        LOG1("Antibanding mode: %u", androidMode);

        status_t status = outputMetadata.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE,
                                                &androidMode,
                                                1);
        return status;
    }

    // add mode parameter conversions here
} // namespace ParameterAdapter
} // namespace icamera
