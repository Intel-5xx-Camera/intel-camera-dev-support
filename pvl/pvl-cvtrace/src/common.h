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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <gst/gst.h>

#define DEFAULT_FILTER_TYPE PVL_NONE
#define DEFAULT_WIDTH   1920 //640
#define DEFAULT_HEIGHT  1080 //480

#define DRAWING_WIDTH   1920
#define DRAWING_HEIGHT  1080

#define DEFAULT_RESIZE_VIDEO_ANALYTICS_WIDTH     1280
#define DEFAULT_RESIZE_VIDEO_ANALYTICS_HEIGHT    720

#define MAX_DETECTABLE  32

#define MAX_MAPPED_FR_ID_NAME   30
#define MAX_MAPPED_FR_NAME      30

#define ERROR -9999

#define ACTIVE_ID_CAMERA "camera"
#define ACTIVE_ID_VIDEO_FILE "video_file"

#define ACTIVE_ID_FD "fd"
#define ACTIVE_ID_FR "fr"
#define ACTIVE_ID_OT "ot"
#define ACTIVE_ID_PD "pd"
#define ACTIVE_ID_OFF "off"

#define INPUT_SRC_NAME_CAMERA "camera"
#define INPUT_SRC_NAME_VIDEO_FILE "video_file"
#define FILTER_NAME_FD "pvl_face_detection"
#define FILTER_NAME_FR "pvl_face_recognition"
#define FILTER_NAME_OT "pvl_object_tracking"
#define FILTER_NAME_PD "pvl_pedestrian_detection"
#define FILTER_NAME_CAPS "capsfilter"
#define FILTER_NAME_SINK "ximagesink"
#define FILTER_NAME_ICAMERA "icamerasrc"
#define FILTER_NAME_V4L2SRC "v4l2src"
#define FILTER_NAME_TEE "tee"
//#define FILTER_NAME_SINK "autovideosink"

#define DEFAULT_PD_IMAGE_FORMAT "RGBA"
#define DEFAULT_APP_LOG FALSE
#define DEFAULT_PLUGIN_LOG FALSE

#define USE_CSS TRUE

typedef enum {
    PVL_NONE = 0,
    PVL_FD = 1,
    PVL_FR = 2,
    PVL_PD = 4,
    PVL_OT = 8,
} PvlFilterType;

typedef enum {
    PVL_FD_ITER,
    PVL_FR_ITER,
    PVL_PD_ITER,
    PVL_OT_ITER,
    NUM_PVL_FILTER_ITER
} PvlFilterIter;

typedef enum {
    FMT_YV12,
    FMT_YUY2,
    FMT_NV21,
    FMT_NV12,
    FMT_GRAY8,
    FMT_RGBx,
    FMT_RGBA,
} Format;

typedef enum {
    VIDEO_FILE,
    CAMERA,
} InputSrc;

typedef enum {
    ANY = 1,
    GST = 2,        //gstreamer log
    GTK = 4,        //gtk log
    USR = 8,        //user event log
    RESULT = 16,     //pvl_result log
} Tag;

typedef struct _rect {
    gint left, top, right, bottom;
    gint person_id;
} rectangle;

typedef struct _fr_id_name {
    gint id;
    gchar name[MAX_MAPPED_FR_NAME];
} fr_id_name;

G_BEGIN_DECLS

void set_app_log (gboolean enable);
void set_plugin_log (gboolean enable);

gfloat print_fps();
void gst_message_print (GstMessage *message);

GParamSpecUInt* get_gparam_spec_uint (GstElement *target_class, const gchar *property_name);
GParamSpecFloat* get_gparam_spec_float (GstElement *target_class, const gchar *property_name);

guint check_verify_available_input (const gchar* source, gint min, gint max);

const gchar* get_pvl_filter_name (PvlFilterType type);
const PvlFilterType get_pvl_filter_type_enum (const gchar* filter_str);
const gint get_pvl_filters_type (gchar** filters_str);

const gchar* get_format_name (Format fmt);
const Format get_format_enum (const gchar* fmt_str);

void print_pad_templates_information (const gchar *element_name);

G_END_DECLS

#endif /* __COMMON_H__ */

