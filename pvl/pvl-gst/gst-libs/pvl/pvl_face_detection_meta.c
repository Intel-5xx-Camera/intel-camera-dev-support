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

#include <string.h>
#include "pvl_face_detection_meta.h"

GType
pvl_face_detection_meta_api_get_type (void)
{
    static volatile GType type;
    static const gchar *tags[] = { "foo", "bar", NULL };

    if (g_once_init_enter (&type)) {
        GType _type = gst_meta_api_type_register ("PvlFaceDetectionMetaAPI", tags);
        g_once_init_leave (&type, _type);
    }

    return type;
}

static gboolean
pvl_face_detection_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
    PvlFaceDetectionMeta *emeta = (PvlFaceDetectionMeta *) meta;

    emeta->num_of_faces = 0;
    emeta->info_list = NULL;

    return TRUE;
}

static void
pvl_face_detection_meta_free (GstMeta * meta, GstBuffer * buffer)
{
    PvlFaceDetectionMeta *emeta = (PvlFaceDetectionMeta *) meta;

    if (emeta->info_list != NULL) {
        g_free (emeta->info_list);
        emeta->info_list = NULL;
    }
}

static gboolean
pvl_face_detection_meta_transform (GstBuffer * transbuf, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
    PvlFaceDetectionMeta *emeta = (PvlFaceDetectionMeta *) meta;

    /* we always copy no matter what transform */
    gst_buffer_add_pvl_face_detection_meta (transbuf, emeta->num_of_faces, emeta->info_list, emeta->processing_time);

    return TRUE;
}

const GstMetaInfo *
pvl_face_detection_meta_get_info (void)
{
    static const GstMetaInfo *meta_info = NULL;

    if (g_once_init_enter (&meta_info)) {
        const GstMetaInfo *mi = gst_meta_register (PVL_FACE_DETECTION_META_API_TYPE,
            "PvlFaceDetectionMeta",
            sizeof (PvlFaceDetectionMeta),
            pvl_face_detection_meta_init,
            pvl_face_detection_meta_free,
            pvl_face_detection_meta_transform);

        GST_DEBUG("gst_meta_register: %p", mi);

        g_once_init_leave (&meta_info, mi);
    }
    return meta_info;
}

PvlFaceDetectionMeta *
gst_buffer_add_pvl_face_detection_meta (GstBuffer *buffer,
                                gint num_of_faces,
                                const PvlFaceDetectionInfo *info_list,
                                GstClockTime processing_time)
{
    PvlFaceDetectionMeta *meta;

    g_return_val_if_fail (GST_IS_BUFFER (buffer), NULL);
    g_return_val_if_fail (gst_buffer_is_writable (buffer), NULL);

    meta = (PvlFaceDetectionMeta *) gst_buffer_add_meta (buffer, PVL_FACE_DETECTION_META_INFO, NULL);
    if (meta != NULL) {
        if (num_of_faces > 0) {
            gint size = num_of_faces * sizeof (PvlFaceDetectionInfo);
            PvlFaceDetectionInfo *new_info_list = (PvlFaceDetectionInfo*)g_malloc (size);
            memcpy (new_info_list, info_list, size);

            meta->num_of_faces = num_of_faces;
            meta->info_list = new_info_list;
            meta->processing_time = processing_time;
        } else {
            meta->num_of_faces = 0;
            meta->info_list = NULL;
            meta->processing_time = processing_time;
        }
    }

    return meta;
}

