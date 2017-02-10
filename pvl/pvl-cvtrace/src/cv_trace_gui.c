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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//#define G_LOG_USE_STRUCTURED 1
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gst/gst.h>
#include <gst/video/video-info.h>

#include "common.h"
#include "cv_trace_gui_glade.h"

#define V4L2_SRC        "v4l2src device=/dev/video0"
#define ICAMERA_ABR     "icamerasrc device-name=imx185"
#define ICAMERA_GT      "icamerasrc device-name=0 io-mode=1"
#define ICAMERA_SRC     ICAMERA_GT
#define DEFAULT_CAMERA_SRC_DESCR(C)    C" ! video/x-raw,width=1920,height=1080"

#define ALWAYS_ALERT_LEVEL  (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL)
#define MAX_LOG_BUFFER  1024

#define LOGE(TAG, ...)  log_handler (TAG, "CVTraceGUI", G_LOG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGC(TAG, ...)  log_handler (TAG, "CVTraceGUI", G_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGW(TAG, ...)  log_handler (TAG, "CVTraceGUI", G_LOG_LEVEL_WARNING, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGI(TAG, ...)  log_handler (TAG, "CVTraceGUI", G_LOG_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGD(TAG, ...)  log_handler (TAG, "CVTraceGUI", G_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

typedef struct _AppData {
    GstElement *pipeline;

    guint bus_watch_id;

    gchar *camera_command;
    gchar *media_file;
    gchar *fr_database_file;
    gchar *pd_image_format;

    GMainLoop *main_loop;

    GtkWidget *window;
    GtkWidget *drawingarea_preview;

    gint detected_num[NUM_PVL_FILTER_ITER];
    rectangle detected_info[NUM_PVL_FILTER_ITER][MAX_DETECTABLE];

    GtkBuilder *builder;
    guintptr window_handle;

    gint pvl_filters_type;          //can be selected one more.
    InputSrc input_src;
    gboolean sync_on_the_clock;     // Default is true. If false, don't drop the video frames.
    gboolean parallelized_video_analytics;     // Default is false. If true, FD/FR/PD pipelines will run on each thread.
    gboolean auto_rotate_display;   // Default is false. If true, Display will be showed based on image-orientation tag.
    gboolean loop_video;
    gboolean resize_video_analytics;

    gint width;                     //draw width
    gint height;                    //draw height

    gint src_width;                 //frame source width
    gint src_height;                //frame source height

    gint resize_video_analytics_width;
    gint resize_video_analytics_height;

    gfloat resize_video_analytics_w_ratio;
    gfloat resize_video_analytics_h_ratio;

    gint font_size;

    gboolean is_EOS;

    fr_id_name mapped_fr_id_name[MAX_MAPPED_FR_ID_NAME];
    gint mapped_fr_id_name_count;

    guint32 r_mouse_press_t;        //left mouse press time - check whether click or long click

    gboolean plugin_log_perf;
    gboolean plugin_log_result;
} AppData;

static Tag app_tag = GST | GTK | USR | ANY;

//app
static void set_css ();
static gboolean create_pipeline (AppData *app_data);
static gboolean restart_pipeline (AppData *app_data);
static void update_pvl_parameters (AppData *app_data);
static gboolean initialize_gtk_ui (AppData *app_data);
static void update_gtk_setting_widgets_for_pvl_filter (AppData *app_data);
static void update_gtk_setting_widgets_for_input_src (AppData *app_data);
static void set_widget_pvl_static_value (AppData *app_data);    //min,max,default to label widget
static void set_label_widget_pvl_static_value (GtkBuilder *builder, GstElement *element, const gchar *property_name);
static void set_widget_pvl_default_value (AppData *app_data);   //default to entry widget
static GValue get_default_param (GstElement *element, const gchar *property_name);
static GValue get_param_from_widget (AppData *app_data, GstElement *element, const gchar *property_name);
static void set_param (GtkWidget *widget, AppData *app_data, const gchar *filter_name, const gchar *property_name);
static void set_font_size (AppData *app_data);
static void show_warning_alert_dialog (GtkWidget *window_widget, const gchar *format, ...);
static void changed_pvl_filter (AppData *app_data);
static void initialize_fr_id_name (AppData *app_data);
static void set_resize_video_analytics_ratio (AppData *app_data);
static gchar* get_camerasrc_description (AppData *app_data);
static void log_handler (const Tag tag, const gchar *domain, GLogLevelFlags log_level, const gchar *file_name, gint line_num, const gchar *func_name, const gchar *format, ...);

//gstraemer callback
static gboolean bus_cb (GstBus *bus, GstMessage *message, AppData *app_data);
static void draw_overlay_cb (GstElement *overlay, cairo_t *cr, guint64 timestamp, guint64 duration, AppData *app_data);
static gboolean decodebin_autoplug_continue_cb (GstElement *decodebin, GstPad *pad, GstCaps *caps, AppData *app_data);

//gtk & gdk callback
static void combobox_source_type_changed_cb (GtkComboBox *widget, AppData *app_data);
static void combobox_pd_image_format_changed_cb (GtkComboBox *widget, AppData *app_data);
static void switch_app_log_active_cb (GtkSwitch *widget, GParamSpec *pspec, AppData *app_data);
static void switch_pvl_plugin_log_active_cb (GtkSwitch *widget, GParamSpec *pspec, AppData *app_data);
static void button_property_reset_clicked_cb (GtkButton *widget, AppData *app_data);
static void button_play_pause_clicked_cb (GtkButton *widget, AppData *app_data);
static void button_apply_camera_command_clicked_cb (GtkButton *widget, AppData *app_data);
static void button_reset_camera_command_clicked_cb (GtkButton *widget, AppData *app_data);
static void filechooser_selection_changed_cb (GtkFileChooser *chooser, AppData *app_data);
static void filechooser_selection_changed_fr_database_cb (GtkFileChooser *chooser, AppData *app_data);
static gboolean draw_area_release_cb (GtkWidget *widget, GdkEventButton *event, AppData *app_data);
static void checkbutton_pvl_face_detection_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_pvl_face_recognition_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_pvl_object_tracking_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_pvl_pedestrian_detection_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_sync_on_the_clock_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_parallelized_video_analytics_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_auto_rotate_display_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_loop_video_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_app_log_result_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_plugin_log_perf_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_plugin_log_result_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void checkbutton_resize_video_analytics_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data);
static void combobox_resize_video_analytics_changed_cb (GtkComboBox *widget, AppData *app_data);
static gboolean key_release_event_entry_camera_apply_command (GtkWidget *widget, GdkEvent *event, AppData *app_data);
static void window_closed_cb (GtkWidget *widget, GdkEvent *event, AppData *app_data);

int
main (int argc, char *argv[])
{
    AppData app_data;
    memset (&app_data, 0, sizeof (app_data));
    app_data.pvl_filters_type = DEFAULT_FILTER_TYPE;    //PVL_NONE
    gint default_log = 0;

    const GOptionEntry options[] = {
        /* you can add your won command line options here */
//        { "filter", 'f', 0, G_OPTION_ARG_INT, &app_data.pvl_filters_type,
//            "Select filter. (default: 1). (filter-list: " FILTER_NAME_FD "(1), " FILTER_NAME_FR "(2), " FILTER_NAME_PD "(4), " FILTER_NAME_OT "(8))", NULL },
//        { "media", 'm', 0, G_OPTION_ARG_STRING, &app_data.media_file,
//            "file-path.", NULL },
        { "source", 's', 0, G_OPTION_ARG_INT, &app_data.input_src, "0: VIDEO_FILE, 1: CAMERA", NULL },
        { "log", 'l', 0, G_OPTION_ARG_INT, &default_log, "0: off, 1: on", NULL, },
        { NULL, }
    };

    GOptionContext *ctx;
    GError *err = NULL;

    ctx = g_option_context_new ("PIPELINE-DESCRIPTION");
    g_option_context_add_main_entries (ctx, options, "demo");
    g_option_context_add_group (ctx, gst_init_get_option_group ());
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
        LOGW (ANY, "Initializing: %s", err != NULL ? err->message : "Unknown error");
        g_clear_error (&err);
        g_option_context_free (ctx);
        return 1;
    }
    g_option_context_free (ctx);

    gboolean app_log = (default_log == 0) ? FALSE : TRUE;
    set_app_log (app_log);
    set_plugin_log (DEFAULT_PLUGIN_LOG);
    app_data.plugin_log_perf = FALSE;
    app_data.plugin_log_result = FALSE;

    /* init */
    gtk_init (&argc, &argv);
    gst_init (&argc, &argv);

    const gchar *glib_ver = glib_check_version (GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
    LOGI (GTK, "glib version: %s\n", glib_ver);

    if (USE_CSS) {
        set_css ();
    }

    /* Initialize app data structure */
    app_data.width = DRAWING_WIDTH;
    app_data.height = DRAWING_HEIGHT;
    app_data.pd_image_format = DEFAULT_PD_IMAGE_FORMAT;
    app_data.resize_video_analytics_w_ratio = 1.f;
    app_data.resize_video_analytics_h_ratio = 1.f;
    app_data.camera_command = get_camerasrc_description (&app_data);

    LOGI (ANY, "pvl_filters[%d], input_src[%d], size[%d,%d], media_file[%s]", app_data.pvl_filters_type, app_data.input_src, app_data.width, app_data.height, app_data.media_file);

    initialize_fr_id_name (&app_data);
    if (create_pipeline (&app_data) == FALSE) {
        LOGW (GST, "fail to create pipeline");
    }

    if (initialize_gtk_ui (&app_data) == FALSE) {
        LOGE (GTK, "Fail to initialize gtk ui.");
    }
    update_gtk_setting_widgets_for_input_src (&app_data);
    update_gtk_setting_widgets_for_pvl_filter (&app_data);

    if (app_data.pipeline != NULL) {    //pipeline is ready to play.
        GstElement *sink_element = gst_bin_get_by_name (GST_BIN (app_data.pipeline), FILTER_NAME_SINK);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink_element), app_data.window_handle);

        /* start the pipeline */
        GstStateChangeReturn sret = gst_element_set_state (GST_ELEMENT (app_data.pipeline), GST_STATE_PLAYING);

        if (sret == GST_STATE_CHANGE_FAILURE) {
            LOGE (GST, "can't change the state to PLAYING. ret: %s", gst_element_state_change_return_get_name (sret));

            show_warning_alert_dialog (app_data.window, "Load video file is failed: %s", app_data.media_file);
            gst_element_set_state (GST_ELEMENT (app_data.pipeline), GST_STATE_NULL);
        }
    }
    gtk_main();

    gst_object_unref (app_data.pipeline);
    gst_deinit ();

    return 0;
}

static
void initialize_fr_id_name (AppData *app_data)
{
    LOGD (ANY, "initialize_fr_id_name");
    gchar *contents;
    gsize length;

    if (g_file_get_contents ("fr_id_name.txt", &contents, &length, NULL)) {
        gchar** fr_id_names = g_strsplit (contents,"\n", 0);
        gint i;

        for (i = 0; i < MAX_MAPPED_FR_ID_NAME; i++) {
            if (g_ascii_strcasecmp (fr_id_names[i], "") == 0) {
                break;
            }
        }
        app_data->mapped_fr_id_name_count = i;
        if (i == 0) {
            LOGW (ANY, "Fail to mapping fr_id_name. Check format!");
            g_free (contents);
            return;
        }
        for (i = 0; i < app_data->mapped_fr_id_name_count; i++) {
            gchar** fr_id_name = g_strsplit (fr_id_names[i], "|", 2);
            app_data->mapped_fr_id_name[i].id = atoi (fr_id_name[0]);
            g_strlcpy (app_data->mapped_fr_id_name[i].name, g_strstrip (fr_id_name[1]), MAX_MAPPED_FR_NAME);
        }
        for (i=0; i< app_data->mapped_fr_id_name_count; i++) {
            LOGD (ANY, "mapped id=%d, name=%s\n",app_data->mapped_fr_id_name[i].id, app_data->mapped_fr_id_name[i].name);
        }
        g_free (contents);
    } else {
        LOGW (ANY, "Fail to load fr_id_name.txt");
    }
}

static void set_css ()
{
    GtkCssProvider *provider;
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path (provider, "cv_trace_gui.css", NULL);
    gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                                GTK_STYLE_PROVIDER (provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_USER);
}

static gchar* get_camerasrc_description(AppData *app_data)
{
    GstElementFactory *icamera_factory = gst_element_factory_find (FILTER_NAME_ICAMERA);
    GstElementFactory *v4l2_factory = gst_element_factory_find (FILTER_NAME_V4L2SRC);

    if (icamera_factory != NULL) {
        return DEFAULT_CAMERA_SRC_DESCR (ICAMERA_SRC);
    }
    if (v4l2_factory != NULL) {
        return DEFAULT_CAMERA_SRC_DESCR (V4L2_SRC);
    }
    return "Not_available";
}

static gboolean
create_pipeline (AppData *app_data)
{
    if (app_data == NULL) {
        LOGW (ANY, "app_data is null.");
        return FALSE;
    }

    LOGD (GST, "create_pipeline FD = %d, FR = %d, PD = %d, OT = %d",
            app_data->pvl_filters_type & PVL_FD,
            app_data->pvl_filters_type & PVL_FR,
            app_data->pvl_filters_type & PVL_PD,
            app_data->pvl_filters_type & PVL_OT);

    /* release */
    if (app_data->pipeline != NULL) {
        gst_object_unref (app_data->pipeline);
        g_source_remove (app_data->bus_watch_id);
        app_data->bus_watch_id = 0;
        app_data->pipeline = NULL;
    }

    GstElement *pipeline, *overlay, *decodebin;
    GstBus *bus;
    GError *err = NULL;
    gchar pipe_descr[1024];
    gchar pipe_src_descr[256];
    gchar pipe_filter_descr[512];
    gchar pipe_sink_descr[256];

    memset(pipe_filter_descr, 0, 512);

    if (app_data->input_src == CAMERA) {
        //create source
        sprintf (pipe_src_descr, "%s", app_data->camera_command);
        //create sink
        sprintf (pipe_sink_descr, " ! videoconvert ! cairooverlay name=cairooverlay ! %s name=%s", FILTER_NAME_SINK, FILTER_NAME_SINK);
    } else {    //video_file
        if (app_data->media_file == NULL) {
            LOGW (ANY, "Video file is empty!");
            return FALSE;
        }
        //create source
        sprintf (pipe_src_descr, "filesrc name=filesrc location=%s ! decodebin name=decodebin", app_data->media_file);

        //create sink
        gchar descript[256];
        sprintf (descript, " ! videoconvert ! cairooverlay name=cairooverlay");

        if (app_data->auto_rotate_display) {
            gchar auto_rotate_descript[256];
            sprintf (auto_rotate_descript, " ! videoflip method=8 ! videoscale ! video/x-raw,width=%d,height=%d", DRAWING_WIDTH, DRAWING_HEIGHT);
            strncat (descript, auto_rotate_descript, sizeof (descript) - 1);
        }
        sprintf(pipe_sink_descr, "%s ! %s name=%s sync=%s", descript, FILTER_NAME_SINK, FILTER_NAME_SINK, app_data->sync_on_the_clock ? "true" : "false");
    }

    if (app_data->parallelized_video_analytics) {
        gchar descript[256];
        sprintf (descript, " ! tee name=%s %s. ! queue", FILTER_NAME_TEE, FILTER_NAME_TEE);
        strncat (pipe_src_descr, descript, sizeof (pipe_src_descr) - 1);

        memcpy (descript, pipe_sink_descr, sizeof (descript) - 1);
        sprintf (pipe_sink_descr, " ! fakesink sync=true %s. ! queue %s", FILTER_NAME_TEE, descript);
    }

    //filter resize
    if (app_data->resize_video_analytics) {
        sprintf (pipe_filter_descr, " ! videoscale ! video/x-raw,width=%d,height=%d", app_data->resize_video_analytics_width, app_data->resize_video_analytics_height);
    }

    //ensure the src image format is NV12.
    if (app_data->input_src == VIDEO_FILE && app_data->pvl_filters_type != 0) {
        strncat (pipe_filter_descr, " ! videoconvert", sizeof (pipe_filter_descr) -1);
    }

    //create pvl filter pipeline description.
    if (app_data->pvl_filters_type & PVL_FD) {
        if (app_data->parallelized_video_analytics) {
            //strncat (pipe_filter_descr, " ! queue", sizeof (pipe_filter_descr) - 1);
        }
        gchar descript[128];
        sprintf (descript, " ! %s name=%s log-perf=%d log-result=%d", FILTER_NAME_FD, FILTER_NAME_FD, app_data->plugin_log_perf, app_data->plugin_log_result);
        strncat (pipe_filter_descr, descript, sizeof (pipe_filter_descr) - 1);
    }

    if (app_data->pvl_filters_type & PVL_FR) {
        if (app_data->parallelized_video_analytics && !(app_data->pvl_filters_type & PVL_FD)) {
            //strncat (pipe_filter_descr, " ! queue", sizeof (pipe_filter_descr) - 1);
        }

        gchar descript[128];
        sprintf (descript, " ! %s name=%s log-perf=%d log-result=%d", FILTER_NAME_FR, FILTER_NAME_FR, app_data->plugin_log_perf, app_data->plugin_log_result);
        strncat (pipe_filter_descr, descript, sizeof (pipe_filter_descr) - 1);
    }

    if (app_data->pvl_filters_type & PVL_OT) {
        if (app_data->parallelized_video_analytics) {
            //strncat (pipe_filter_descr, " ! queue", sizeof (pipe_filter_descr) - 1);
        }

        gchar descript[128];
        sprintf (descript, " ! %s name=%s log-perf=%d log-result=%d", FILTER_NAME_OT, FILTER_NAME_OT, app_data->plugin_log_perf, app_data->plugin_log_result);
        strncat (pipe_filter_descr, descript, sizeof (pipe_filter_descr) - 1);
    }

    if (app_data->pvl_filters_type & PVL_PD) {
        if (app_data->pd_image_format == NULL) {
            LOGE (ANY, "PD image format is empty!");
            show_warning_alert_dialog (app_data->window, "PD image format is empty!");
            return FALSE;
        }

        if (app_data->input_src == CAMERA || !((app_data->pvl_filters_type & ~PVL_PD) == 0)) {
            strncat (pipe_filter_descr, " ! videoconvert", sizeof (pipe_filter_descr) -1);
        }

        gchar descript[128];
        sprintf (descript, " ! video/x-raw,format=%s", app_data->pd_image_format);
        strncat (pipe_filter_descr, descript, sizeof (pipe_filter_descr) - 1);

        if (app_data->parallelized_video_analytics) {
            //strncat (pipe_filter_descr, " ! queue", sizeof (pipe_filter_descr) - 1);
        }

        sprintf (descript, " ! %s name=%s log-perf=%d log-result=%d", FILTER_NAME_PD, FILTER_NAME_PD, app_data->plugin_log_perf, app_data->plugin_log_result);
        strncat (pipe_filter_descr, descript, sizeof (pipe_filter_descr) - 1);
    }
    LOGI (GST, "pvl_filter_pipe_description -> \n  %s", pipe_filter_descr);

    if (app_data->pvl_filters_type == 0) {
        sprintf (&pipe_descr[0], "%s %s", pipe_src_descr, pipe_sink_descr);
    } else {
        sprintf (&pipe_descr[0], "%s %s %s", pipe_src_descr, pipe_filter_descr, pipe_sink_descr);
    }
    LOGI (GST, "PIPELINE_description -> \n  %s", pipe_descr);

    pipeline = gst_parse_launch (pipe_descr, &err);
    if (pipeline == NULL) {
        show_warning_alert_dialog (app_data->window, err->message);
        LOGE (GST, "gst_parse_launch: %s", err->message);
        return FALSE;
    }

    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    guint bus_watch_id = gst_bus_add_watch (bus, (GstBusFunc)bus_cb, app_data);
    gst_object_unref (bus);

    overlay = gst_bin_get_by_name (GST_BIN (pipeline), "cairooverlay");
    if (overlay != NULL) {
        g_signal_connect (overlay, "draw", G_CALLBACK (draw_overlay_cb), app_data);
    }

    //prepare src width / height
    app_data->src_width = 0;
    app_data->src_height = 0;
    decodebin = gst_bin_get_by_name (GST_BIN (pipeline), "decodebin");
    if (decodebin != NULL) {    //video file
        LOGD (GST, "add decodebin autoplug-continue callback\n");
        g_signal_connect (decodebin, "autoplug-continue", G_CALLBACK (decodebin_autoplug_continue_cb), app_data);
    } else {    //camera
        app_data->src_width = 1920;
        app_data->src_height = 1080;
        set_font_size (app_data);
    }

    set_resize_video_analytics_ratio (app_data);

    /* set member values of AppData */
    app_data->pipeline = pipeline;
    app_data->bus_watch_id = bus_watch_id;


    if ((app_data->pvl_filters_type & PVL_FR) && app_data->fr_database_file != NULL) {
        GstElement *element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_FR);
        g_object_set (element, "database_file", app_data->fr_database_file, NULL);
    }
    return TRUE;
}

static gboolean
restart_pipeline (AppData *app_data)
{
    LOGD (GST, "restart pipeline");

    if (app_data->input_src == VIDEO_FILE && app_data->media_file == NULL) {
        LOGW (ANY, "media_file is NULL. Select media_file");
        return FALSE;
    }

    LOGD (GST, "try to set GST_STATE_NULL..");
    g_usleep (500000);  //avoid hang up.
    GstStateChangeReturn sret = gst_element_set_state (GST_ELEMENT (app_data->pipeline), GST_STATE_NULL);
    LOGD (GST, "set_state to NULL. ret: %s", gst_element_state_change_return_get_name (sret));

    create_pipeline (app_data);

    /* connect sink element into video overlay on window  */
    GstElement *sink_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_SINK);
    LOGD (GST, "sink_element[%p]", sink_element);
    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink_element), app_data->window_handle);

    /* set the state to PLAYING.  */
    sret = gst_element_set_state (GST_ELEMENT (app_data->pipeline), GST_STATE_PLAYING);
    LOGD (GST, "set_state to PLAYING. ret: %s", gst_element_state_change_return_get_name (sret));

    if (sret == GST_STATE_CHANGE_FAILURE) {
        LOGW (GST, "can't change the state to PLAYING. ret: %s", gst_element_state_change_return_get_name (sret));
    }

    return TRUE;
}

static gboolean
decodebin_autoplug_continue_cb (GstElement *decodebin, GstPad *pad, GstCaps *caps, AppData *app_data)
{
    GstStructure *str;
    str = gst_caps_get_structure (caps, 0);

    const gchar *name = gst_structure_get_name (str);
    gint width = 0, height = 0;

    if (g_strrstr (name, "video")) {
        gst_structure_get_int (str, "width", &width);
        gst_structure_get_int (str, "height", &height);

        if (width > 0 && height > 0
                && app_data->src_width != width && app_data->src_height != height) {
            app_data->src_width = width;
            app_data->src_height = height;
            set_font_size (app_data);
            LOGD (GST, "Video file: decode[%s], [W/H] is [%d/%d]", name, width, height);
            set_resize_video_analytics_ratio (app_data);
            return TRUE;
        }
    }
    LOGD (GST, "decodebin autoplug: decode[%s], [W/H] = [%d/%d] is ignore.", name, width, height);
    return TRUE;
}

static void set_resize_video_analytics_ratio (AppData *app_data) {
    if (!app_data->resize_video_analytics)
        return;

    if (app_data->src_width !=0 && app_data->src_height != 0) {
        app_data->resize_video_analytics_w_ratio = (gfloat) app_data->src_width / app_data->resize_video_analytics_width;
        app_data->resize_video_analytics_h_ratio = (gfloat) app_data->src_height / app_data->resize_video_analytics_height;
        LOGD (ANY, "resize video analytics w/h ratio = [%f/%f]", app_data->resize_video_analytics_w_ratio, app_data->resize_video_analytics_h_ratio);
    }
}

static void set_font_size (AppData *app_data)
{
#if 1
    if (app_data->src_width > 1500) {
        app_data->font_size = 50;
    } else if (app_data->src_width > 1000) {
        app_data->font_size = 40;
    } else if (app_data->src_width > 500) {
        app_data->font_size = 30;
    } else {
        app_data->font_size = 20;
    }
#else
    app_data->font_size = 20;
#endif
}


static void set_entry_widget (GtkBuilder *builder, const gchar *property_name, GValue g_value)
{
    gchar entry_id[64];
    gchar value_str[64];
    sprintf (entry_id, "entry_%s", property_name);
    GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (builder, entry_id));
    if (entry != NULL) {
        if (G_VALUE_HOLDS_UINT (&g_value)) {
            sprintf (value_str, "%d", g_value_get_uint (&g_value));
        } else if (G_VALUE_HOLDS_INT (&g_value)) {
            sprintf (value_str, "%d", g_value_get_int (&g_value));
        } else if (G_VALUE_HOLDS_FLOAT (&g_value)) {
            sprintf (value_str, "%f", g_value_get_float (&g_value));
        }
        gtk_entry_set_text (GTK_ENTRY (entry), value_str);
    }
}

static void
set_label_widget_pvl_static_value (GtkBuilder *builder, GstElement *element, const gchar *property_name)
{
    if (builder == NULL || element == NULL || property_name == NULL) {
        LOGW (GST, "builder(%p) element(%p) property(%s)", builder, element, property_name);
        return;
    }

    GParamSpec *gparam_spec = g_object_class_find_property (G_OBJECT_GET_CLASS(element), property_name);
    if (gparam_spec) {
        gchar min_str[64];
        gchar max_str[64];
        gchar def_str[64];

        //get values from element's gparam_spec
        if (G_IS_PARAM_SPEC_UINT (gparam_spec)) {
            GParamSpecUInt *gparam_spec_uint = G_PARAM_SPEC_UINT(gparam_spec);
            if (gparam_spec_uint != NULL) {
                sprintf (min_str, "%d", gparam_spec_uint->minimum);
                sprintf (max_str, "%d", gparam_spec_uint->maximum);
                sprintf (def_str, "%d", gparam_spec_uint->default_value);
            }
        } else if (G_IS_PARAM_SPEC_INT (gparam_spec)) {
            GParamSpecInt *gparam_spec_int = G_PARAM_SPEC_INT(gparam_spec);
            if (gparam_spec_int != NULL) {
                sprintf (min_str, "%d", gparam_spec_int->minimum);
                sprintf (max_str, "%d", gparam_spec_int->maximum);
                sprintf (def_str, "%d", gparam_spec_int->default_value);
            }
        } else if (G_IS_PARAM_SPEC_FLOAT (gparam_spec)) {
            GParamSpecFloat *gparam_spec_float = G_PARAM_SPEC_FLOAT(gparam_spec);
            if (gparam_spec_float != NULL) {
                sprintf (min_str, "%.4f", gparam_spec_float->minimum);
                sprintf (max_str, "%.4f", gparam_spec_float->maximum);
                sprintf (def_str, "%.4f", gparam_spec_float->default_value);
            }
        } else {
            LOGW (GST, "Can't support PARAM_SPEC_DATA_TYPE");
            return;
        }

        //set value to gtk_label_widget
        gchar label_id_min[64];
        sprintf (label_id_min, "label_%s_min", property_name);
        GtkWidget *label = GTK_WIDGET (gtk_builder_get_object (builder, label_id_min));
        if (label != NULL) {
            gtk_label_set_text (GTK_LABEL (label), min_str);
        }

        gchar label_id_max[64];
        sprintf (label_id_max, "label_%s_max", property_name);
        label = GTK_WIDGET (gtk_builder_get_object (builder, label_id_max));
        if (label != NULL) {
            gtk_label_set_text (GTK_LABEL (label), max_str);
        }

        gchar label_id_def[64];
        sprintf (label_id_def, "label_%s_default", property_name);
        label = GTK_WIDGET (gtk_builder_get_object (builder, label_id_def));
        if (label != NULL) {
            gtk_label_set_text (GTK_LABEL (label), def_str);
        }
    } else {
        LOGW (GST, "%s is null", property_name);
    }
}

static void
update_pvl_parameters (AppData *app_data)
{
    GstElement *pipeline = app_data->pipeline;
    if (pipeline == NULL) {
        LOGW (GST, "pipeline is null");
        return;
    }

    GValue value_cur = {0, };
    GValue value_default = {0, };
    if (app_data->pvl_filters_type & PVL_FD) {
        GstElement *fd_element = gst_bin_get_by_name (GST_BIN (pipeline), FILTER_NAME_FD);
        if (fd_element == NULL) {
            LOGW (GST, "Can't find filter(%s) on pipeline.", FILTER_NAME_FD);
            return;
        }

        value_default = get_default_param (fd_element, "max_detectable_faces");
        value_cur = get_param_from_widget (app_data, fd_element, "max_detectable_faces");
        if (G_VALUE_HOLDS_UINT (&value_default) && G_VALUE_HOLDS_UINT (&value_cur)
                && (g_value_get_uint (&value_default) != g_value_get_uint (&value_cur))) {
            gchar entry_id[64];
            sprintf (entry_id, "entry_%s", "max_detectable_faces");
            GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
            set_param (entry, app_data, FILTER_NAME_FD, "max_detectable_faces");
        }

        value_default = get_default_param (fd_element, "min_face_ratio");
        value_cur = get_param_from_widget (app_data, fd_element, "min_face_ratio");
        if (G_VALUE_HOLDS_FLOAT (&value_default) && G_VALUE_HOLDS_FLOAT (&value_cur)
                && (g_value_get_float (&value_default) != g_value_get_float (&value_cur))) {
            gchar entry_id[64];
            sprintf (entry_id, "entry_%s", "min_face_ratio");
            GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
            set_param (entry, app_data, FILTER_NAME_FD, "min_face_ratio");
        }

        value_default = get_default_param (fd_element, "rip_range");
        value_cur = get_param_from_widget (app_data, fd_element, "rip_range");
        if (G_VALUE_HOLDS_UINT (&value_default) && G_VALUE_HOLDS_UINT (&value_cur)
                && (g_value_get_uint (&value_default) != g_value_get_uint (&value_cur))) {
            gchar entry_id[64];
            sprintf (entry_id, "entry_%s", "rip_range");
            GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
            set_param (entry, app_data, FILTER_NAME_FD, "rip_range");
        }

        value_default = get_default_param (fd_element, "rop_range");
        value_cur = get_param_from_widget (app_data, fd_element, "rop_range");
        if (G_VALUE_HOLDS_UINT (&value_default) && G_VALUE_HOLDS_UINT (&value_cur)
                && (g_value_get_uint (&value_default) != g_value_get_uint (&value_cur))) {
            gchar entry_id[64];
            sprintf (entry_id, "entry_%s", "rop_range");
            GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
            set_param (entry, app_data, FILTER_NAME_FD, "rop_range");
        }

        value_default = get_default_param (fd_element, "num_rollover_frame");
        value_cur = get_param_from_widget (app_data, fd_element, "num_rollover_frame");
        if (G_VALUE_HOLDS_UINT (&value_default) && G_VALUE_HOLDS_UINT (&value_cur)
                && (g_value_get_uint (&value_default) != g_value_get_uint (&value_cur))) {
            gchar entry_id[64];
            sprintf (entry_id, "entry_%s", "num_rollover_frame");
            GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
            set_param (entry, app_data, FILTER_NAME_FD, "num_rollover_frame");
        }
    }

    if (app_data->pvl_filters_type & PVL_FR) {
        GstElement *fr_element = gst_bin_get_by_name (GST_BIN (pipeline), FILTER_NAME_FR);
        if (fr_element == NULL) {
            LOGW (GST, "Can't find filter(%s) on pipeline.", FILTER_NAME_FR);
            return;
        }

        value_default = get_default_param (fr_element, "max_recognizable_faces");
        value_cur = get_param_from_widget (app_data, fr_element, "max_recognizable_faces");
        if (G_VALUE_HOLDS_UINT (&value_default) && G_VALUE_HOLDS_UINT (&value_cur)
                && (g_value_get_uint (&value_default) != g_value_get_uint (&value_cur))) {
            gchar entry_id[64];
            sprintf (entry_id, "entry_%s", "max_recognizable_faces");
            GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
            set_param (entry, app_data, FILTER_NAME_FR, "max_recognizable_faces");
        }
    }

    if (app_data->pvl_filters_type & PVL_PD) {
        GstElement *pd_element = gst_bin_get_by_name (GST_BIN (pipeline), FILTER_NAME_PD);
        if (pd_element == NULL) {
            LOGW (GST, "Can't find filter(%s) on pipeline.", FILTER_NAME_PD);
            return;
        }

        value_default = get_default_param (pd_element, "max_detectable_pedestrians");
        value_cur = get_param_from_widget (app_data, pd_element, "max_detectable_pedestrians");
        if (G_VALUE_HOLDS_INT (&value_default) && G_VALUE_HOLDS_INT (&value_cur)
                && (g_value_get_int (&value_default) != g_value_get_int (&value_cur))) {
            gchar entry_id[64];
            sprintf (entry_id, "entry_%s", "max_detectable_pedestrians");
            GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
            set_param (entry, app_data, FILTER_NAME_PD, "max_detectable_pedestrians");
        }

        value_default = get_default_param (pd_element, "min_pedestrian_height");
        value_cur = get_param_from_widget (app_data, pd_element, "min_pedestrian_height");

        if (G_VALUE_HOLDS_UINT (&value_default) && G_VALUE_HOLDS_UINT (&value_cur)
                && (g_value_get_uint (&value_default) != g_value_get_uint (&value_cur))) {
            gchar entry_id[64];
            sprintf (entry_id, "entry_%s", "min_pedestrian_height");
            GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
            set_param (entry, app_data, FILTER_NAME_PD, "min_pedestrian_height");
        }
    }
}

static GValue
get_default_param (GstElement *element, const gchar *property_name)
{
    GValue ret_val = {0, };

    if (element == NULL || property_name == NULL) {
        LOGW (GST, "Wrong parameter: element(%p), property_name(%s)", element, property_name);
        return ret_val;
    }

    GParamSpec *gparam_spec = g_object_class_find_property (G_OBJECT_GET_CLASS(element), property_name);
    if (gparam_spec) {
        if (G_IS_PARAM_SPEC_UINT (gparam_spec)) {
            GParamSpecUInt *gparam_spec_uint = G_PARAM_SPEC_UINT(gparam_spec);
            if (gparam_spec_uint != NULL) {
                g_value_init (&ret_val, G_TYPE_UINT);
                g_value_set_uint (&ret_val, gparam_spec_uint->default_value);
            }
        } else if (G_IS_PARAM_SPEC_INT (gparam_spec)) {
            GParamSpecInt *gparam_spec_int = G_PARAM_SPEC_INT(gparam_spec);
            if (gparam_spec_int != NULL) {
                g_value_init (&ret_val, G_TYPE_INT);
                g_value_set_int (&ret_val, gparam_spec_int->default_value);
            }
        } else if (G_IS_PARAM_SPEC_FLOAT (gparam_spec)) {
            GParamSpecFloat *gparam_spec_float = G_PARAM_SPEC_FLOAT(gparam_spec);
            if (gparam_spec_float != NULL) {
                g_value_init (&ret_val, G_TYPE_FLOAT);
                g_value_set_float (&ret_val, gparam_spec_float->default_value);
            }
        }
    } else {
        LOGW (GST, "%s is null", property_name);
    }
    return ret_val;
}

static GValue
get_param_from_widget (AppData *app_data, GstElement *element, const gchar *property_name)
{
    GValue ret_val = {0, };

    if (app_data->builder == NULL || property_name == NULL) {
        LOGW (GST, "Wrong parameter: builder((%p), property_name(%s)", app_data->builder, property_name);
        return ret_val;
    }

    gchar entry_id[64];
    sprintf (entry_id, "entry_%s", property_name);
    GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (app_data->builder, entry_id));
    if (entry == NULL) {
        LOGW (GST, "entry widget is null.: (entry_%s)", property_name);
        return ret_val;
    }

    GParamSpec *gparam_spec = g_object_class_find_property (G_OBJECT_GET_CLASS(element), property_name);

    const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
    char *pend;

    if (G_IS_PARAM_SPEC_UINT (gparam_spec)) {
        guint value = strtoul (text, &pend, 10);
        if (*pend != '\0') {
            return ret_val;
        }
        LOGD (USR, "[property_name, value]: [%s, %d]", property_name, value);
        g_value_init (&ret_val, G_TYPE_UINT);
        g_value_set_uint (&ret_val, value);
        return ret_val;

    } else if (G_IS_PARAM_SPEC_INT (gparam_spec)) {
        gint value = strtoul (text, &pend, 10);
        if (*pend != '\0') {
            return ret_val;
        }
        LOGD (USR, "[property_name, value]: [%s, %d]", property_name, value);
        g_value_init (&ret_val, G_TYPE_INT);
        g_value_set_int (&ret_val, value);
        return ret_val;

    } else if (G_IS_PARAM_SPEC_FLOAT (gparam_spec)) {
        gfloat value = atof (text);
        LOGD (USR, "[property_name, value]: [%s, %d]", property_name, value);
        g_value_init (&ret_val, G_TYPE_FLOAT);
        g_value_set_float (&ret_val, value);
        return ret_val;

    } else {
        LOGW (GST, "no matched param_spec_type! : %s", property_name);
        return ret_val;
    }
}

static void
set_param (GtkWidget *widget, AppData *app_data, const gchar *filter_name, const gchar *property_name)
{
    GstElement *element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), filter_name);
    if (element == NULL) {
        LOGW (GST, "Can't find filter(%s) on pipeline.", filter_name);
        return;
    }

    GParamSpec *gparam_spec = g_object_class_find_property (G_OBJECT_GET_CLASS(element), property_name);

    const gchar *text = gtk_entry_get_text (GTK_ENTRY (widget));
    char *pend;

    if (G_IS_PARAM_SPEC_UINT (gparam_spec)) {
        guint value_get = 0;
        guint value = strtoul (text, &pend, 10);
        if (*pend != '\0') {
            goto restore_value;
        }

        g_object_set (element, property_name, value, NULL);
        g_object_get (element, property_name, &value_get, NULL);
        LOGD (USR, "set_param : %s set/get ( %d / %d )", property_name, value, value_get);

        if (value != value_get) {
            goto restore_value;
        }
        return;

    } else if (G_IS_PARAM_SPEC_INT (gparam_spec)) {
        gint value_get = 0;
        gint value = strtol (text, &pend, 10);
        if (*pend != '\0') {
            goto restore_value;
        }

        g_object_set (element, property_name, value, NULL);
        g_object_get (element, property_name, &value_get, NULL);
        LOGD (USR, "set_param_by_enter : %s set/get ( %d / %d )", property_name, value, value_get);

        if (value != value_get) {
            goto restore_value;
        }
        return;

    } else if (G_IS_PARAM_SPEC_FLOAT (gparam_spec)) {
        gfloat value_get = 0;
        gfloat value = atof (text);

        g_object_set (element, property_name, value, NULL);
        g_object_get (element, property_name, &value_get, NULL);
        LOGD (USR, "set_param_by_enter : %s set/get ( %f / %f )", property_name, value, value_get);

        if (fabs (value - value_get) > 0.0000001f) {
            LOGW (GST, "Set: %lf, Get: %lf, diff: %lf", value, value_get, fabs (value - value_get));
            goto restore_value;
        }
        return;
    } else {
        LOGW (GST, "no matched param_spec_type! : %s", property_name);
        return;
    }

restore_value:
    {
        gchar message[64];

        if (G_IS_PARAM_SPEC_UINT (gparam_spec)) {
            guint value_get = 0;
            gchar text_new[64];
            g_object_get (element, property_name, &value_get, NULL);
            sprintf (text_new, "%d", value_get);
            gtk_entry_set_text (GTK_ENTRY (widget), text_new);

            GParamSpecUInt *gparam_spec_uint = G_PARAM_SPEC_UINT (gparam_spec);
            if (gparam_spec_uint != NULL) {
                sprintf (message, "Valid range: %d ~ %d", gparam_spec_uint->minimum, gparam_spec_uint->maximum);
                show_warning_alert_dialog (app_data->window, message);
            }

        } else if (G_IS_PARAM_SPEC_INT (gparam_spec)) {
            gint value_get = 0;
            gchar text_new[64];
            g_object_get (element, property_name, &value_get, NULL);
            sprintf (text_new, "%d", value_get);
            gtk_entry_set_text (GTK_ENTRY (widget), text_new);
            GParamSpecInt *gparam_spec_int = G_PARAM_SPEC_INT (gparam_spec);
            if (gparam_spec_int != NULL) {
                sprintf (message, "Valid range: %d ~ %d", gparam_spec_int->minimum, gparam_spec_int->maximum);
                show_warning_alert_dialog (app_data->window, message);
            }
        } else if (G_IS_PARAM_SPEC_FLOAT (gparam_spec)) {
            gfloat value_get = 0;
            gchar text_new[64];
            g_object_get (element, property_name, &value_get, NULL);
            sprintf (text_new, "%.4f", value_get);
            gtk_entry_set_text (GTK_ENTRY (widget), text_new);

            GParamSpecFloat *gparam_spec_float = G_PARAM_SPEC_FLOAT (gparam_spec);
            if (gparam_spec_float != NULL) {
                sprintf (message, "Valid range: %.4f ~ %.4f", gparam_spec_float->minimum, gparam_spec_float->maximum);
                show_warning_alert_dialog (app_data->window, message);
            }
        }
    }
}

static void
set_param_by_keyboard (GtkWidget *widget, GdkEvent *event, AppData *app_data, const gchar *filter_name, const gchar *property_name)
{
    GdkEventType event_type = gdk_event_get_event_type (event);
    if (event_type == GDK_KEY_RELEASE) {
        guint keyval = 0;
        gdk_event_get_keyval (event, &keyval);
        if (keyval == GDK_KEY_Return) {
            LOGD (USR, "Enter key: %s", property_name);
            set_param (widget, app_data, filter_name, property_name);
        }
    }
}

gboolean
key_release_event_entry_camera_apply_command (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    GdkEventType event_type = gdk_event_get_event_type (event);
    if (event_type == GDK_KEY_RELEASE) {
        guint keyval = 0;
        gdk_event_get_keyval (event, &keyval);
        if (keyval == GDK_KEY_Return) {
            LOGD (USR, "key_release_event_entry_camera_apply_command");

            GstStateChangeReturn gret = gst_element_get_state(GST_ELEMENT (app_data->pipeline), NULL, NULL, GST_NSECOND);
            LOGD (GST, "gret %d, %s", gret, gst_element_state_change_return_get_name(gret));

            if (gret != GST_STATE_CHANGE_ASYNC) {
                GtkWidget *entry_camera_command = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "entry_camera_command"));
                gchar *camera_command = (gchar*) gtk_entry_get_text (GTK_ENTRY (entry_camera_command));
                LOGI (GTK, "input camera command: %s", camera_command);
                app_data->camera_command = camera_command;

                restart_pipeline (app_data);
                update_pvl_parameters (app_data);
            }
        }
    }

    return TRUE;
}

gboolean
key_release_event_entry_max_detectable_faces (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    set_param_by_keyboard (widget, event, app_data, FILTER_NAME_FD, "max_detectable_faces");
    return TRUE;
}

gboolean
key_release_event_entry_min_face_ratio (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    set_param_by_keyboard (widget, event, app_data, FILTER_NAME_FD, "min_face_ratio");
    return TRUE;
}

gboolean
key_release_event_entry_rip_range (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    set_param_by_keyboard (widget, event, app_data, FILTER_NAME_FD, "rip_range");
    return TRUE;
}

gboolean
key_release_event_entry_rop_range (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    set_param_by_keyboard (widget, event, app_data, FILTER_NAME_FD, "rop_range");
    return TRUE;
}

gboolean
key_release_event_entry_num_rollover_frame (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    set_param_by_keyboard (widget, event, app_data, FILTER_NAME_FD, "num_rollover_frame");
    return TRUE;
}

gboolean
key_release_event_entry_max_recognizable_faces (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    set_param_by_keyboard (widget, event, app_data, FILTER_NAME_FR, "max_recognizable_faces");
    return TRUE;
}

gboolean
key_release_event_entry_max_detectable_pedestrians (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    set_param_by_keyboard (widget, event, app_data, FILTER_NAME_PD, "max_detectable_pedestrians");
    return TRUE;
}

gboolean
key_release_event_entry_min_pedestrian_height (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    set_param_by_keyboard (widget, event, app_data, FILTER_NAME_PD, "min_pedestrian_height");
    return TRUE;
}

static void
signal_connect_for_enter_key (AppData *app_data)
{
    GtkBuilder *builder = app_data->builder;
    GtkWidget *entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry_max_detectable_faces"));
    g_signal_connect (G_OBJECT (entry), "key-release-event", G_CALLBACK (key_release_event_entry_max_detectable_faces), app_data);

    entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry_min_face_ratio"));
    g_signal_connect (G_OBJECT (entry), "key-release-event", G_CALLBACK (key_release_event_entry_min_face_ratio), app_data);

    entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry_rip_range"));
    g_signal_connect (G_OBJECT (entry), "key-release-event", G_CALLBACK (key_release_event_entry_rip_range), app_data);

    entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry_rop_range"));
    g_signal_connect (G_OBJECT (entry), "key-release-event", G_CALLBACK (key_release_event_entry_rop_range), app_data);

    entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry_num_rollover_frame"));
    g_signal_connect (G_OBJECT (entry), "key-release-event", G_CALLBACK (key_release_event_entry_num_rollover_frame), app_data);

    entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry_max_recognizable_faces"));
    g_signal_connect (G_OBJECT (entry), "key-release-event", G_CALLBACK (key_release_event_entry_max_recognizable_faces), app_data);

    entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry_max_detectable_pedestrians"));
    g_signal_connect (G_OBJECT (entry), "key-release-event", G_CALLBACK (key_release_event_entry_max_detectable_pedestrians), app_data);

    entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry_min_pedestrian_height"));
    g_signal_connect (G_OBJECT (entry), "key-release-event", G_CALLBACK (key_release_event_entry_min_pedestrian_height), app_data);

}

static void
set_widget_pvl_static_value (AppData *app_data)
{
    if (app_data == NULL || app_data->builder == NULL) {
        LOGW (ANY, "AppData(%p) builder(%p)", app_data, (app_data != NULL) ? app_data->builder : NULL);
        return;
    }

    GtkBuilder *builder = app_data->builder;

    if (app_data->pvl_filters_type & PVL_FD) {
        GstElement *fd_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_FD);
        set_label_widget_pvl_static_value (builder, fd_element, "max_detectable_faces");
        set_label_widget_pvl_static_value (builder, fd_element, "min_face_ratio");
        set_label_widget_pvl_static_value (builder, fd_element, "rip_range");
        set_label_widget_pvl_static_value (builder, fd_element, "rop_range");
        set_label_widget_pvl_static_value (builder, fd_element, "num_rollover_frame");
    }

    if (app_data->pvl_filters_type & PVL_FR) {
        GstElement *fr_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_FR);
        set_label_widget_pvl_static_value (builder, fr_element, "max_recognizable_faces");
    }

    if (app_data->pvl_filters_type & PVL_PD) {
        GstElement *pd_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_PD);
        set_label_widget_pvl_static_value (builder, pd_element, "max_detectable_pedestrians");
        set_label_widget_pvl_static_value (builder, pd_element, "min_pedestrian_height");
    }
}

static void
set_widget_pvl_default_value (AppData *app_data)
{
    if (app_data == NULL || app_data->builder == NULL) {
        LOGW (ANY, "AppData(%p) builder(%p)", app_data, (app_data != NULL) ? app_data->builder : NULL);
        return;
    }

    GtkBuilder *builder = app_data->builder;

    if (app_data->pvl_filters_type & PVL_FD) {
        GstElement *fd_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_FD);
        if (fd_element != NULL) {
            set_entry_widget (builder, "max_detectable_faces", get_default_param (fd_element, "max_detectable_faces"));
            set_entry_widget (builder, "min_face_ratio", get_default_param (fd_element, "min_face_ratio"));
            set_entry_widget (builder, "rip_range", get_default_param (fd_element, "rip_range"));
            set_entry_widget (builder, "rop_range", get_default_param (fd_element, "rop_range"));
            set_entry_widget (builder, "num_rollover_frame", get_default_param (fd_element, "num_rollover_frame"));
        }
    }

    if (app_data->pvl_filters_type & PVL_FR) {
        GstElement *fr_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_FR);
        if (fr_element != NULL) {
            set_entry_widget (builder, "max_recognizable_faces", get_default_param (fr_element, "max_recognizable_faces"));
        }
    }

    if (app_data->pvl_filters_type & PVL_PD) {
        GstElement *pd_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_PD);
        if (pd_element != NULL) {
            set_entry_widget (builder, "max_detectable_pedestrians", get_default_param (pd_element, "max_detectable_pedestrians"));
            set_entry_widget (builder, "min_pedestrian_height", get_default_param (pd_element, "min_pedestrian_height"));
        }
    }
}

static gboolean
draw_area_release_cb (GtkWidget *widget, GdkEventButton *event, AppData *app_data)
{
    gint x = (gint) event->x;
    gint y = (gint) event->y;
    guint button = (gint) event->button;
    guint32 time = event->time;
    GdkEventType type = event->type;

    if (button == 1) {  //left button (Panel ui handling)
        if (type == GDK_BUTTON_RELEASE) {
            GtkWidget *revealer_setting = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "revealer_setting"));
            if (revealer_setting == NULL) {
                LOGW (GTK, "revealer_setting is NULL.");
            }
            gboolean is_reveal = gtk_revealer_get_reveal_child (GTK_REVEALER (revealer_setting));
            LOGD (GTK, "is_reveal is %d", is_reveal);
            gtk_revealer_set_reveal_child (GTK_REVEALER (revealer_setting), !is_reveal);
        }

    } else if (button ==3) {    //right button (OT event handling)
        GstElement *element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_OT);
        if (element == NULL) {
            LOGD (GST, "Can't find pvl_object_tracking filter from pipeline.");
            return FALSE;
        }
        LOGD (USR, "OT start trigger x = %d , y = %d, type = %d, button = %d, time = %d\n", x, y, type, button, time);

        if (type == GDK_BUTTON_PRESS) {
            app_data->r_mouse_press_t = time;
        } else if (type == GDK_BUTTON_RELEASE) {
            char structure_name[64] = "";
            GstStructure *structure = NULL;
            if (time - app_data->r_mouse_press_t < 300) {
                //click event, add OT area.
                GstElement *ot_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_OT);
                if (ot_element == NULL) {
                    LOGW (GST, "OT element is NULL");
                    return FALSE;
                }

                sprintf (structure_name, "%s_start", FILTER_NAME_OT);
                if (app_data->src_width != DRAWING_WIDTH || app_data->src_height != DRAWING_HEIGHT) {
                    if (app_data->src_width > DRAWING_WIDTH || app_data->src_height > DRAWING_HEIGHT) {
                        LOGW (ANY, "src_W/H[%d/%d] is bigger than DRAWING W/H[%d/%d]", app_data->src_width, app_data->src_height, DRAWING_WIDTH, DRAWING_HEIGHT);
                    }
                    x = x - (DRAWING_WIDTH - app_data->src_width) / 2;
                    y = y - (DRAWING_HEIGHT - app_data->src_height) /2;
                    LOGD (USR, "OT target trigger trans x = %d, y = %d\n", x, y);
                }

                structure = gst_structure_new (  structure_name,
                        "x", G_TYPE_UINT, x,
                        "y", G_TYPE_UINT, y, NULL);
            } else {
                //long click event, clear OT area.
                sprintf (structure_name, "%s_stop", FILTER_NAME_OT);
                structure = gst_structure_new_empty (structure_name);
                app_data->src_width = 0;
                app_data->src_height = 0;
            }

            if (structure != NULL) {
                GstEvent *gst_event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM, structure);
                gst_element_send_event (element, gst_event);
            }
        }
    }
    return TRUE;
}

static void
changed_pvl_filter (AppData *app_data)
{
    //alway ui is showed.
    update_gtk_setting_widgets_for_pvl_filter (app_data);

    //clear detection_num for each pvl filter
    int i;
    for (i = 0; i < NUM_PVL_FILTER_ITER; i++) {
        app_data->detected_num[i] = 0;
    }

    restart_pipeline (app_data);

    //should be updated after create pipeline
    set_widget_pvl_static_value (app_data);
    set_widget_pvl_default_value (app_data);
}

static void
checkbutton_pvl_face_detection_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "check pvl_face_detection");

    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    if (is_checked) {
        app_data->pvl_filters_type |= PVL_FD;
    } else {
        app_data->pvl_filters_type &= ~PVL_FD;
    }

    changed_pvl_filter (app_data);
}

static void
checkbutton_pvl_face_recognition_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "check pvl_face_recognition");

    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    if (is_checked) {
        app_data->pvl_filters_type |= (PVL_FD | PVL_FR);
    } else {
        app_data->pvl_filters_type &= ( ~(PVL_FD | PVL_FR));
    }

    changed_pvl_filter (app_data);
}

static void
set_default_app_setting_for_pedestrian_detection (AppData *app_data)
{
    app_data->parallelized_video_analytics = TRUE;
    app_data->resize_video_analytics = TRUE;
    app_data->resize_video_analytics_width = DEFAULT_RESIZE_VIDEO_ANALYTICS_WIDTH;
    app_data->resize_video_analytics_height = DEFAULT_RESIZE_VIDEO_ANALYTICS_HEIGHT;

    set_resize_video_analytics_ratio (app_data);

    gchar *default_size = g_strdup_printf ("%dx%d", DEFAULT_RESIZE_VIDEO_ANALYTICS_WIDTH, DEFAULT_RESIZE_VIDEO_ANALYTICS_HEIGHT);
    GtkWidget *comboboxtext_resize_video_analytics = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "comboboxtext_resize_video_analytics"));
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (comboboxtext_resize_video_analytics), default_size);
    gtk_widget_set_sensitive (comboboxtext_resize_video_analytics, TRUE);


    GtkToggleButton *checkbutton_resize_video_analytics = GTK_TOGGLE_BUTTON (gtk_builder_get_object (app_data->builder, "checkbutton_resize_video_analytics"));
    GtkToggleButton *checkbutton_parallelized_video_analytics = GTK_TOGGLE_BUTTON (gtk_builder_get_object (app_data->builder, "checkbutton_parallelized_video_analytics"));

    g_signal_handlers_block_by_func (checkbutton_resize_video_analytics, checkbutton_resize_video_analytics_toggled_cb, app_data);
    gtk_toggle_button_set_active (checkbutton_resize_video_analytics, TRUE);
    g_signal_handlers_unblock_by_func (checkbutton_resize_video_analytics, checkbutton_resize_video_analytics_toggled_cb, app_data);

    g_signal_handlers_block_by_func (checkbutton_parallelized_video_analytics, checkbutton_parallelized_video_analytics_toggled_cb, app_data);
    gtk_toggle_button_set_active (checkbutton_parallelized_video_analytics, TRUE);
    g_signal_handlers_unblock_by_func (checkbutton_parallelized_video_analytics, checkbutton_parallelized_video_analytics_toggled_cb, app_data);
}

static void
checkbutton_pvl_pedestrian_detection_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "checkbutton_pvl_pedestrian_detection");

    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    if (is_checked) {
        app_data->pvl_filters_type |= PVL_PD;
        set_default_app_setting_for_pedestrian_detection (app_data);
    } else {
        app_data->pvl_filters_type &= ~PVL_PD;
    }

    changed_pvl_filter (app_data);
}

static void
checkbutton_pvl_object_tracking_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "checkbutton_pvl_object_tracking");

    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    if (is_checked) {
        app_data->pvl_filters_type |= PVL_OT;
    } else {
        app_data->pvl_filters_type &= ~PVL_OT;
    }

    changed_pvl_filter (app_data);
}

static void
checkbutton_sync_on_the_clock_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "checkbutton_sync_on_the_clock_toggled_cb");
    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    app_data->sync_on_the_clock = is_checked;

    restart_pipeline (app_data);
    update_pvl_parameters (app_data);
}

static void checkbutton_parallelized_video_analytics_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "checkbutton_parallelized_video_analytics_toggled_cb");
    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    app_data->parallelized_video_analytics = is_checked;

    restart_pipeline (app_data);
    update_pvl_parameters (app_data);
}

static void checkbutton_auto_rotate_display_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "checkbutton_auto_rotate_display_toggled_cb");
    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    app_data->auto_rotate_display = is_checked;

    restart_pipeline (app_data);
    update_pvl_parameters (app_data);
}

static void
checkbutton_loop_video_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "checkbutton_loop_video_toggled_cb");
    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    app_data->loop_video = is_checked;
}

static void
checkbutton_resize_video_analytics_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    LOGD (USR, "checkbutton_resize_video_analytics_toggled_cb");
    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    app_data->resize_video_analytics = is_checked;

    GtkWidget *resize_video_analytics = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "comboboxtext_resize_video_analytics"));
    if (resize_video_analytics) {
        gtk_widget_set_sensitive (resize_video_analytics, is_checked);
    } else {
        LOGW (GTK, "comboboxtext_resize_video_analytics is NULL");
    }

    restart_pipeline (app_data);
    update_pvl_parameters (app_data);
}

static gboolean
initialize_gtk_ui (AppData *app_data)
{
    guint major = gtk_get_major_version ();
    guint minor = gtk_get_minor_version ();
    guint micro = gtk_get_micro_version ();
    LOGI (GTK, "GTK version : %d.%d.%d", major, minor, micro);

    /* START GTK - preapre the gtk ui */
    GdkWindow *video_window_xwindow;
    GtkWidget *window, *drawingarea_preview;
    GError *error = NULL;

    GtkBuilder *builder = gtk_builder_new ();
    if (!gtk_builder_add_from_string (builder, STR_MAIN_LAYOUT, -1, &error)) {
        LOGE (ANY, "%s", error->message);
        g_free (error);
        return FALSE;
    }
    app_data->builder = builder;

    window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
    LOGD (GTK, "window %p", window);

    drawingarea_preview = GTK_WIDGET (gtk_builder_get_object (builder, "drawingarea_preview"));
    LOGD (GTK, "drawingarea_preview %p", drawingarea_preview);
//    gtk_widget_set_double_buffered (drawingarea_preview, TRUE);

    gtk_widget_set_size_request (drawingarea_preview, DRAWING_WIDTH, DRAWING_HEIGHT);

    g_signal_connect (G_OBJECT (drawingarea_preview), "button-press-event", G_CALLBACK (draw_area_release_cb), app_data);
    g_signal_connect (G_OBJECT (drawingarea_preview), "button-release-event", G_CALLBACK (draw_area_release_cb), app_data);

    g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (window_closed_cb), app_data);
    //initialize_pvl_filter
    GtkWidget *checkbutton_fd = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_pvl_face_detection"));
    GtkWidget *checkbutton_fr = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_pvl_face_recognition"));
    GtkWidget *checkbutton_pd = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_pvl_pedestrian_detection"));
    GtkWidget *checkbutton_ot = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_pvl_object_tracking"));
    GtkWidget *checkbutton_sync_on_the_clock = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_sync_on_the_clock"));
    GtkWidget *checkbutton_parallelized_video_analytics = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_parallelized_video_analytics"));
    GtkWidget *checkbutton_auto_rotate_display = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_auto_rotate_display"));
    GtkWidget *checkbutton_loop_video = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_loop_video"));

    g_signal_connect ( G_OBJECT (checkbutton_fd), "toggled", G_CALLBACK (checkbutton_pvl_face_detection_toggled_cb), app_data);
    g_signal_connect ( G_OBJECT (checkbutton_fr), "toggled", G_CALLBACK (checkbutton_pvl_face_recognition_toggled_cb), app_data);
    g_signal_connect ( G_OBJECT (checkbutton_pd), "toggled", G_CALLBACK (checkbutton_pvl_pedestrian_detection_toggled_cb), app_data);
    g_signal_connect ( G_OBJECT (checkbutton_ot), "toggled", G_CALLBACK (checkbutton_pvl_object_tracking_toggled_cb), app_data);

    //verify pvl filter available
    GstElementFactory *pvl_factory = gst_element_factory_find (FILTER_NAME_FD);
    if (pvl_factory == NULL) {
        gtk_widget_set_sensitive (checkbutton_fd, FALSE);
    }
    pvl_factory = gst_element_factory_find (FILTER_NAME_FR);
    if (pvl_factory == NULL) {
        gtk_widget_set_sensitive (checkbutton_fr, FALSE);
    }
    pvl_factory = gst_element_factory_find (FILTER_NAME_PD);
    if (pvl_factory == NULL) {
        gtk_widget_set_sensitive (checkbutton_pd, FALSE);
    }
    pvl_factory = gst_element_factory_find (FILTER_NAME_OT);
    if (pvl_factory == NULL) {    //OT disable
        gtk_widget_set_sensitive (checkbutton_ot, FALSE);
    }

    g_signal_connect ( G_OBJECT (checkbutton_sync_on_the_clock), "toggled", G_CALLBACK (checkbutton_sync_on_the_clock_toggled_cb), app_data);
    g_signal_connect ( G_OBJECT (checkbutton_parallelized_video_analytics), "toggled", G_CALLBACK (checkbutton_parallelized_video_analytics_toggled_cb), app_data);
    g_signal_connect ( G_OBJECT (checkbutton_auto_rotate_display), "toggled", G_CALLBACK (checkbutton_auto_rotate_display_toggled_cb), app_data);
    g_signal_connect ( G_OBJECT (checkbutton_loop_video), "toggled", G_CALLBACK (checkbutton_loop_video_toggled_cb), app_data);

    app_data->sync_on_the_clock = gtk_toggle_button_get_active ((GtkToggleButton*)checkbutton_sync_on_the_clock);
    app_data->parallelized_video_analytics = gtk_toggle_button_get_active ((GtkToggleButton*)checkbutton_parallelized_video_analytics);

    //Common setting - input src and pvl filter
    GtkWidget *combobox_source_type = GTK_WIDGET (gtk_builder_get_object (builder, "comboboxtext_source_type"));
    if (combobox_source_type) {
        //verify camerasrc availablitiy (icamerasrc & v4l2src)
        GstElementFactory *icamera_factory = gst_element_factory_find (FILTER_NAME_ICAMERA);
        GstElementFactory *v4l2_factory = gst_element_factory_find (FILTER_NAME_V4L2SRC);
        if (icamera_factory != NULL || v4l2_factory != NULL) {
            gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combobox_source_type), ACTIVE_ID_CAMERA, INPUT_SRC_NAME_CAMERA);
        }
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combobox_source_type), ACTIVE_ID_VIDEO_FILE, INPUT_SRC_NAME_VIDEO_FILE);
        if (app_data->input_src == CAMERA && (icamera_factory != NULL || v4l2_factory)) {
            gtk_combo_box_set_active_id (GTK_COMBO_BOX (combobox_source_type), ACTIVE_ID_CAMERA);
        }
        else {
            gtk_combo_box_set_active_id (GTK_COMBO_BOX (combobox_source_type), ACTIVE_ID_VIDEO_FILE);
        }
        g_signal_connect (G_OBJECT (combobox_source_type) , "changed",
            G_CALLBACK (combobox_source_type_changed_cb), app_data);
    } else {
        LOGW (GTK, "comboboxtext_input_src is NULL");
    }

    //Camera setting
    GtkWidget *button_apply_camera_command = GTK_WIDGET (gtk_builder_get_object (builder, "button_apply_camera_command"));
    if (button_apply_camera_command) {
        g_signal_connect ( G_OBJECT (button_apply_camera_command), "clicked",
                G_CALLBACK (button_apply_camera_command_clicked_cb), app_data);
    } else {
        LOGW (GTK, "button_apply_camera_command is NULL");
    }
    GtkWidget *button_reset_camera_command = GTK_WIDGET (gtk_builder_get_object (builder, "button_reset_camera_command"));
    if (button_reset_camera_command) {
        g_signal_connect ( G_OBJECT (button_reset_camera_command), "clicked",
                G_CALLBACK (button_reset_camera_command_clicked_cb), app_data);
    } else {
        LOGW (GTK, "button_apply_camera_command is NULL");
    }
    GtkWidget *entry_camera_command = GTK_WIDGET (gtk_builder_get_object (builder, "entry_camera_command"));
    if (entry_camera_command) {
        gtk_entry_set_text (GTK_ENTRY (entry_camera_command), get_camerasrc_description(app_data));
        g_signal_connect (G_OBJECT (entry_camera_command), "key-release-event", G_CALLBACK (key_release_event_entry_camera_apply_command), app_data);
    } else {
        LOGW (GTK, "entry_camera_command is NULL!");
    }

    //Video File setting
    GtkWidget *filechooser_button_video = GTK_WIDGET (gtk_builder_get_object (builder, "filechooserbutton_video_file"));
    if (filechooser_button_video) {
        g_signal_connect ( GTK_FILE_CHOOSER (filechooser_button_video), "selection-changed",
            G_CALLBACK (filechooser_selection_changed_cb), app_data);

    } else {
        LOGW (GTK, "filechooser_button_video is NULL");
    }
    GtkWidget *button_video_file_play_pause = GTK_WIDGET (gtk_builder_get_object (builder, "button_video_file_play_pause"));
    if (button_video_file_play_pause) {
        g_signal_connect (G_OBJECT (button_video_file_play_pause) , "clicked",
            G_CALLBACK (button_play_pause_clicked_cb), app_data);
    } else {
        LOGW (GTK, "button_play_pause is NULL");
    }

    //App log setting
    GtkWidget *switch_app_log = GTK_WIDGET (gtk_builder_get_object (builder, "switch_app_log"));
    if (switch_app_log) {
        g_signal_connect (GTK_SWITCH (switch_app_log),
                "notify::active",
                G_CALLBACK (switch_app_log_active_cb),
                app_data);
    } else {
        LOGW (GTK, "switch_app_log is NULL");
    }
    GtkWidget *checkbutton_app_log_result = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_app_log_result"));
    if (checkbutton_app_log_result) {
        g_signal_connect ( G_OBJECT (checkbutton_app_log_result), "toggled", G_CALLBACK (checkbutton_app_log_result_toggled_cb), app_data);
        gtk_widget_set_sensitive (checkbutton_app_log_result, DEFAULT_APP_LOG);
    } else {
        LOGW (GTK, "checkbutton_app_log_result is NULL");
    }

    //Plugin log setting
    GtkWidget *switch_pvl_plugin_log = GTK_WIDGET (gtk_builder_get_object (builder, "switch_pvl_plugin_log"));
    if (switch_pvl_plugin_log) {
        g_signal_connect (GTK_SWITCH (switch_pvl_plugin_log),"notify::active",
                G_CALLBACK (switch_pvl_plugin_log_active_cb), app_data);
    } else {
        LOGW (GTK, "switch_pvl_plugin_log is NULL");
    }
    GtkWidget *checkbutton_plugin_log_perf = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_plugin_log_perf"));
    if (checkbutton_plugin_log_perf) {
        g_signal_connect ( G_OBJECT (checkbutton_plugin_log_perf), "toggled", G_CALLBACK (checkbutton_plugin_log_perf_toggled_cb), app_data);
        gtk_widget_set_sensitive (checkbutton_plugin_log_perf, DEFAULT_PLUGIN_LOG);
    } else {
        LOGW (GTK, "checkbutton_plugin_log_perf is NULL");
    }
    GtkWidget *checkbutton_plugin_log_result = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_plugin_log_result"));
    if (checkbutton_plugin_log_result) {
        g_signal_connect ( G_OBJECT (checkbutton_plugin_log_result), "toggled", G_CALLBACK (checkbutton_plugin_log_result_toggled_cb), app_data);
        gtk_widget_set_sensitive (checkbutton_plugin_log_result, DEFAULT_PLUGIN_LOG);
    } else {
        LOGW (GTK, "checkbutton_plugin_log_result is NULL");
    }

    signal_connect_for_enter_key (app_data);

    GtkWidget *combobox_pd_image_format = GTK_WIDGET (gtk_builder_get_object (builder, "comboboxtext_pd_image_format"));
    if (combobox_pd_image_format) {
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combobox_pd_image_format), "NV12", "NV12");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combobox_pd_image_format), "RGBA", "RGBA");
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (combobox_pd_image_format), DEFAULT_PD_IMAGE_FORMAT);
        g_signal_connect (G_OBJECT (combobox_pd_image_format) , "changed",
            G_CALLBACK (combobox_pd_image_format_changed_cb), app_data);
        gtk_widget_set_sensitive (combobox_pd_image_format, TRUE);
    } else {
        LOGW (GTK, "comboboxtext_pd_image_format is NULL");
    }


    //OT setting & panel UI, widget_set_events must to be set before widget realized.
    gtk_widget_set_events (drawingarea_preview, gtk_widget_get_events (drawingarea_preview)
              | GDK_LEAVE_NOTIFY_MASK
              | GDK_BUTTON_PRESS_MASK
              | GDK_BUTTON_RELEASE_MASK
              | GDK_POINTER_MOTION_MASK
              | GDK_POINTER_MOTION_HINT_MASK);

    //Apply & Reset setting
    GtkWidget *button_property_reset = GTK_WIDGET (gtk_builder_get_object (builder, "button_setting_property_reset"));
    if (button_property_reset) {
        g_signal_connect (G_OBJECT (button_property_reset) , "clicked",
            G_CALLBACK (button_property_reset_clicked_cb), app_data);
    } else {
        LOGW (GTK, "button_property_reset is NULL");
    }

    //FR database
    GtkWidget *filechooser_button_fr_database = GTK_WIDGET (gtk_builder_get_object (builder, "filechooserbutton_fr_database"));
    if (filechooser_button_fr_database) {
        g_signal_connect ( GTK_FILE_CHOOSER (filechooser_button_fr_database), "selection-changed",
            G_CALLBACK (filechooser_selection_changed_fr_database_cb), app_data);

    } else {
        LOGW (GTK, "filechooser_button_video is NULL");
    }

    GtkWidget *checkbutton_resize_video_analytics = GTK_WIDGET (gtk_builder_get_object (builder, "checkbutton_resize_video_analytics"));
    g_signal_connect ( G_OBJECT (checkbutton_resize_video_analytics), "toggled", G_CALLBACK (checkbutton_resize_video_analytics_toggled_cb), app_data);

    GtkWidget *comboboxtext_resize_video_analytics = GTK_WIDGET (gtk_builder_get_object (builder, "comboboxtext_resize_video_analytics"));
    if (comboboxtext_resize_video_analytics) {
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (comboboxtext_resize_video_analytics), "1920x1080", "1920 x 1080");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (comboboxtext_resize_video_analytics), "1280x960", "1280 x 960");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (comboboxtext_resize_video_analytics), "1280x720", "1280 x 720");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (comboboxtext_resize_video_analytics), "800x600", "800 x 600");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (comboboxtext_resize_video_analytics), "640x480", "640 x 480");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (comboboxtext_resize_video_analytics), "320x240", "320 x 240");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (comboboxtext_resize_video_analytics), "176x144", "176 x 144");
        gchar *default_size = g_strdup_printf ("%dx%d", DEFAULT_RESIZE_VIDEO_ANALYTICS_WIDTH, DEFAULT_RESIZE_VIDEO_ANALYTICS_HEIGHT);
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (comboboxtext_resize_video_analytics), default_size);
        g_signal_connect (G_OBJECT (comboboxtext_resize_video_analytics), "changed", G_CALLBACK (combobox_resize_video_analytics_changed_cb), app_data);
        gtk_widget_set_sensitive (comboboxtext_resize_video_analytics, FALSE);
        app_data->resize_video_analytics_width = DEFAULT_RESIZE_VIDEO_ANALYTICS_WIDTH;
        app_data->resize_video_analytics_height = DEFAULT_RESIZE_VIDEO_ANALYTICS_HEIGHT;
    } else {
        LOGW (GTK, "comboboxtext_resize_video_analytics is NULL");
    }

    app_data->window = window;

    gtk_widget_show_all (window);
    gtk_widget_realize (window);

    video_window_xwindow = gtk_widget_get_window (drawingarea_preview);
    app_data->window_handle = GDK_WINDOW_XID (video_window_xwindow);
    app_data->drawingarea_preview = drawingarea_preview;

    return TRUE;
}

static void
switch_app_log_active_cb (GtkSwitch *widget, GParamSpec *pspec, AppData *app_data)
{
    gboolean enable = gtk_switch_get_active (GTK_SWITCH (widget));
    set_app_log (enable);

    GtkWidget *checkbutton_app_log_result  = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "checkbutton_app_log_result"));
    gtk_widget_set_sensitive (checkbutton_app_log_result, enable);
}

static void
switch_pvl_plugin_log_active_cb (GtkSwitch *widget, GParamSpec *pspec, AppData *app_data)
{
    gboolean enable = gtk_switch_get_active (GTK_SWITCH (widget));
    set_plugin_log (enable);

    GtkWidget *checkbutton_plugin_log_perf  = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "checkbutton_plugin_log_perf"));
    GtkWidget *checkbutton_plugin_log_result  = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "checkbutton_plugin_log_result"));
    gtk_widget_set_sensitive (checkbutton_plugin_log_perf, enable);
    gtk_widget_set_sensitive (checkbutton_plugin_log_result, enable);
}

static void checkbutton_app_log_result_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    if (is_checked) {
        app_tag |= RESULT;
    } else {
        app_tag &= ~RESULT;
    }
}

static void checkbutton_plugin_log_perf_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    app_data->plugin_log_perf = is_checked ? TRUE : FALSE;

    restart_pipeline (app_data);
    update_pvl_parameters (app_data);
}

static void checkbutton_plugin_log_result_toggled_cb (GtkToggleButton *togglebutton, AppData *app_data)
{
    gboolean is_checked = gtk_toggle_button_get_active (togglebutton);
    app_data->plugin_log_result = is_checked ? TRUE : FALSE;

    restart_pipeline (app_data);
    update_pvl_parameters (app_data);
}

static void
combobox_resize_video_analytics_changed_cb (GtkComboBox *widget, AppData *app_data)
{
    const gchar *active_value = gtk_combo_box_get_active_id (widget);
    LOGD (USR, "combobox_resize_video_analytics_changed_cb: %s", active_value);

    gchar** values = g_strsplit (active_value, "x", 2);
    gint i, resize[2];
    for (i = 0; i < 2; i++) {
        resize[i] = atoi (values[i]);
    }
    app_data->resize_video_analytics_width = resize[0];
    app_data->resize_video_analytics_height = resize[1];

    restart_pipeline (app_data);
    update_pvl_parameters (app_data);
}

static void
show_warning_alert_dialog (GtkWidget *window_widget, const gchar *format, ...) {
    gchar message[256];
    GtkWidget *dialog;
    GtkWindow *window = GTK_WINDOW (window_widget);

    va_list args;
    va_start (args, format);
    vsprintf (message, format, args);
    va_end (args);

    dialog = gtk_message_dialog_new (window,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING,
                GTK_BUTTONS_OK,
                "%s", message);

    gtk_window_set_title (GTK_WINDOW (dialog), "Warning");
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

static void
button_property_reset_clicked_cb (GtkButton *button, AppData *app_data)
{
    LOGD (USR, "button_property_reset_clicked_cb");
    set_widget_pvl_default_value (app_data);

    if (app_data == NULL || app_data->builder == NULL) {
        LOGW (ANY, "AppData(%p) builder(%p)", app_data, (app_data != NULL) ? app_data->builder : NULL);
        return;
    }

    GtkBuilder *builder = app_data->builder;

    if (app_data->pvl_filters_type & PVL_FD) {
        GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_max_detectable_faces"));
        set_param (widget, app_data, FILTER_NAME_FD, "max_detectable_faces");

        widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_min_face_ratio"));
        set_param (widget, app_data, FILTER_NAME_FD, "min_face_ratio");

        widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_rip_range"));
        set_param (widget, app_data, FILTER_NAME_FD, "rip_range");

        widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_rop_range"));
        set_param (widget, app_data, FILTER_NAME_FD, "rop_range");

        widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_num_rollover_frame"));
        set_param (widget, app_data, FILTER_NAME_FD, "num_rollover_frame");
    }

    if (app_data->pvl_filters_type & PVL_FR) {
        GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_max_recognizable_faces"));
        set_param (widget, app_data, FILTER_NAME_FR, "max_recognizable_faces");

    } else if (app_data->pvl_filters_type & PVL_PD) {
        GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_max_detectable_pedestrians"));
        set_param (widget, app_data, FILTER_NAME_PD, "max_detectable_pedestrians");
        widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_min_pedestrian_height"));
        set_param (widget, app_data, FILTER_NAME_PD, "min_pedestrian_height");
    }
}

static void
update_gtk_setting_widgets_for_pvl_filter (AppData *app_data)
{
    GtkWidget *expander_fd  = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "expander_fd"));
    GtkWidget *expander_fr  = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "expander_fr"));
    GtkWidget *expander_pd  = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "expander_pd"));

    LOGD (GTK, "pvl filter current status : fd[%d], fr[%d], pd[%d], ot[%d]",
            (app_data->pvl_filters_type & PVL_FD),
            (app_data->pvl_filters_type & PVL_FR),
            (app_data->pvl_filters_type & PVL_PD),
            (app_data->pvl_filters_type & PVL_OT));

    gtk_widget_set_sensitive (expander_fd, FALSE);
    gtk_widget_set_sensitive (expander_fr, FALSE);
    gtk_widget_set_sensitive (expander_pd, FALSE);

    if (app_data->pvl_filters_type & PVL_FD) {
        gtk_expander_set_expanded (GTK_EXPANDER (expander_fd), TRUE);
        gtk_widget_set_sensitive (expander_fd, TRUE);
    } else {
        gtk_expander_set_expanded (GTK_EXPANDER (expander_fd), FALSE);
        gtk_widget_set_sensitive (expander_fd, FALSE);
    }

    GtkWidget *checkbutton_fd = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "checkbutton_pvl_face_detection"));
    if (app_data->pvl_filters_type & PVL_FR) {
        gtk_expander_set_expanded (GTK_EXPANDER (expander_fr), TRUE);
        gtk_widget_set_sensitive (expander_fr, TRUE);

        gtk_widget_set_sensitive (checkbutton_fd, FALSE);
        g_signal_handlers_block_by_func (checkbutton_fd, checkbutton_pvl_face_detection_toggled_cb, app_data);
        gtk_toggle_button_set_active ((GtkToggleButton*)checkbutton_fd, TRUE);
        g_signal_handlers_unblock_by_func (checkbutton_fd, checkbutton_pvl_face_detection_toggled_cb, app_data);
    } else {
        gtk_expander_set_expanded (GTK_EXPANDER (expander_fr), FALSE);
        gtk_widget_set_sensitive (expander_fr, FALSE);

        if (!(app_data->pvl_filters_type & PVL_FD)) {
            gtk_widget_set_sensitive (checkbutton_fd, TRUE);
            g_signal_handlers_block_by_func (checkbutton_fd, checkbutton_pvl_face_detection_toggled_cb, app_data);
            gtk_toggle_button_set_active ((GtkToggleButton*)checkbutton_fd, FALSE);
            g_signal_handlers_unblock_by_func (checkbutton_fd, checkbutton_pvl_face_detection_toggled_cb, app_data);
        }
    }

    if (app_data->pvl_filters_type & PVL_PD) {
        gtk_expander_set_expanded (GTK_EXPANDER (expander_pd), TRUE);
        gtk_widget_set_sensitive (expander_pd, TRUE);
    } else {
        gtk_expander_set_expanded (GTK_EXPANDER (expander_pd), FALSE);
        gtk_widget_set_sensitive (expander_pd, FALSE);
    }
}

// ------------------------------------------
// source type - CAMERA / VIDEO FILE
static void
combobox_source_type_changed_cb (GtkComboBox *widget, AppData *app_data)
{
    LOGD (USR, "combobox_source_type_changed_cb");
    const gchar* active_value = gtk_combo_box_get_active_id (widget);
    LOGD (GTK, "select_input_src_changed : %s", active_value);

    InputSrc input_src = CAMERA;
    if (g_ascii_strcasecmp (ACTIVE_ID_CAMERA, active_value) == 0) {
        input_src = CAMERA;
    } else if (g_ascii_strcasecmp (ACTIVE_ID_VIDEO_FILE, active_value) == 0) {
        input_src = VIDEO_FILE;
    } else {
        LOGE (GTK, "can't find %s", active_value);
    }
    app_data->input_src = input_src;
    update_gtk_setting_widgets_for_input_src (app_data);

    if (input_src == CAMERA) {
        restart_pipeline (app_data);
        //should be updated after create pipeline
        set_widget_pvl_static_value (app_data);
        set_widget_pvl_default_value (app_data);
    } else if (input_src == VIDEO_FILE) {
        //stop pipeline
        GstStateChangeReturn sret = gst_element_set_state (GST_ELEMENT (app_data->pipeline), GST_STATE_NULL);
        LOGD (GST, "set_state to NULL. ret: %s", gst_element_state_change_return_get_name (sret));
    } else {
        LOGW (USR, "can't find input_src: %d",input_src);
    }
}

static void
combobox_pd_image_format_changed_cb (GtkComboBox *widget, AppData *app_data)
{
    LOGD (USR, "combobox_pd_image_format_changed_cb");
    gchar *active_value = (gchar*) gtk_combo_box_get_active_id (widget);
    LOGD (GTK, "combobox_pd_image_format_changed : %s", active_value);

    app_data->pd_image_format = active_value;
    update_gtk_setting_widgets_for_input_src (app_data);

    restart_pipeline (app_data);
    update_pvl_parameters (app_data);
}

static void
button_play_pause_clicked_cb (GtkButton *button, AppData *app_data)
{
    LOGD (USR, "button_play_pause_clicked_cb");
    if (app_data->pipeline == NULL || app_data->media_file == NULL) {
        show_warning_alert_dialog (app_data->window, "Select video first!");
        LOGW (ANY, "select video file first");
        return;
    }

    GtkWidget* button_video_file_play_pause = GTK_WIDGET (gtk_builder_get_object (app_data->builder,
                "button_video_file_play_pause"));

    if (app_data->is_EOS) {
        create_pipeline (app_data);
        GstElement *sink_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_SINK);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink_element), app_data->window_handle);

        GstStateChangeReturn sret = gst_element_set_state (GST_ELEMENT (app_data->pipeline), GST_STATE_PLAYING);
        LOGD (GST, "set_state to PLAYING. ret: %s", gst_element_state_change_return_get_name (sret));
        gtk_button_set_label (GTK_BUTTON (button_video_file_play_pause), "Pause");

        app_data->is_EOS = FALSE;
        return;
    }

    GstStateChangeReturn gret = gst_element_get_state(GST_ELEMENT (app_data->pipeline), NULL, NULL, GST_CLOCK_TIME_NONE);
    LOGD (GST, "button_play_pause_clicked_cb gret %d, %s", gret, gst_element_state_change_return_get_name(gret));
    LOGD (GST, "GST_STATE() = %d", GST_STATE (GST_ELEMENT (app_data->pipeline)));

    GstState curState = GST_STATE (GST_ELEMENT (app_data->pipeline));
    if (curState == GST_STATE_PLAYING) {
        GstStateChangeReturn sret = gst_element_set_state (GST_ELEMENT (app_data->pipeline), GST_STATE_PAUSED);
        gtk_button_set_label (GTK_BUTTON (button_video_file_play_pause), "Resume");
        LOGD (GST, "set_state to PAUSE. ret: %s", gst_element_state_change_return_get_name (sret));
    } else if (curState == GST_STATE_PAUSED || curState == GST_STATE_NULL) {
        GstStateChangeReturn sret = gst_element_set_state (GST_ELEMENT (app_data->pipeline), GST_STATE_PLAYING);
        gtk_button_set_label (GTK_BUTTON (button_video_file_play_pause), "Pause");
        LOGD (GST, "set_state to PLAYING. ret: %s", gst_element_state_change_return_get_name (sret));
    } else {
        LOGD (GST, "cur plugin state is : %d", curState);
    }
}

static void button_apply_camera_command_clicked_cb (GtkButton *widget, AppData *app_data)
{
    LOGD (USR, "button_apply_camera_command_clicked_cb");
    GstStateChangeReturn gret = gst_element_get_state(GST_ELEMENT (app_data->pipeline), NULL, NULL, GST_NSECOND);
    LOGD (GST, "gret %d, %s", gret, gst_element_state_change_return_get_name(gret));

    if (gret != GST_STATE_CHANGE_ASYNC) {
        GtkWidget *entry_camera_command = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "entry_camera_command"));
        gchar *camera_command = (gchar*) gtk_entry_get_text (GTK_ENTRY (entry_camera_command));
        LOGI (GTK, "input camera command: %s", camera_command);
        app_data->camera_command = camera_command;

        restart_pipeline (app_data);
        update_pvl_parameters (app_data);
    }
}

static void button_reset_camera_command_clicked_cb (GtkButton *widget, AppData *app_data)
{
    LOGD (USR, "button_reset_camera_command_clicked_cb");

    GstStateChangeReturn gret = gst_element_get_state(GST_ELEMENT (app_data->pipeline), NULL, NULL, GST_NSECOND);
    LOGD (GST, "gret %d, %s", gret, gst_element_state_change_return_get_name(gret));

    if (gret != GST_STATE_CHANGE_ASYNC) {
        GtkWidget *entry_camera_command = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "entry_camera_command"));
        gtk_entry_set_text (GTK_ENTRY (entry_camera_command), get_camerasrc_description (app_data));
        app_data->camera_command = get_camerasrc_description (app_data);

        restart_pipeline (app_data);
        update_pvl_parameters (app_data);
    }
}

static void
filechooser_selection_changed_cb (GtkFileChooser *chooser, AppData *app_data)
{
    LOGD (USR, "filechooser_selection_changed_cb");

    gchar *file_name = gtk_file_chooser_get_filename(chooser);
    LOGD (ANY, "selected video file_name = %s", file_name);
    app_data->media_file = file_name;

    if (app_data->media_file == NULL) {
        LOGW (ANY, "media_file is null.");
        return;
    }

    restart_pipeline (app_data);
    //should be updated after create pipeline
    set_widget_pvl_static_value (app_data);
    set_widget_pvl_default_value (app_data);

    GtkWidget* button_video_file_play_pause = GTK_WIDGET (gtk_builder_get_object (app_data->builder,
            "button_video_file_play_pause"));
    gtk_button_set_label (GTK_BUTTON(button_video_file_play_pause), "Pause");
}

static void
filechooser_selection_changed_fr_database_cb (GtkFileChooser *chooser, AppData *app_data)
{
    gchar *file_name = gtk_file_chooser_get_filename(chooser);
    LOGD (GTK, "selected fr database file_name = %s", file_name);
    app_data->fr_database_file = file_name;

    if (app_data->fr_database_file != NULL) {
        GstElement *element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_FR);
        g_object_set (element, "database_file", app_data->fr_database_file, NULL);
    }
}

static void
update_gtk_setting_widgets_for_input_src (AppData *app_data)
{
    gint desired_width, desired_height;
    if (app_data->input_src == CAMERA) {
        desired_width = app_data->width;
        desired_height = app_data->height;
    } else {    //vieo file
        desired_width = DRAWING_WIDTH;
        desired_height = DRAWING_HEIGHT;
    }

    LOGD (GTK, "desired w/h [%d/%d] , app_data->w/h [%d/%d]\n", desired_width, desired_height, app_data->width, app_data->height);

    if (app_data->width != desired_width || app_data->height != desired_height) {
        app_data->width = desired_width;
        app_data->height = desired_height;

        gtk_widget_set_size_request (app_data->drawingarea_preview, app_data->width, app_data->height);
    }

    GtkWidget *expander_camera = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "expander_camera"));
    GtkWidget *expander_video_file = GTK_WIDGET (gtk_builder_get_object (app_data->builder, "expander_video_file"));

    if (app_data->input_src == CAMERA) {
        gtk_expander_set_expanded (GTK_EXPANDER (expander_camera), TRUE);
        gtk_expander_set_expanded (GTK_EXPANDER (expander_video_file), FALSE);
        gtk_widget_set_sensitive (expander_camera, TRUE);
        gtk_widget_set_sensitive (expander_video_file, FALSE);
    } else {
        gtk_expander_set_expanded (GTK_EXPANDER (expander_camera), FALSE);
        gtk_expander_set_expanded (GTK_EXPANDER (expander_video_file), TRUE);
        gtk_widget_set_sensitive (expander_camera, FALSE);
        gtk_widget_set_sensitive (expander_video_file, TRUE);
    }
}

static void
draw_overlay_cb (GstElement *overlay, cairo_t *cr,
        guint64 timestamp, guint64 duration, AppData *app_data)
{
    int fd_count = app_data->detected_num[PVL_FD_ITER];
    int fr_count = app_data->detected_num[PVL_FR_ITER];
    int pd_count = app_data->detected_num[PVL_PD_ITER];
    int ot_count = app_data->detected_num[PVL_OT_ITER];

    float w_w = 1.f;    //width weight
    float h_w = 1.f;    //height weight
    if (app_data->resize_video_analytics && app_data->parallelized_video_analytics) {
        w_w = app_data->resize_video_analytics_w_ratio;
        h_w = app_data->resize_video_analytics_h_ratio;
    }

    if (fr_count > 0) {
        rectangle *fr_rect = app_data->detected_info[PVL_FR_ITER];
        cairo_set_line_width (cr, 2);

        int i;
        for (i = 0; i < fr_count; i++) {

            int j;
            int find_id_index = -1;
            for (j = 0; j < app_data->mapped_fr_id_name_count; j++) {
                if (fr_rect[i].person_id == app_data->mapped_fr_id_name[j].id) {
                    find_id_index = j;
                    break;
                }
            }

            // select color to draw.
            if (fr_rect[i].person_id > 0) {
                cairo_set_source_rgb (cr, 0, 0, 1);
            } else {
                cairo_set_source_rgb (cr, 1, 0, 0);
            }

            // draw face rectangles.
            cairo_move_to (cr, fr_rect[i].left * w_w, fr_rect[i].top * h_w);
            cairo_line_to (cr, fr_rect[i].right * w_w, fr_rect[i].top * h_w);
            cairo_line_to (cr, fr_rect[i].right * w_w, fr_rect[i].bottom * h_w);
            cairo_line_to (cr, fr_rect[i].left * w_w, fr_rect[i].bottom * h_w);
            cairo_line_to (cr, fr_rect[i].left * w_w, fr_rect[i].top * h_w);
            cairo_stroke (cr);

            if (fr_rect[i].person_id > 0) {
                cairo_move_to (cr, fr_rect[i].left * w_w, fr_rect[i].bottom * h_w + 20);
                cairo_set_font_size (cr, 20);
                if (find_id_index != -1) {
                    cairo_show_text (cr, app_data->mapped_fr_id_name[find_id_index].name);
                } else {
                    cairo_show_text (cr, g_strdup_printf ("id: %d", fr_rect[i].person_id));
                }
            }
        }

    } else if (fd_count > 0) {
        rectangle *fd_rect = app_data->detected_info[PVL_FD_ITER];
        cairo_set_source_rgb (cr, 0, 1, 0);
        cairo_set_line_width (cr, 2);

        int i;
        for (i = 0; i < fd_count; i++) {
            cairo_move_to (cr, fd_rect[i].left * w_w, fd_rect[i].top * h_w);
            cairo_line_to (cr, fd_rect[i].right * w_w, fd_rect[i].top * h_w);
            cairo_line_to (cr, fd_rect[i].right * w_w, fd_rect[i].bottom * h_w);
            cairo_line_to (cr, fd_rect[i].left * w_w, fd_rect[i].bottom * h_w);
            cairo_line_to (cr, fd_rect[i].left * w_w, fd_rect[i].top * h_w);
        }
        cairo_stroke (cr);
    }

    if (pd_count > 0) {
        rectangle *pd_rect = app_data->detected_info[PVL_PD_ITER];
        cairo_set_source_rgb (cr, 1, 1, 0);
        cairo_set_line_width (cr, 2);

        int i;
        for (i = 0; i < pd_count; i++) {
            cairo_move_to (cr, pd_rect[i].left * w_w, pd_rect[i].top * h_w);
            cairo_line_to (cr, pd_rect[i].right * w_w, pd_rect[i].top * h_w);
            cairo_line_to (cr, pd_rect[i].right * w_w, pd_rect[i].bottom * h_w);
            cairo_line_to (cr, pd_rect[i].left * w_w, pd_rect[i].bottom * h_w);
            cairo_line_to (cr, pd_rect[i].left * w_w, pd_rect[i].top * h_w);
        }
        cairo_stroke (cr);
    }

    if (ot_count > 0) {
        rectangle *ot_rect = app_data->detected_info[PVL_OT_ITER];
        cairo_set_source_rgb (cr, 0, 1, 1);
        cairo_set_line_width (cr, 2);

        int i;
        for (i = 0; i < ot_count; i++) {
            cairo_move_to (cr, ot_rect[i].left * w_w, ot_rect[i].top * h_w);
            cairo_line_to (cr, ot_rect[i].right * w_w, ot_rect[i].top * h_w);
            cairo_line_to (cr, ot_rect[i].right * w_w, ot_rect[i].bottom * h_w);
            cairo_line_to (cr, ot_rect[i].left * w_w, ot_rect[i].bottom * h_w);
            cairo_line_to (cr, ot_rect[i].left * w_w, ot_rect[i].top * h_w);
        }
        cairo_stroke (cr);
    }

    gchar s_fps[20];
    gfloat fps = print_fps();
    LOGD (RESULT, "fps: %2.2f", fps);
    sprintf (s_fps, "FPS:%2.2f", fps);

    cairo_move_to (cr, app_data->src_width * 0.02, app_data->src_height * 0.05 + 20);
    cairo_set_font_size (cr, app_data->font_size);
    cairo_text_path (cr, s_fps);

    cairo_set_source_rgb (cr, 1, 1, 1); //white
    cairo_fill_preserve (cr);

    cairo_set_source_rgb (cr, 0, 0, 0); //black
    cairo_set_line_width (cr, 1);
    cairo_stroke (cr);
}

static gboolean
bus_cb (GstBus *bus, GstMessage *message, AppData *app_data)
{
    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING:
    case GST_MESSAGE_INFO:
        gst_message_print (message);
        break;
    case GST_MESSAGE_ASYNC_DONE:
        LOGD (GST, "GST_MESSAGE_ASYNC_DONE");
        break;
    case GST_MESSAGE_EOS:
        LOGD (GST, "GST_MESSAGE_EOS");
        if (app_data->pipeline != NULL) {
            GstStateChangeReturn sret = gst_element_set_state (GST_ELEMENT (app_data->pipeline), GST_STATE_NULL);
            LOGD (GST, "set_state to NULL. ret: %s", gst_element_state_change_return_get_name (sret));
        }
        if (app_data->input_src == VIDEO_FILE) {
            if (app_data->loop_video) {
                create_pipeline (app_data);
                update_pvl_parameters (app_data);

                GstElement *sink_element = gst_bin_get_by_name (GST_BIN (app_data->pipeline), FILTER_NAME_SINK);
                gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink_element), app_data->window_handle);
                GstStateChangeReturn sret = gst_element_set_state (GST_ELEMENT (app_data->pipeline), GST_STATE_PLAYING);
                LOGD (GST, "set_state to PLAYING. ret: %s", gst_element_state_change_return_get_name (sret));
            } else {
                app_data->is_EOS = TRUE;
                GtkWidget *button_video_file_play_pause = GTK_WIDGET (gtk_builder_get_object (app_data->builder,
                            "button_video_file_play_pause"));
                gtk_button_set_label (GTK_BUTTON(button_video_file_play_pause), "Play");
            }
        }
        break;
    case GST_MESSAGE_ELEMENT: {
        gint num_faces;
        const GstStructure *structure = gst_message_get_structure(message);

        if (strcmp (gst_structure_get_name (structure), FILTER_NAME_FD) == 0) {
            gst_structure_get_int (structure, "num_faces", &num_faces);

            if (gst_structure_get_int (structure, "num_faces", &num_faces)
                    && !(app_data->pvl_filters_type & PVL_FR)) {
                app_data->detected_num[PVL_FD_ITER] = num_faces;
                if (num_faces > 0) {
                    const GValue *fd_value = gst_structure_get_value (structure, "faces");
                    gint size = (gint)gst_value_list_get_size (fd_value);
                    if (num_faces != size) {
                        LOGW (RESULT, "num_faces: %d / list size: %d", num_faces, size);
                    }

                    gint i;
                    rectangle *rect = app_data->detected_info[PVL_FD_ITER];
                    for (i = 0; i < num_faces; i++) {
                        const GValue *faces_value = gst_value_list_get_value (fd_value, i);
                        const GstStructure *faces_structure =
                            gst_value_get_structure (faces_value);

                        gst_structure_get_int (faces_structure, "left", &rect[i].left);
                        gst_structure_get_int (faces_structure, "top", &rect[i].top);
                        gst_structure_get_int (faces_structure, "right", &rect[i].right);
                        gst_structure_get_int (faces_structure, "bottom", &rect[i].bottom);

                        LOGD (RESULT, "[%d/%d] face rect = (%d,%d,%d,%d)", i, num_faces,
                                rect[i].left,
                                rect[i].top,
                                rect[i].right,
                                rect[i].bottom);
                    }
                }
            }
        } else if (strcmp (gst_structure_get_name (structure), FILTER_NAME_FR) == 0) {
            gint num_faces;
            if (gst_structure_get_int (structure, "num_faces", &num_faces)) {
                app_data->detected_num[PVL_FR_ITER] = num_faces;
                if (num_faces > 0) {
                    const GValue *fr_value = gst_structure_get_value (structure, "faces");
                    gint size = (gint)gst_value_list_get_size (fr_value);
                    if (num_faces != size) {
                        LOGW (RESULT, "num_faces: %d / list size: %d", num_faces, size);
                    }

                    rectangle *rect = app_data->detected_info[PVL_FR_ITER];
                    gint i;
                    for (i = 0; i < num_faces; i++) {
                        const GValue *faces_value = gst_value_list_get_value (fr_value, i);
                        const GstStructure *faces_structure = 
                            gst_value_get_structure (faces_value);

                        gst_structure_get_int (faces_structure, "left", &rect[i].left);
                        gst_structure_get_int (faces_structure, "top", &rect[i].top);
                        gst_structure_get_int (faces_structure, "right", &rect[i].right);
                        gst_structure_get_int (faces_structure, "bottom", &rect[i].bottom);
                        gst_structure_get_int (faces_structure, "person_id", &rect[i].person_id);

                        LOGD (RESULT, "[%d/%d] face rect = (%d,%d,%d,%d) | person_id = %d | rectWH = (%d,%d)",
                                i,
                                num_faces,
                                rect[i].left,
                                rect[i].top,
                                rect[i].right,
                                rect[i].bottom,
                                rect[i].person_id,
                                rect[i].right-rect[i].left,
                                rect[i].bottom-rect[i].top);
                    }
                    app_data->detected_num[PVL_FR_ITER] = num_faces;
                }
            }
        } else if (strcmp (gst_structure_get_name (structure), FILTER_NAME_PD) == 0) {
            gint num_pedestrians;
            if (gst_structure_get_int (structure, "num_pedestrians", &num_pedestrians)) {
                app_data->detected_num[PVL_PD_ITER] = num_pedestrians;
                if (num_pedestrians > 0) {
                    const GValue *pd_value;
                    pd_value = gst_structure_get_value (structure, "pedestrians");
                    gint size = (gint)gst_value_list_get_size (pd_value);
                    if (num_pedestrians != size) {
                        LOGW (RESULT, "num_pedestrians: %d / list size: %d", num_pedestrians, size);
                    }
                    rectangle *rect = app_data->detected_info[PVL_PD_ITER];
                    gint i;
                    for (i = 0; i < num_pedestrians; i++) {
                        const GValue *obj_value = gst_value_list_get_value (pd_value, i);
                        const GstStructure *objs_structure = gst_value_get_structure (obj_value);
                        gboolean tracking_id;
                        gst_structure_get_int (objs_structure, "left", &rect[i].left);
                        gst_structure_get_int (objs_structure, "top", &rect[i].top);
                        gst_structure_get_int (objs_structure, "right", &rect[i].right);
                        gst_structure_get_int (objs_structure, "bottom", &rect[i].bottom);
                        gst_structure_get_int (objs_structure, "tracking_id", &tracking_id);

                        LOGD (RESULT, "[%d/%d] pedestrian rect = (%d,%d,%d,%d), tracking id = (%d) ",
                                i,
                                num_pedestrians,
                                rect[i].left,
                                rect[i].top,
                                rect[i].right,
                                rect[i].bottom,
                                tracking_id);
                    }
                }
            }
        } else if (strcmp (gst_structure_get_name (structure), FILTER_NAME_OT) == 0) {
            gint num_obj;

            if (gst_structure_get_int (structure, "num_objects", &num_obj)) {
                app_data->detected_num[PVL_OT_ITER] = num_obj;
                if (num_obj > 0) {
                    const GValue *ot_value;
                    ot_value = gst_structure_get_value (structure, "objects");
                    gint size = (gint)gst_value_list_get_size (ot_value);
                    if (num_obj != size) {
                        LOGW (RESULT, "num_objs: %d / list size: %d", num_obj, size);
                    }
                    rectangle *rect = app_data->detected_info[PVL_OT_ITER];
                    gint i;
                    for (i = 0; i < num_obj; i++) {
                        const GValue *obj_value = gst_value_list_get_value (ot_value, i);
                        const GstStructure *objs_structure = gst_value_get_structure (obj_value);
                        gboolean is_tracking_id;
                        gst_structure_get_int (objs_structure, "left", &rect[i].left);
                        gst_structure_get_int (objs_structure, "top", &rect[i].top);
                        gst_structure_get_int (objs_structure, "right", &rect[i].right);
                        gst_structure_get_int (objs_structure, "bottom", &rect[i].bottom);
                        gst_structure_get_boolean (objs_structure, "is_tracking_id",
                                &is_tracking_id);

                        LOGD (RESULT, "[%d/%d] obj rect = (%d,%d,%d,%d) : is_tracking %d",
                                i,
                                num_obj,
                                rect[i].left,
                                rect[i].top,
                                rect[i].right,
                                rect[i].bottom,
                                is_tracking_id);
                    }
                }
            }
        }
        break;
    }
    default:
        break;
    }
    return TRUE;
}

static void log_handler (const Tag tag, const gchar *domain, GLogLevelFlags log_level, const gchar *file_name, gint line_num, const gchar *func_name, const gchar *format, ...)
{
    const gchar *env_log_level;
    env_log_level = g_getenv ("G_MESSAGES_DEBUG");

    if ((log_level & ALWAYS_ALERT_LEVEL) == 0
            && (env_log_level == NULL || strcmp (env_log_level, "all") != 0 || (app_tag & tag) == 0)) {
        return;
    }

    gchar message[MAX_LOG_BUFFER];
    va_list args;
    va_start (args, format);
    vsprintf (message, format, args);
    va_end (args);

    GTimeVal time_val;
    g_get_current_time (&time_val);
    gchar *time = g_time_val_to_iso8601 (&time_val);

    switch (log_level) {
    case G_LOG_LEVEL_ERROR:
        g_print ("%s ** (%s): \033[1;31mERROR\033[0m \033[1;41m[%s:%d:%s()] %s\033[0m\n", time, domain, file_name, line_num, func_name, message);
//        g_assert (FALSE);
        break;
    case G_LOG_LEVEL_CRITICAL:
        g_print ("%s ** (%s): \033[1;31mCRITICAL\033[0m \033[1;41m[%s:%d:%s()] %s\033[0m\n", time, domain, file_name, line_num, func_name, message);
//        g_assert (FALSE);
        break;
    case G_LOG_LEVEL_WARNING:
        g_print ("%s ** (%s): \033[1;33mWARNING\033[0m [%s:%d:%s()] %s\n", time, domain, file_name, line_num, func_name, message);
        break;
    case G_LOG_LEVEL_INFO:
        g_print ("%s ** (%s): \033[1;32mINFO\033[0m [%s:%d:%s()] %s\n", time, domain, file_name, line_num, func_name, message);
        break;
    case G_LOG_LEVEL_DEBUG:
        g_print ("%s ** (%s): \033[1;34mDEBUG\033[0m [%s:%d:%s()] %s\n", time, domain, file_name, line_num, func_name, message);
        break;
    default:
        return;
    }
}

static void
window_closed_cb (GtkWidget *widget, GdkEvent *event, AppData *app_data)
{
    LOGD (GTK, "window_closed!");
    gtk_widget_hide (widget);
    gst_element_set_state (app_data->pipeline, GST_STATE_NULL);
    g_source_remove (app_data->bus_watch_id);

    gtk_main_quit ();
}
