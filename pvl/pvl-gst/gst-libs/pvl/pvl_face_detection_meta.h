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

#ifndef __PVL_FACE_DETECTION_META_H__
#define __PVL_FACE_DETECTION_META_H__

#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _PvlFaceDetectionInfo PvlFaceDetectionInfo;
struct _PvlFaceDetectionInfo {
    gint left;          /* The rectangular region of the detected face. */
    gint top;
    gint right;
    gint bottom;
    gint confidence;    /* The confidence value of the detected face [0, 100]. */
    gint rip_angle;     /* The approximated value of RIP angle (in degree) of the detected face. */
    gint rop_angle;     /* The approximated of ROP angle (in degree) of the detected face. */
    gint tracking_id;   /* The tracking id of the face. The value will be unique throughout
                           the component life cycle, unless pvl_face_detection_reset() is called. */
};

typedef struct _PvlFaceDetectionMeta PvlFaceDetectionMeta;
struct _PvlFaceDetectionMeta {
    GstMeta meta;
    gint num_of_faces;
    PvlFaceDetectionInfo* info_list;
    GstClockTime processing_time;   /* Unit: us */
};

#define PVL_FACE_DETECTION_META_API_TYPE (pvl_face_detection_meta_api_get_type())
#define gst_buffer_get_pvl_face_detection_meta(b) \
    ((PvlFaceDetectionMeta*)gst_buffer_get_meta((b),PVL_FACE_DETECTION_META_API_TYPE))
#define PVL_FACE_DETECTION_META_INFO (pvl_face_detection_meta_get_info())

const GstMetaInfo *pvl_face_detection_meta_get_info (void);
GType pvl_face_detection_meta_api_get_type (void);
PvlFaceDetectionMeta * gst_buffer_add_pvl_face_detection_meta (GstBuffer *buffer,
                                                gint num_of_faces,
                                                const PvlFaceDetectionInfo *info_list,
                                                GstClockTime processing_time);

G_END_DECLS

#endif /*__PVL_FACE_DETECTION_META_H__*/
