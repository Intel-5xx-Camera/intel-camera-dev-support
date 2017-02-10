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

#ifndef __GST_PVL_FACE_DETECTION_FILTER_H__
#define __GST_PVL_FACE_DETECTION_FILTER_H__

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

#include <pvl_face_detection.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_PVL_FD_FILTER \
  (gst_pvl_fd_filter_get_type())
#define GST_PVL_FD_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PVL_FD_FILTER,GstPvlFdFilter))
#define GST_PVL_FD_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PVL_FD_FILTER,GstPvlFdFilterClass))
#define GST_IS_PVL_FD_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PVL_FD_FILTER))
#define GST_IS_PVL_FD_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PVL_FD_FILTER))

typedef struct _GstPvlFdFilter      GstPvlFdFilter;
typedef struct _GstPvlFdFilterClass GstPvlFdFilterClass;

struct _GstPvlFdFilter
{
    GstVideoFilter base_filter;
    pvl_face_detection *pvl_fd;

    guint orientation;

    gboolean log_perf;
    gboolean log_result;

    /* accumulated time */
    guint64 start_time;
    guint64 count;
    guint64 fd_time;
    guint64 meta_time;
    guint64 msg_time;
};

struct _GstPvlFdFilterClass
{
    GstVideoFilterClass parent_class;
};

GType gst_pvl_fd_filter_get_type (void);

G_END_DECLS

#endif /* __GST_PVL_FACE_DETECTION_FILTER_H__ */

