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
#  include <config.h>
#endif

#include "gst_pvl_util.h"

pvl_image_format
get_pvl_format (GstVideoFormat format)
{
    switch (format) {
        case GST_VIDEO_FORMAT_NV21:
            return pvl_image_format_nv21;
        case GST_VIDEO_FORMAT_NV12:
            return pvl_image_format_nv12;
        case GST_VIDEO_FORMAT_YV12:
            return pvl_image_format_yv12;
        case GST_VIDEO_FORMAT_YUY2:
            return pvl_image_format_yuy2;
        case GST_VIDEO_FORMAT_GRAY8:
            return pvl_image_format_gray;
        case GST_VIDEO_FORMAT_RGBx:
        case GST_VIDEO_FORMAT_RGBA:
            return pvl_image_format_rgba32;

        default:
            return pvl_image_format_gray;
    }
}

void
wrap_pvl_image (pvl_image *dst, GstVideoFrame *src, guint orientation)
{
    GstVideoFormat gst_format = GST_VIDEO_FRAME_FORMAT (src);

    switch (gst_format) {
        case GST_VIDEO_FORMAT_NV21:
        case GST_VIDEO_FORMAT_NV12:
        case GST_VIDEO_FORMAT_YV12:
        case GST_VIDEO_FORMAT_GRAY8:
        case GST_VIDEO_FORMAT_YUY2:
        case GST_VIDEO_FORMAT_RGBx:
        case GST_VIDEO_FORMAT_RGBA:
            dst->data = (uint8_t*) GST_VIDEO_FRAME_PLANE_DATA (src, 0);
            dst->stride = GST_VIDEO_FRAME_PLANE_STRIDE (src, 0);
            dst->width = GST_VIDEO_FRAME_WIDTH (src);
            dst->height = GST_VIDEO_FRAME_HEIGHT (src);
            dst->size = GST_VIDEO_FRAME_SIZE (src);
            dst->rotation = orientation;
            dst->format = get_pvl_format (gst_format);
            break;

        default:
            break;
    }
}

gint
get_pvl_orientation (GstEvent *event)
{
    GstTagList *taglist;
    gchar *orientation;
    gint ret = -1;

    gst_event_parse_tag (event, &taglist);

    if (gst_tag_list_get_string (taglist, "image-orientation", &orientation)) {
        if (!g_strcmp0 ("rotate-0", orientation))
            ret = 0;
        else if (!g_strcmp0 ("rotate-90", orientation))
            ret = 270;
        else if (!g_strcmp0 ("rotate-180", orientation))
            ret = 180;
        else if (!g_strcmp0 ("rotate-270", orientation))
            ret = 90;
        else if (!g_strcmp0 ("flip-rotate-0", orientation))
            ret = 0;
        else if (!g_strcmp0 ("flip-rotate-90", orientation))
            ret = 270;
        else if (!g_strcmp0 ("flip-rotate-180", orientation))
            ret = 180;
        else if (!g_strcmp0 ("flip-rotate-270", orientation))
            ret = 90;

        g_free (orientation);
    }
    return ret;
}

const char* get_gststatechange_string (GstStateChange transition)
{
    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        return ("GST_STATE_CHANGE_NULL_TO_READY");

    case GST_STATE_CHANGE_READY_TO_NULL:
        return ("GST_STATE_CHANGE_READY_TO_NULL");

    case GST_STATE_CHANGE_READY_TO_PAUSED:
        return ("GST_STATE_CHANGE_READY_TO_PAUSED");

    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        return ("GST_STATE_CHANGE_PAUSED_TO_PLAYING");

    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        return ("GST_STATE_CHANGE_PLAYING_TO_PAUSED");

    case GST_STATE_CHANGE_PAUSED_TO_READY:
        return ("GST_STATE_CHANGE_PAUSED_TO_READY");

    default:
        return ("GST_STATE_CHANGE_NOT_DEFINED");
    }
}

