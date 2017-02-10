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

#include "test_stream_factory.h"
#include <hardware/camera3.h>
#include "camera_metadata_hidden.h"


namespace Parmz = Parameterization;

namespace TestStreamFactory {

std::vector<Parmz::TestParam> getImx135SupportedStreams(int camId)
{
    std::vector<Parmz::TestParam> streams;
    streams.push_back(Parmz::TestParam(camId, 4160, 3112, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 640, 480,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 4160, 3112, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 640, 480,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 640, 480,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));

    return streams;
}

std::vector<Parmz::TestParam> getImx214SupportedStreams(int camId)
{
    std::vector<Parmz::TestParam> streams;

    streams.push_back(Parmz::TestParam(camId, 4096, 3104, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 4096, 2304, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 3840, 2160, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 640,  360,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 4096, 3104, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 4096, 2304, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 3840, 2160, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 640,  360,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 3840, 2160, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 640,  360,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    return streams;
}

std::vector<Parmz::TestParam> getOv5670SupportedStreams(int camId)
{
    std::vector<Parmz::TestParam> streams;

    streams.push_back(Parmz::TestParam(camId, 2560, 1920, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 2560, 1440, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1280, 720, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 2560, 1920, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 2560, 1440, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    return streams;
}

std::vector<Parmz::TestParam> getImx230SupportedStreams(int camId)
{
    std::vector<Parmz::TestParam> streams;
    streams.push_back(Parmz::TestParam(camId, 5248, 3936, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 5248, 2952, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 320,  240,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 5248, 3936, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 5248, 2952, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 320,  240,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 5248, 3936, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 5248, 2952, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 320,  240,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    return streams;
}

std::vector<Parmz::TestParam> getOv8858SupportedStreams(int camId)
{
    std::vector<Parmz::TestParam> streams;
    streams.push_back(Parmz::TestParam(camId, 3264, 2448, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 3264, 1836, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 320,  240,  HAL_PIXEL_FORMAT_BLOB));
    streams.push_back(Parmz::TestParam(camId, 3264, 2448, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 3264, 1836, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 320,  240,  HAL_PIXEL_FORMAT_YCbCr_420_888));
    streams.push_back(Parmz::TestParam(camId, 3264, 2448, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 3264, 1836, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 1920, 1080, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 1280, 720,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 640,  480,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    streams.push_back(Parmz::TestParam(camId, 320,  240,  HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED));
    return streams;
}

} // namespace TestStreamFactory
