/*
 * Copyright 2016 Intel Corporation * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dlfcn.h>
#include <gst/gst.h>
#include "gst_pvl_face_detection_filter.h"
#include "gst_pvl_face_recognition_filter.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
    gboolean ret = FALSE;

    {
        const char* PVL_FILTER_NAME = "pvl_face_detection";
        gboolean ret_t = gst_element_register (plugin, PVL_FILTER_NAME, GST_RANK_NONE, GST_TYPE_PVL_FD_FILTER);
        GST_DEBUG ("%s ret: %d\n", PVL_FILTER_NAME, ret_t);
        ret |= ret_t;
    }

    {
        const char* PVL_FILTER_NAME = "pvl_face_recognition";
        gboolean ret_t = gst_element_register (plugin, PVL_FILTER_NAME, GST_RANK_NONE, GST_TYPE_PVL_FR_FILTER);
        GST_DEBUG ("%s ret: %d\n", PVL_FILTER_NAME, ret_t);
        ret |= ret_t;
    }

    return ret;
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    pvl_filter,
    "PVL Video filters",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://www.intel.com"
)

