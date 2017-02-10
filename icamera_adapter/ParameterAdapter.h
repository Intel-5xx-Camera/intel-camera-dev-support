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

#ifndef _PARAMETERADAPTER_H_
#define _PARAMETERADAPTER_H_

#include "camera/CameraMetadata.h"
#include "Errors.h"
#include "Parameters.h"

// todo adapter between icamera and camera3 parameters
namespace icamera {
namespace ParameterAdapter {
    status_t convertAeComp(int ev,
                           android::CameraMetadata &outputMetadata,
                           const camera_metadata_t &staticMetadata);
    status_t convertFps(int fps,
                        android::CameraMetadata &outputMetadata,
                        const camera_metadata_t &staticMetadata);
    status_t convertDvs(camera_video_stabilization_mode_t mode,
                        android::CameraMetadata &outputMetadata,
                        const camera_metadata_t &staticMetadata);
    status_t convertAeMode(camera_ae_mode_t aeMode,
                           android::CameraMetadata &outputMetadata,
                           const camera_metadata_t &staticMetadata);
    status_t convertExposureTime(int64_t exposureTime,
                                 android::CameraMetadata &outputMetadata,
                                 const camera_metadata_t &staticMetadata);
    status_t convertBandingMode(camera_antibanding_mode_t bandingMode,
                                android::CameraMetadata &outputMetadata,
                                const camera_metadata_t &staticMetadata);
    // add mode parameter conversions here
} // namespace ParameterAdapter
} // namespace icamera

#endif /* _PARAMETERADAPTER_H_ */
