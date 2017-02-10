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

#ifndef __GST_PVL_UTIL_H__
#define __GST_PVL_UTIL_H__

#include <gst/gst.h>
#include <gst/video/video-format.h>
#include <gst/video/video-frame.h>
#include <pvl_types.h>

G_BEGIN_DECLS

pvl_image_format get_pvl_format (GstVideoFormat format);
gint get_pvl_orientation (GstEvent *event);
void wrap_pvl_image (pvl_image *dst, GstVideoFrame *src, guint orientation);
const char* get_gststatechange_string (GstStateChange transition);

G_END_DECLS

#endif /* __GST_PVL_UTIL_H__ */

