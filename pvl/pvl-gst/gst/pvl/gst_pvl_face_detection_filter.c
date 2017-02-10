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

#include <string.h>
#include <stdlib.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <pvl_face_detection_meta.h>

#include "gst_pvl_util.h"
#include "gst_pvl_face_detection_filter.h"

#define PVL_FD_FILTER_NAME "pvl_face_detection"

GST_DEBUG_CATEGORY_STATIC (gst_pvl_fd_filter_debug_category);
#define GST_CAT_DEFAULT gst_pvl_fd_filter_debug_category
#define parent_class gst_pvl_fd_filter_parent_class

/* Filter signals and args */
enum
{
  PROP_0,   // Don't remove this value. property value's index should be larger than 0.
  PROP_MAX_DETECTABLE_FACES,
  PROP_MIN_FACE_RATIO,
  PROP_RIP_RANGE,
  PROP_ROP_RANGE,
  PROP_NUM_ROLLOVER_FRAMES,
  PROP_LOG_PERF,
  PROP_LOG_RESULT,
};

#define VIDEO_CAPS \
    GST_VIDEO_CAPS_MAKE("{ YV12, NV21, NV12, YUY2, GRAY8, RGBx, RGBA }")

G_DEFINE_TYPE_WITH_CODE (GstPvlFdFilter, gst_pvl_fd_filter,
        GST_TYPE_VIDEO_FILTER,
        GST_DEBUG_CATEGORY_INIT (gst_pvl_fd_filter_debug_category, PVL_FD_FILTER_NAME, 0,
            "debug category for pvl face-detection filter element"));

/* define functions */
static void initialize_property (GObjectClass *gobject_class);
static void gst_pvl_fd_filter_init (GstPvlFdFilter * filter);
static void gst_pvl_fd_filter_finalize (GObject * obj);
static GstStateChangeReturn gst_pvl_fd_filter_change_state (GstElement *element, GstStateChange transition);
static GstFlowReturn gst_pvl_fd_filter_transform_frame_ip (GstVideoFilter *filter, GstVideoFrame *frame);
static void gst_pvl_fd_filter_post_message (GstPvlFdFilter * filter, GstBuffer *buf, int result_count, pvl_face_detection_result *fd_results, GstClockTime processing_time);
static void gst_pvl_fd_filter_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_pvl_fd_filter_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_pvl_fd_filter_sink_event (GstBaseTransform *trans, GstEvent *event);

/* GObject vmethod implementations */

/* initialize class */
static void
gst_pvl_fd_filter_class_init (GstPvlFdFilterClass * klass)
{
    GST_TRACE ("called klass = %p", klass);
    GObjectClass *gobject_class = (GObjectClass *) klass;

    /* property */
    gobject_class->set_property = gst_pvl_fd_filter_set_property;
    gobject_class->get_property = gst_pvl_fd_filter_get_property;
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
            "Face Detection",
            "Video/Filter",
            "Detect faces in video frames",
            "PVA Platform");

    GstBaseTransformClass *trans = GST_BASE_TRANSFORM_CLASS (klass);
    trans->sink_event = GST_DEBUG_FUNCPTR (gst_pvl_fd_filter_sink_event);

    GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
    element_class->change_state = GST_DEBUG_FUNCPTR (gst_pvl_fd_filter_change_state);

    gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_pvl_fd_filter_finalize);

    video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_pvl_fd_filter_transform_frame_ip);

}

static void
initialize_property (GObjectClass *gobject_class)
{
    pvl_face_detection *fd;
    pvl_config config = {{0, 0, 0, NULL},{pvl_false, pvl_false, pvl_false, pvl_false, NULL, NULL}, { NULL, NULL, NULL, NULL, NULL} };
    pvl_err ret = pvl_face_detection_get_default_config(&config);
    if (ret != pvl_success) {
        GST_ERROR ("error in pvl_face_detection_get_default_config() %d", ret);
        return;
    }

    ret = pvl_face_detection_create(&config, &fd);
    if (ret != pvl_success) {
        GST_ERROR ("error in pvl_face_detection_create() %d", ret);
        return;
    }

    pvl_face_detection_parameters params;
    ret = pvl_face_detection_get_parameters (fd, &params);
    if (ret != pvl_success) {
        pvl_face_detection_destroy(fd);
        GST_ERROR ("error in pvl_face_get_parameters() %d", ret);
        return;
    }

    GST_DEBUG ("max_num_faces(%d) rip_range(%d) rop(%d) min_face_ratio(%f)", params.max_num_faces, params.rip_range, params.rop_range, params.min_face_ratio);
    GST_DEBUG ("max_supported_num_faces(%d)", fd->max_supported_num_faces);
    GST_DEBUG ("rip_range_max(%d) rip_range_resolution(%d)", fd->rip_range_max, fd->rip_range_resolution);
    GST_DEBUG ("rop_range_max(%d) rop_range_resolution(%d)", fd->rop_range_max, fd->rop_range_resolution);

    /* READWRITE properies */
    g_object_class_install_property (gobject_class, PROP_MAX_DETECTABLE_FACES,
        g_param_spec_uint ("max_detectable_faces", "MaxDetectableFaces",
            "Number of detectable faces",
            1, fd->max_supported_num_faces, params.max_num_faces, (GParamFlags)G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_MIN_FACE_RATIO,
        g_param_spec_float ("min_face_ratio", "MinFaceRatio",
            "The ratio of minimum detectable face size to the shorter side of the input image.",
            0.0f, 1.0f, params.min_face_ratio, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_RIP_RANGE,
        g_param_spec_uint ("rip_range", "RipRange", "The degree of RIP (Rotation In-Plane) ranges, representing [-rip_range, +rip_range]. Valid Rip ranges are 15, 45, 75, 105, 135, 165.",
            0, fd->rip_range_max, params.rip_range, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_ROP_RANGE,
        g_param_spec_uint ("rop_range", "RopRange", "The degree of ROP (Rotation Out-of-Plane) ranges, representing [-rop_range, +rop_range]. Valid Rop ranges are 0, 90.",
            0, fd->rop_range_max, params.rop_range, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_NUM_ROLLOVER_FRAMES,
        g_param_spec_uint ("num_rollover_frame", "NumRolloverFrame",
            "The number of rollover frames indicating how many frames the entire scanning will be distributed.",
            0, 100, params.num_rollover_frames, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_LOG_PERF,
        g_param_spec_boolean ("log_perf", "LogPerf",
            "Turn the performance-log On/Off.",
            FALSE, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_LOG_RESULT,
        g_param_spec_boolean ("log_result", "LogResult",
            "Turn the analyzed-result-log On/Off.",
            FALSE, G_PARAM_READWRITE));

    pvl_face_detection_destroy (fd);
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_pvl_fd_filter_init (GstPvlFdFilter * filter)
{
    GST_TRACE ("called, filter = %p", filter);
    pvl_config config = {{0, 0, 0, NULL},{pvl_false, pvl_false, pvl_false, pvl_false, NULL, NULL}, { NULL, NULL, NULL, NULL, NULL} };
    pvl_err ret = pvl_face_detection_get_default_config (&config);
    GST_INFO ("Configuration Info");
    GST_INFO ("  Version: %s", config.version.description);
    GST_INFO ("  Supporting GPU Acceleration: %s", (config.acceleration.is_supporting_gpu == pvl_true) ? "Yes" : "No");
    GST_INFO ("  Supporting IPU Acceleration: %s", (config.acceleration.is_supporting_ipu == pvl_true) ? "Yes" : "No");

    filter->orientation = 0;

    if (filter->pvl_fd == NULL) {
        ret = pvl_face_detection_create (&config, &filter->pvl_fd);
        if (ret != pvl_success) {
            GST_ERROR ("error in pvl_face_detection_create() %d", ret);
            return;
        }
    }
    GST_DEBUG ("pvl_face_detection instance = %p", filter->pvl_fd);
}

static void
gst_pvl_fd_filter_finalize (GObject * obj)
{
    GST_TRACE ("called object = %p", obj);

    GstPvlFdFilter *filter = GST_PVL_FD_FILTER (obj);
    if (filter != NULL) {
        pvl_face_detection *pvl_fd = filter->pvl_fd;
        if (pvl_fd != NULL) {
            pvl_face_detection_destroy(pvl_fd);
            filter->pvl_fd = NULL;
        }
    }
    G_OBJECT_CLASS (gst_pvl_fd_filter_parent_class)->finalize (obj);
}

static GstStateChangeReturn
gst_pvl_fd_filter_change_state (GstElement *element, GstStateChange transition)
{
    GST_TRACE ("%s : %s", GST_ELEMENT_NAME (element), get_gststatechange_string (transition));

    GstStateChangeReturn ret = GST_ELEMENT_CLASS (gst_pvl_fd_filter_parent_class)->change_state (element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE)
        return ret;

    if (transition == GST_STATE_CHANGE_PAUSED_TO_PLAYING) {
        GstPvlFdFilter *filter = GST_PVL_FD_FILTER (element);
        filter->start_time = 0;
        filter->count = 0;
        filter->fd_time = 0;
        filter->meta_time = 0;
        filter->msg_time = 0;
    }

    return ret;
}

static gboolean
gst_pvl_fd_filter_sink_event (GstBaseTransform *trans, GstEvent *event)
{
    GstPvlFdFilter *fdfilter = GST_PVL_FD_FILTER (trans);
    gint orientation = 0;
    GST_DEBUG ("handling %s event", GST_EVENT_TYPE_NAME (event));

    switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:
        orientation = get_pvl_orientation (event);
        if (orientation != -1) {
            fdfilter->orientation = orientation;
            GST_DEBUG ("orientation: %d", orientation);
        }
        break;
    default:
        break;
    }
    return GST_BASE_TRANSFORM_CLASS (parent_class)->sink_event (trans, event);
}

static GstFlowReturn
gst_pvl_fd_filter_transform_frame_ip (GstVideoFilter *filter, GstVideoFrame *frame)
{
    pvl_image pvl_frame = { 0 };
    pvl_face_detection_result *fd_results = NULL;
    GstPvlFdFilter *fdfilter = GST_PVL_FD_FILTER (filter);
    pvl_face_detection *pvl_fd = fdfilter->pvl_fd;
    pvl_face_detection_parameters params;
    int num_faces, max_num_faces;
    pvl_err ret;

    GstClockTime start_time = gst_util_get_timestamp ();  /* @START_PVL */
    GstClockTime fd_time = 0, meta_time = 0, msg_time = 0;
    GstClockTime processing_time = 0;

    if (pvl_fd == NULL) {
        GST_ERROR ("pvl_face_detection instance is null.");
        return GST_FLOW_ERROR;
    }

    GST_OBJECT_LOCK (fdfilter);

    ret = pvl_face_detection_get_parameters (pvl_fd, &params);
    if (ret == pvl_success) {
        max_num_faces = params.max_num_faces;
    } else {
        GST_ERROR ("pvl_face_detection_get_parameters ret: %d", ret);
        GST_OBJECT_UNLOCK (fdfilter);
        return GST_FLOW_ERROR;
    }

    fd_results = (pvl_face_detection_result*)malloc (sizeof (pvl_face_detection_result) * max_num_faces);
    if (fd_results == NULL) {
        GST_WARNING ("Can't allocate %ld bytes.", sizeof (pvl_face_detection_result) * max_num_faces);
        GST_OBJECT_UNLOCK (fdfilter);
        return GST_FLOW_ERROR;
    }
    wrap_pvl_image (&pvl_frame, frame, fdfilter->orientation);

    num_faces = pvl_face_detection_run_in_preview (pvl_fd, &pvl_frame, fd_results, max_num_faces);

    GST_OBJECT_UNLOCK (fdfilter);

    fd_time = gst_util_get_timestamp ();    /* @END_PVL */
    processing_time = GST_TIME_AS_USECONDS (fd_time - start_time);

    if (num_faces > 0)
    {
        PvlFaceDetectionInfo info_list[num_faces];
        gint i;

        if (fdfilter->log_result) {
            for (i = 0; i < num_faces; i++) {
                g_print ("[%d/%d:%d] face rect = (%d,%d,%d,%d)\n", i + 1, num_faces, max_num_faces,
                    fd_results[i].rect.left,
                    fd_results[i].rect.top,
                    fd_results[i].rect.right,
                    fd_results[i].rect.bottom);
            }
        }

        for (i = 0; i < num_faces; i++) {
            info_list[i].left = fd_results[i].rect.left;
            info_list[i].top = fd_results[i].rect.top;
            info_list[i].right = fd_results[i].rect.right;
            info_list[i].bottom = fd_results[i].rect.bottom;

            info_list[i].confidence = fd_results[i].confidence;
            info_list[i].rip_angle = fd_results[i].rip_angle;
            info_list[i].rop_angle = fd_results[i].rop_angle;
            info_list[i].tracking_id = fd_results[i].tracking_id;
        }

        gst_buffer_add_pvl_face_detection_meta (frame->buffer, num_faces, info_list, processing_time);
        meta_time = gst_util_get_timestamp ();  /* @SEND_META */

        gst_pvl_fd_filter_post_message (fdfilter, frame->buffer, num_faces, fd_results, processing_time);
        msg_time = gst_util_get_timestamp ();   /* @SEND_MSG */
    } else {
        gst_buffer_add_pvl_face_detection_meta (frame->buffer, num_faces, NULL, processing_time);
        meta_time = gst_util_get_timestamp ();  /* @SEND_META */

        gst_pvl_fd_filter_post_message (fdfilter, frame->buffer, num_faces, NULL, processing_time);
        msg_time = gst_util_get_timestamp ();   /* @SEND_MSG */
    }

    free (fd_results);

    if (fdfilter->log_perf)
    {
        guint64 fd_diff = GST_TIME_AS_USECONDS (fd_time - start_time);
        guint64 meta_diff = GST_TIME_AS_USECONDS (meta_time - fd_time);
        guint64 msg_diff = GST_TIME_AS_USECONDS (msg_time - meta_time);

        fdfilter->count++;
        fdfilter->fd_time += fd_diff;
        fdfilter->meta_time += meta_diff;
        fdfilter->msg_time += msg_diff;
        if (fdfilter->start_time == 0) {
            fdfilter->start_time = start_time;
        }

        if (fdfilter->start_time + 3000000000 < start_time) {
            double diff_time = (double)(start_time - fdfilter->start_time) / 1000000;   // ms

            /*
             * @Frame-start    @START_PVL   @END_PVL     @SEND_META  @SEND_MSG
             * |               |            |            |           |
             * |---------------|------------|------------|-----------|------------------------
             * |       ?       |<-   fd   ->|<-  meta  ->|<-  msg  ->|.... Can't know after this...
             */
            g_print ("FD: fps: %5.1lf / avg-f2f: %5.1lf ms / fd: %5.1lf ms / meta: %5.1lf ms / msg: %5.1lf ms\n",
                    (double)fdfilter->count / (diff_time / 1000),
                    diff_time / fdfilter->count,
                    (double)(fdfilter->fd_time / fdfilter->count) / 1000,
                    (double)(fdfilter->meta_time / fdfilter->count) / 1000,
                    (double)(fdfilter->msg_time / fdfilter->count) / 1000);

            fdfilter->count = 0;
            fdfilter->fd_time = 0;
            fdfilter->meta_time = 0;
            fdfilter->msg_time = 0;
            fdfilter->start_time = start_time;
        }
    }

    return GST_FLOW_OK;
}

static void
gst_pvl_fd_filter_post_message (GstPvlFdFilter * filter, GstBuffer *buf, int result_count, pvl_face_detection_result *fd_results, GstClockTime processing_time)
{
    GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (filter);
    GstClockTime running_time, stream_time;

    running_time = gst_segment_to_running_time (&trans->segment, GST_FORMAT_TIME,
        GST_BUFFER_TIMESTAMP (buf));
    stream_time = gst_segment_to_stream_time (&trans->segment, GST_FORMAT_TIME,
        GST_BUFFER_TIMESTAMP (buf));

    GstStructure *data = gst_structure_new (PVL_FD_FILTER_NAME,
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
                    "left", G_TYPE_INT, fd_results[i].rect.left,
                    "top", G_TYPE_INT, fd_results[i].rect.top,
                    "right", G_TYPE_INT, fd_results[i].rect.right,
                    "bottom", G_TYPE_INT, fd_results[i].rect.bottom,
                    "confidence", G_TYPE_INT, fd_results[i].confidence,
                    "rip_angle", G_TYPE_INT, fd_results[i].rip_angle,
                    "rop_angle", G_TYPE_INT, fd_results[i].rop_angle,
                    "tracking_id", G_TYPE_INT, fd_results[i].tracking_id,
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
gst_pvl_fd_filter_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
    GstPvlFdFilter *filter = GST_PVL_FD_FILTER (object);
    pvl_face_detection_parameters params;
    pvl_face_detection_get_parameters (filter->pvl_fd, &params);

    GST_TRACE ("called filter: %p / pvl_fd: %p", filter, filter->pvl_fd);

    switch (prop_id) {
    case PROP_MAX_DETECTABLE_FACES:
        GST_OBJECT_LOCK (filter);
        params.max_num_faces = g_value_get_uint (value);
        GST_DEBUG ("Set %s: %d", g_param_spec_get_name (pspec), params.max_num_faces);
        pvl_face_detection_set_parameters (filter->pvl_fd, &params);
        GST_OBJECT_UNLOCK (filter);
        break;

    case PROP_MIN_FACE_RATIO:
        GST_OBJECT_LOCK (filter);
        params.min_face_ratio = g_value_get_float (value);
        GST_DEBUG ("Set %s: %f", g_param_spec_get_name (pspec), params.min_face_ratio);
        pvl_face_detection_set_parameters (filter->pvl_fd, &params);
        GST_OBJECT_UNLOCK (filter);
        break;

    case PROP_RIP_RANGE:
        GST_OBJECT_LOCK (filter);
        params.rip_range = g_value_get_uint (value);
        GST_DEBUG ("Set %s: %d", g_param_spec_get_name (pspec), params.rip_range);
        pvl_face_detection_set_parameters (filter->pvl_fd, &params);
        GST_OBJECT_UNLOCK (filter);
        break;

    case PROP_ROP_RANGE:
        GST_OBJECT_LOCK (filter);
        params.rop_range = g_value_get_uint (value);
        GST_DEBUG ("Set %s: %d", g_param_spec_get_name (pspec), params.rop_range);
        pvl_face_detection_set_parameters (filter->pvl_fd, &params);
        GST_OBJECT_UNLOCK (filter);
        break;

    case PROP_NUM_ROLLOVER_FRAMES:
        GST_OBJECT_LOCK (filter);
        params.num_rollover_frames = g_value_get_uint (value);
        GST_DEBUG ("Set %s: %d", g_param_spec_get_name (pspec), params.num_rollover_frames);
        pvl_face_detection_set_parameters (filter->pvl_fd, &params);
        GST_OBJECT_UNLOCK (filter);
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
gst_pvl_fd_filter_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
    GstPvlFdFilter *filter = GST_PVL_FD_FILTER (object);
    pvl_face_detection_parameters params;
    pvl_face_detection_get_parameters (filter->pvl_fd, &params);

    GST_TRACE ("called filter: %p / pvl_fd: %p", filter, filter->pvl_fd);

    switch (prop_id) {
    case PROP_MAX_DETECTABLE_FACES:
        g_value_set_uint (value, params.max_num_faces);
        GST_DEBUG ("Get %s: %d", g_param_spec_get_name (pspec), params.max_num_faces);
        break;

    case PROP_MIN_FACE_RATIO: {
        g_value_set_float (value, params.min_face_ratio);
        GST_DEBUG ("Get %s: %f", g_param_spec_get_name (pspec), params.min_face_ratio);
        break;
    }

    case PROP_RIP_RANGE:
        g_value_set_uint (value, params.rip_range);
        GST_DEBUG ("Get %s: %d", g_param_spec_get_name (pspec), params.rip_range);
        break;

    case PROP_ROP_RANGE:
        g_value_set_uint (value, params.rop_range);
        GST_DEBUG ("Get %s: %d", g_param_spec_get_name (pspec), params.rop_range);
        break;

    case PROP_NUM_ROLLOVER_FRAMES:
        g_value_set_uint (value, params.num_rollover_frames);
        GST_DEBUG ("Get %s: %d", g_param_spec_get_name (pspec), params.num_rollover_frames);
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

