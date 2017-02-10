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

#ifndef __GST_PVL_FACE_RECOGNITION_FILTER_H__
#define __GST_PVL_FACE_RECOGNITION_FILTER_H__

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

#include <pvl_face_recognition.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_PVL_FR_FILTER \
  (gst_pvl_fr_filter_get_type())
#define GST_PVL_FR_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PVL_FR_FILTER,GstPvlFrFilter))
#define GST_PVL_FR_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PVL_FR_FILTER,GstPvlFrFilterClass))
#define GST_IS_PVL_FR_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PVL_FR_FILTER))
#define GST_IS_PVL_FR_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PVL_FR_FILTER))

typedef struct _GstPvlFrFilter      GstPvlFrFilter;
typedef struct _GstPvlFrFilterClass GstPvlFrFilterClass;

struct _GstPvlFrFilter
{
    GstVideoFilter base_filter;
    pvl_face_recognition *pvl_fr;
    pvl_face_recognition_facedata *pvl_fr_db;
    char db_path[256];

    guint orientation;

    gboolean log_perf;
    gboolean log_result;

    /* accumulated time */
    guint64 start_time;
    guint64 count;
    guint64 fr_time;
    guint64 meta_time;
    guint64 msg_time;
};

struct _GstPvlFrFilterClass
{
    GstVideoFilterClass parent_class;
};

GType gst_pvl_fr_filter_get_type (void);

G_END_DECLS

#endif /* __GST_PVL_FACE_RECOGNITION_FILTER_H__ */

