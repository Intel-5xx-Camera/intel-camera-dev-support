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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <pvl_face_detection_meta.h>
#include <pvl_face_recognition_meta.h>

#include "gst_pvl_util.h"
#include "gst_pvl_face_recognition_filter.h"

#define PVL_FR_FILTER_NAME "pvl_face_recognition"
#define USE_FR_ROI_LIMIT

GST_DEBUG_CATEGORY_STATIC (gst_pvl_fr_filter_debug_category);
#define GST_CAT_DEFAULT gst_pvl_fr_filter_debug_category
#define parent_class gst_pvl_fr_filter_parent_class

/* Filter signals and args */
enum
{
  PROP_0,   // Don't remove this value. property value's index should be larger than 0.
  PROP_MAX_RECOGNIZABLE_FACES,
  PROP_DATABASE_FILE,
  PROP_LOG_PERF,
  PROP_LOG_RESULT,
};

#define VIDEO_CAPS \
    GST_VIDEO_CAPS_MAKE("{ YV12, NV21, NV12, YUY2, GRAY8, RGBx, RGBA }")

G_DEFINE_TYPE_WITH_CODE (GstPvlFrFilter, gst_pvl_fr_filter,
        GST_TYPE_VIDEO_FILTER,
        GST_DEBUG_CATEGORY_INIT (gst_pvl_fr_filter_debug_category, PVL_FR_FILTER_NAME, 0,
            "debug category for pvl face-recognition filter element"));

/* define functions */
static void initialize_property (GObjectClass *gobject_class);
static void gst_pvl_fr_filter_init (GstPvlFrFilter *filter);
static void gst_pvl_fr_filter_finalize (GObject *obj);
static GstStateChangeReturn gst_pvl_fr_filter_change_state (GstElement *element, GstStateChange transition);
static GstFlowReturn gst_pvl_fr_filter_transform_frame_ip (GstVideoFilter *filter, GstVideoFrame *frame);
static void gst_pvl_fr_filter_post_message (GstPvlFrFilter * filter, GstBuffer *buf, int result_count, PvlFaceRecognitionInfo *fr_info_list, GstClockTime processing_time);
static void gst_pvl_fr_filter_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_pvl_fr_filter_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean loadExternalDB (const gchar *db_path, pvl_face_recognition_facedata* facedata, int32_t *facedata_cnt, pvl_face_recognition *fr);
static gboolean initExternalDB (pvl_face_recognition_facedata *fr_database, int32_t max_db_faces, int32_t facedata_size);

static gboolean gst_pvl_fr_filter_sink_event (GstBaseTransform *trans, GstEvent *event);

/* GObject vmethod implementations */
static void
gst_pvl_fr_filter_class_init (GstPvlFrFilterClass * klass)
{
    GST_TRACE ("called klass = %p", klass);
    GObjectClass *gobject_class = (GObjectClass *) klass;

    /* property */
    gobject_class->set_property = gst_pvl_fr_filter_set_property;
    gobject_class->get_property = gst_pvl_fr_filter_get_property;
    initialize_property (gobject_class);

    GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

    /* pads */
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
            gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                gst_caps_from_string (VIDEO_CAPS)));
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
            gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                gst_caps_from_string (VIDEO_CAPS)));

    /* metadata */
    gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
            "Face Recognition",
            "Video/Filter",
            "Analyze faces in video frames",
            "PVA Platform");

    GstBaseTransformClass *trans = GST_BASE_TRANSFORM_CLASS (klass);
    trans->sink_event = GST_DEBUG_FUNCPTR (gst_pvl_fr_filter_sink_event);

    GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
    element_class->change_state = GST_DEBUG_FUNCPTR (gst_pvl_fr_filter_change_state);

    gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_pvl_fr_filter_finalize);

    video_filter_class->transform_frame_ip =
            GST_DEBUG_FUNCPTR (gst_pvl_fr_filter_transform_frame_ip);
}

static void
initialize_property (GObjectClass *gobject_class)
{
    pvl_face_recognition *fr;
    pvl_config config = {{0, 0, 0, NULL},{pvl_false, pvl_false, pvl_false, pvl_false, NULL, NULL}, { NULL, NULL, NULL, NULL, NULL} };
    pvl_err ret = pvl_face_recognition_get_default_config (&config);
    if (ret != pvl_success) {
        GST_ERROR ("error in pvl_face_recognition_get_default_config() %d", ret);
        return;
    }

    ret = pvl_face_recognition_create (&config, &fr);
    if (ret != pvl_success) {
        GST_ERROR ("error in pvl_face_recognition_create() %d", ret);
        return;
    }

    pvl_face_recognition_parameters params;
    ret = pvl_face_recognition_get_parameters (fr, &params);
    if (ret != pvl_success) {
        pvl_face_recognition_destroy (fr);
        GST_ERROR ("error in pvl_face_get_parameters() %d", ret);
        return;
    }

    /* READWRITE properies */
    g_object_class_install_property (gobject_class, PROP_MAX_RECOGNIZABLE_FACES,
        g_param_spec_uint ("max_recognizable_faces", "MaxRecognizableFaces",
            "Number of recognizable faces",
            1, fr->max_supported_faces_in_preview, params.max_faces_in_preview, (GParamFlags)G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_DATABASE_FILE,
        g_param_spec_string ("database_file", "DatabaseFile",
            "Database file name",
            "", (GParamFlags)G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_LOG_PERF,
        g_param_spec_boolean ("log_perf", "LogPerf",
            "Turn the performance-log On/Off.",
            FALSE, G_PARAM_READWRITE));

    pvl_face_recognition_destroy (fr);

    g_object_class_install_property (gobject_class, PROP_LOG_RESULT,
        g_param_spec_boolean ("log_result", "LogResult",
            "Turn the analyzed-result-log On/Off.",
            FALSE, G_PARAM_READWRITE));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_pvl_fr_filter_init (GstPvlFrFilter * filter)
{
    GST_TRACE ("called filter = %p", filter);
    pvl_config config = {{0, 0, 0, NULL},{pvl_false, pvl_false, pvl_false, pvl_false, NULL, NULL}, { NULL, NULL, NULL, NULL, NULL} };
    pvl_err ret = pvl_face_recognition_get_default_config (&config);
    GST_INFO ("Configuration Info");
    GST_INFO ("  Version: %s", config.version.description);
    GST_INFO ("  Supporting GPU Acceleration: %s", (config.acceleration.is_supporting_gpu == pvl_true) ? "Yes" : "No");
    GST_INFO ("  Supporting IPU Acceleration: %s", (config.acceleration.is_supporting_ipu == pvl_true) ? "Yes" : "No");

    filter->orientation = 0;

    if (filter->pvl_fr == NULL) {
        ret = pvl_face_recognition_create (&config, &filter->pvl_fr);
        if (ret != pvl_success) {
            GST_ERROR("error in pvl_face_recognition_create() %d", ret);
            return;
        }
    }

    if (filter->pvl_fr_db == NULL) {
        filter->pvl_fr_db = (pvl_face_recognition_facedata*)malloc (filter->pvl_fr->max_faces_in_database * sizeof(pvl_face_recognition_facedata));
    }

    if (!initExternalDB (filter->pvl_fr_db, filter->pvl_fr->max_faces_in_database, filter->pvl_fr->facedata_size)) {
        GST_ERROR ("can't allocate memory for external DB.");
        return;
    }
}

static void
gst_pvl_fr_filter_finalize (GObject * obj)
{
    GstPvlFrFilter *filter = GST_PVL_FR_FILTER (obj);
    GST_TRACE ("called filter = %p", filter);

    if (filter != NULL) {
        if (filter->pvl_fr_db != NULL) {
            free (filter->pvl_fr_db);
            filter->pvl_fr_db = NULL;
        }

        pvl_face_recognition *pvl_fr = filter->pvl_fr;
        if (pvl_fr != NULL) {
            pvl_face_recognition_destroy (pvl_fr);
            filter->pvl_fr = NULL;
        }
    }
    G_OBJECT_CLASS (gst_pvl_fr_filter_parent_class)->finalize (obj);
}

static GstStateChangeReturn
gst_pvl_fr_filter_change_state (GstElement *element, GstStateChange transition)
{
    GST_TRACE ("%s : %s", GST_ELEMENT_NAME (element), get_gststatechange_string (transition));

    GstStateChangeReturn ret = GST_ELEMENT_CLASS (gst_pvl_fr_filter_parent_class)->change_state (element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE)
        return ret;

    if (transition == GST_STATE_CHANGE_PAUSED_TO_PLAYING) {
        GstPvlFrFilter *filter = GST_PVL_FR_FILTER (element);
        filter->start_time = 0;
        filter->count = 0;
        filter->fr_time = 0;
        filter->meta_time = 0;
        filter->msg_time = 0;
    }

    return ret;
}

static gboolean
gst_pvl_fr_filter_sink_event (GstBaseTransform *trans, GstEvent *event)
{
    GstPvlFrFilter *frfilter = GST_PVL_FR_FILTER (trans);
    gint orientation = 0;
    GST_DEBUG ("handling %s event", GST_EVENT_TYPE_NAME (event));

    switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:
        orientation = get_pvl_orientation (event);
        if (orientation != -1) {
            frfilter->orientation = orientation;
            GST_DEBUG ("orientation: %d", orientation);
        }
        break;
    default:
        break;
    }
    return GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);
}

static GstFlowReturn
gst_pvl_fr_filter_transform_frame_ip (GstVideoFilter *filter, GstVideoFrame *frame)
{
    pvl_image pvl_frame = { 0 };
    pvl_face_recognition_result *fr_results = NULL;
    GstPvlFrFilter *frfilter = GST_PVL_FR_FILTER (filter);
    pvl_face_recognition *pvl_fr = frfilter->pvl_fr;
    pvl_face_recognition_parameters params;
    int num_faces = 0, max_recognizable_faces, i;
    pvl_err ret;

    GstClockTime start_time = gst_util_get_timestamp ();  /* @START_PVL */
    GstClockTime fr_time = 0, meta_time = 0, msg_time = 0;
    GstClockTime processing_time = 0;

    if (pvl_fr == NULL) {
        GST_ERROR ("fr is null");
        return GST_FLOW_ERROR;
    }

    GST_OBJECT_LOCK (frfilter);

    /* get parameters */
    ret = pvl_face_recognition_get_parameters (pvl_fr, &params);
    if (ret == pvl_success) {
        max_recognizable_faces = params.max_faces_in_preview;
    } else {
        GST_ERROR ("pvl_face_recognition_get_parameters ret: %d", ret);
        GST_OBJECT_UNLOCK (frfilter);
        return GST_FLOW_ERROR;
    }

    PvlFaceDetectionMeta *info = gst_buffer_get_pvl_face_detection_meta (frame->buffer);
    if (info != NULL) {
        num_faces = (info->num_of_faces > max_recognizable_faces) ? max_recognizable_faces : info->num_of_faces;
    }

    /* If no detected face, skip the face analyzing. */
    pvl_rect face_rects[num_faces];
    int32_t face_angles[num_faces];
    int32_t tracking_ids[num_faces];
    gboolean try_fr_index[num_faces];
    int32_t try_fr_count = 0;
    if (num_faces > 0) {
        ret = pvl_face_recognition_create_result_buffer (pvl_fr, num_faces, &fr_results);
        if (ret != pvl_success) {
            GST_ERROR ("pvl_face_recognition_create_result_buffer ret: %d", ret);
            GST_OBJECT_UNLOCK (frfilter);
            return GST_FLOW_ERROR;
        }

        /* mapping GstVideoFrame into pvl_image */
        wrap_pvl_image (&pvl_frame, frame, frfilter->orientation);

        /* mapping face info to analyze faces */
        for (i = 0; i < num_faces; i++) {
            PvlFaceDetectionInfo *face_info = &info->info_list[i];
#ifdef USE_FR_ROI_LIMIT
            gboolean try_fr = (face_info->rop_angle < 45 && face_info->rop_angle > -45);
            if (try_fr) {
                face_rects[try_fr_count].left = face_info->left;
                face_rects[try_fr_count].top = face_info->top;
                face_rects[try_fr_count].right = face_info->right;
                face_rects[try_fr_count].bottom = face_info->bottom;
                face_angles[try_fr_count] = face_info->rip_angle;
                tracking_ids[try_fr_count] = face_info->tracking_id;
                try_fr_count++;
            }
            try_fr_index[i] = try_fr;
#else
            face_rects[i].left = face_info->left;
            face_rects[i].top = face_info->top;
            face_rects[i].right = face_info->right;
            face_rects[i].bottom = face_info->bottom;
            face_angles[i] = face_info->rip_angle;
            tracking_ids[i] = face_info->tracking_id;
            try_fr_count = num_faces;
#endif
        }

        /* analyze */
        ret = pvl_face_recognition_run_in_preview_with_rect (pvl_fr, &pvl_frame,
                try_fr_count, face_rects, face_angles, tracking_ids, fr_results);
    }

    GST_OBJECT_UNLOCK (frfilter);

    fr_time = gst_util_get_timestamp ();    /* @END_PVL */
    processing_time = GST_TIME_AS_USECONDS (fr_time - start_time);

    if (ret >= 0)
    {
        PvlFaceRecognitionInfo fr_info_list[num_faces];
        int32_t index_fr_result = 0;
        for (i = 0; i < num_faces; i++) {
            PvlFaceDetectionInfo *face_info = &info->info_list[i];
            int32_t person_id = -10000;
#ifdef USE_FR_ROI_LIMIT
            if (try_fr_index[i]) {
                person_id = fr_results[index_fr_result].facedata.person_id;
                index_fr_result++;
            }
#else
            person_id = fr_results[i].facedata.person_id;
#endif
            fr_info_list[i].left = face_info->left;
            fr_info_list[i].top = face_info->top;
            fr_info_list[i].right = face_info->right;
            fr_info_list[i].bottom = face_info->bottom;
            fr_info_list[i].person_id = person_id;

            if (frfilter->log_result) {
                g_print ("[%d/%d:%d] face rect = (%d,%d,%d,%d) person_id(%d)\n", i + 1, num_faces, max_recognizable_faces,
                    face_info->left,
                    face_info->top,
                    face_info->right,
                    face_info->bottom,
                    person_id);
            }
        }

        gst_buffer_add_pvl_face_recognition_meta (frame->buffer, num_faces, fr_info_list, processing_time);
        meta_time = gst_util_get_timestamp ();  /* @SEND_META */

        gst_pvl_fr_filter_post_message (frfilter, frame->buffer, num_faces, fr_info_list, processing_time);
        msg_time = gst_util_get_timestamp ();   /* @SEND_MSG */
    } else {
        gst_buffer_add_pvl_face_recognition_meta (frame->buffer, 0, NULL, processing_time);
        meta_time = gst_util_get_timestamp ();  /* @SEND_META */

        gst_pvl_fr_filter_post_message (frfilter, frame->buffer, 0, NULL, processing_time);
        msg_time = gst_util_get_timestamp ();   /* @SEND_MSG */
    }

    if (fr_results != NULL) {
        pvl_face_recognition_destroy_result_buffer (fr_results);
        fr_results = NULL;
    }

    if (frfilter->log_perf)
    {
        guint64 fr_diff = GST_TIME_AS_USECONDS (fr_time - start_time);
        guint64 meta_diff = GST_TIME_AS_USECONDS (meta_time - fr_time);
        guint64 msg_diff = GST_TIME_AS_USECONDS (msg_time - meta_time);

        frfilter->count++;
        frfilter->fr_time += fr_diff;
        frfilter->meta_time += meta_diff;
        frfilter->msg_time += msg_diff;
        if (frfilter->start_time == 0) {
            frfilter->start_time = start_time;
        }

        if (frfilter->start_time + 3000000000 < start_time) {
            double diff_time = (double)(start_time - frfilter->start_time) / 1000000;   // ms

            /*
             * @Frame-start    @START_PVL   @END_PVL     @SEND_META  @SEND_MSG
             * |               |            |            |           |
             * |---------------|------------|------------|-----------|------------------------
             * |       ?       |<-   fr   ->|<-  meta  ->|<-  msg  ->|.... Can't know after this...
             */
            g_print ("FR: fps: %5.1lf / avg-f2f: %5.1lf ms / fr: %5.1lf ms / meta: %5.1lf ms / msg: %5.1lf ms\n",
                    (double)frfilter->count / (diff_time / 1000),
                    diff_time / frfilter->count,
                    (double)(frfilter->fr_time / frfilter->count) / 1000,
                    (double)(frfilter->meta_time / frfilter->count) / 1000,
                    (double)(frfilter->msg_time / frfilter->count) / 1000);

            frfilter->count = 0;
            frfilter->fr_time = 0;
            frfilter->meta_time = 0;
            frfilter->msg_time = 0;
            frfilter->start_time = start_time;
        }
    }

    return GST_FLOW_OK;
}

static void
gst_pvl_fr_filter_post_message (GstPvlFrFilter * filter, GstBuffer *buf, int result_count, PvlFaceRecognitionInfo *fr_info_list, GstClockTime processing_time)
{
    GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (filter);
    GstClockTime running_time, stream_time;

    running_time = gst_segment_to_running_time (&trans->segment, GST_FORMAT_TIME,
        GST_BUFFER_TIMESTAMP (buf));
    stream_time = gst_segment_to_stream_time (&trans->segment, GST_FORMAT_TIME,
        GST_BUFFER_TIMESTAMP (buf));

    GstStructure *data = gst_structure_new (PVL_FR_FILTER_NAME,
            "timestamp", G_TYPE_UINT64, GST_BUFFER_TIMESTAMP (buf),
            "stream_time", G_TYPE_UINT64, stream_time,
            "running_time", G_TYPE_UINT64, running_time,
            "duration", G_TYPE_UINT64, GST_BUFFER_DURATION (buf),
            "processing_time", G_TYPE_UINT64, processing_time,
            "num_faces", G_TYPE_INT, result_count,
            NULL);

    if (result_count > 0) {
        gint i;
        GValue facelist = { 0 };
        GValue facedata = { 0 };
        g_value_init (&facelist, GST_TYPE_LIST);

        for (i = 0; i < result_count; i++) {
            GstStructure *s = gst_structure_new ("face",
                    "left", G_TYPE_INT, fr_info_list[i].left,
                    "top", G_TYPE_INT, fr_info_list[i].top,
                    "right", G_TYPE_INT, fr_info_list[i].right,
                    "bottom", G_TYPE_INT, fr_info_list[i].bottom,
                    "person_id", G_TYPE_INT, fr_info_list[i].person_id,
                    NULL);

            g_value_init (&facedata, GST_TYPE_STRUCTURE);
            g_value_take_boxed (&facedata, s);
            gst_value_list_append_value (&facelist, &facedata);
            g_value_unset (&facedata);
        }

        GstMessage *msg = gst_message_new_element (GST_OBJECT (filter), data);
        gst_structure_set_value ((GstStructure *) gst_message_get_structure (msg), "faces", &facelist);
        g_value_unset (&facelist);

        gst_element_post_message (GST_ELEMENT (filter), msg);
    } else {
        GstMessage *msg = gst_message_new_element (GST_OBJECT (filter), data);
        gst_element_post_message (GST_ELEMENT (filter), msg);
    }
}

static void
gst_pvl_fr_filter_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
    GstPvlFrFilter *filter = GST_PVL_FR_FILTER (object);
    pvl_face_recognition_parameters params;
    pvl_face_recognition_get_parameters (filter->pvl_fr, &params);

    GST_TRACE ("called filter: %p / pvl_fr: %p", filter, filter->pvl_fr);

    switch (prop_id) {
    case PROP_MAX_RECOGNIZABLE_FACES:
        GST_OBJECT_LOCK (filter);
        params.max_faces_in_preview = g_value_get_uint (value);
        GST_DEBUG ("Set %s: %d", g_param_spec_get_name (pspec), params.max_faces_in_preview);
        pvl_face_recognition_set_parameters (filter->pvl_fr, &params);
        GST_OBJECT_UNLOCK (filter);
        break;

    case PROP_DATABASE_FILE:
        snprintf (filter->db_path, 256, "%s", g_value_get_string (value));
        GST_DEBUG ("Set %s: %s", g_param_spec_get_name (pspec), filter->db_path);
        loadExternalDB (filter->db_path, filter->pvl_fr_db, NULL, filter->pvl_fr);
        break;

    case PROP_LOG_PERF:
        filter->log_perf = g_value_get_boolean (value);
        GST_DEBUG ("Set %s: %d", g_param_spec_get_name (pspec), filter->log_perf);
        break;

    case PROP_LOG_RESULT:
        filter->log_result = g_value_get_boolean (value);
        GST_DEBUG ("Set %s: %d", g_param_spec_get_name (pspec), filter->log_result);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gst_pvl_fr_filter_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
    GstPvlFrFilter *filter = GST_PVL_FR_FILTER (object);
    pvl_face_recognition_parameters params;
    pvl_face_recognition_get_parameters (filter->pvl_fr, &params);

    GST_TRACE ("called filter: %p / pvl_fr: %p", filter, filter->pvl_fr);

    switch (prop_id) {
    case PROP_MAX_RECOGNIZABLE_FACES:
        g_value_set_uint (value, params.max_faces_in_preview);
        GST_DEBUG ("Get %s: %d", g_param_spec_get_name (pspec), params.max_faces_in_preview);
        break;

    case PROP_DATABASE_FILE:
        g_value_set_string (value, filter->db_path);
        GST_DEBUG ("Get %s: %s", g_param_spec_get_name (pspec), filter->db_path);
        break;

    case PROP_LOG_PERF:
        g_value_set_boolean (value, filter->log_perf);
        GST_DEBUG ("Get %s: %d", g_param_spec_get_name (pspec), filter->log_perf);
        break;

    case PROP_LOG_RESULT:
        g_value_set_boolean (value, filter->log_result);
        GST_DEBUG ("Get %s: %d", g_param_spec_get_name (pspec), filter->log_result);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

gboolean
initExternalDB (pvl_face_recognition_facedata *fr_database, int32_t max_db_faces, int32_t facedata_size)
{
    gint i;
    memset (fr_database, 0, sizeof (pvl_face_recognition_facedata) * max_db_faces);

    fr_database[0].data = (unsigned char *)malloc (sizeof (unsigned char) * facedata_size * max_db_faces);
    if (fr_database[0].data == NULL) {
        GST_ERROR ("Error: in allocating memory of external database.");
        return pvl_false;
    }

    memset (fr_database[0].data, 0, facedata_size * max_db_faces);
    for (i = 1; i < max_db_faces; i++) {
        fr_database[i].data = fr_database[i - 1].data + facedata_size;
    }

    return pvl_true;
}

gboolean
loadExternalDB (const gchar *db_path, pvl_face_recognition_facedata* facedata, int32_t *facedata_cnt, pvl_face_recognition *fr)
{
    pvl_err err = pvl_success;
    FILE *fp = NULL;
    gint i;

    GST_DEBUG ("load db path: %s / fr: %p", db_path, fr);

    if (facedata == NULL) {
        GST_ERROR ("facedata should be allocated buffers.");
        return FALSE;
    }

    fp = fopen (db_path, "rb");
    if (fp == NULL) {
        GST_ERROR ("Error: in loading face feature data. File: %s", db_path);
        return FALSE;
    }

    int num_registered = 0;
    for (i = 0; i < fr->max_faces_in_database; i++) {
        unsigned char *ptr_bk;
        ptr_bk = facedata[i].data;
        memset (&facedata[i], 0, sizeof (pvl_face_recognition_facedata));
        facedata[i].data = ptr_bk;
        memset (facedata[i].data, 0, fr->facedata_size);
    }

    // Load data
    size_t num_registered_size = sizeof (int);
    size_t read_size = fread (&num_registered, num_registered_size, 1, fp);
    if (read_size != 1) {
        GST_ERROR ("Can't fread the db file.");
        fclose (fp);
        return FALSE;
    }
    if (num_registered < 0 || num_registered > fr->max_faces_in_database) {
        GST_ERROR ("num_registered(%d)", num_registered);
        fclose (fp);
        return FALSE;
    }
    GST_DEBUG ("Load data: %d faces", num_registered);

    pvl_face_recognition_reset (fr);

    for (i = 0; i < num_registered; i++) {
        unsigned char *ptr_bk;

        // load external database
        size_t facedata_size = fr->facedata_size;
        read_size = fread (facedata[i].data, 1, facedata_size, fp);
        if (read_size != facedata_size) {
            GST_ERROR ("facedata_size(%ld) read_size(%ld), these sizes should be same.", facedata_size, read_size);
            fclose (fp);
            return FALSE;
        }
        ptr_bk = facedata[i].data;

        size_t facedata_struct_size = sizeof (pvl_face_recognition_facedata);
        read_size = fread (&facedata[i], 1, facedata_struct_size, fp);
        if (read_size != facedata_struct_size) {
            GST_ERROR ("facedata_struct_size(%ld) read_size(%ld), these sizes should be same.", facedata_struct_size, read_size);
            fclose (fp);
            return FALSE;
        }
        facedata[i].data = ptr_bk;

        // This will try to register faces into internal database
        err = pvl_face_recognition_register_facedata (fr, &facedata[i]);
        if (err != pvl_success) {
            GST_ERROR ("Error: in registering facedata from external database.");
            num_registered = i + 1;
            break;
        }
    }

    fclose (fp);

    if (facedata_cnt != NULL) {
        (*facedata_cnt) = num_registered;
    }

    return TRUE;
}

