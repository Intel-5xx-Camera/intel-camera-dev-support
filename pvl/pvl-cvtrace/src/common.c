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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"

static gint frames = 0;
static gfloat fps = 0.f;
static gint64 start_time = 0;

void
set_app_log (gboolean enable)
{
    if (enable) {
        g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);
    } else {
        g_setenv ("G_MESSAGES_DEBUG", "", TRUE);
    }
}

void
set_plugin_log (gboolean enable)
{
    if (enable) {
        gst_debug_set_threshold_for_name ("pvl_*", GST_LEVEL_DEBUG);
    } else {
        gst_debug_set_threshold_for_name ("*", GST_LEVEL_ERROR);
    }
}

gfloat
print_fps()
{
    frames++;
    gint64 cur_time = g_get_monotonic_time();
    if (start_time + 1000000 < cur_time) {
        fps = (gfloat) frames * 1000000 / (cur_time - start_time);
        start_time = cur_time;
        frames = 0;
    }
    return fps;
}

void
gst_message_print (GstMessage *message)
{
    GError *err;
    gchar *debug;
    gchar *str;

    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error (message, &err, &debug);
        break;
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning (message, &err, &debug);
        break;
    case GST_MESSAGE_INFO:
        gst_message_parse_info (message, &err, &debug);
        break;
    default:
        return;
    }

    str = gst_element_get_name (message->src);
    g_warning ("%s %s: %s", str, GST_MESSAGE_TYPE_NAME(message), err->message);
    g_debug ("Debug: %s", debug);
    g_free (str);

    g_error_free(err);
    g_free (debug);
}

GParamSpecUInt*
get_gparam_spec_uint (GstElement *target_class, const gchar *property_name)
{
    GParamSpec * gparam_spec = g_object_class_find_property (G_OBJECT_GET_CLASS(target_class), property_name);
    if (gparam_spec) {
        GParamSpecUInt *gparam_spec_uint = G_PARAM_SPEC_UINT (gparam_spec);
        return gparam_spec_uint;
    } else {
        g_warning ("%s is null", property_name);
    }
    return NULL;
}

GParamSpecFloat*
get_gparam_spec_float (GstElement *target_class, const gchar *property_name)
{
    GParamSpec * gparam_spec = g_object_class_find_property (G_OBJECT_GET_CLASS(target_class), property_name);
    if (gparam_spec) {
        GParamSpecFloat *gparam_spec_float = G_PARAM_SPEC_FLOAT (gparam_spec);
        return gparam_spec_float;
    } else {
        g_warning ("%s is null", property_name);
    }
    return NULL;
}

guint
check_verify_available_input (const gchar* source, gint min, gint max)
{
    char *pend;
    const long value = strtoul (source, &pend, 10);

    if (*pend != '\0') {
        return ERROR;
    }
    if (value < min || value > max) {
        return ERROR;
    }
    return (gint)value;
}

const gchar*
get_pvl_filter_name (PvlFilterType type)
{
    switch (type) {
        case PVL_FD: return FILTER_NAME_FD;
        case PVL_FR: return FILTER_NAME_FR;
        case PVL_OT: return FILTER_NAME_OT;
        case PVL_PD: return FILTER_NAME_PD;
        case PVL_NONE:
        default: return "NONE";
    }
}

const PvlFilterType
get_pvl_filter_type_enum (const gchar* filter_str)
{
    PvlFilterType filter_type = PVL_NONE;
    gchar* filter_new = g_ascii_strup(filter_str, -1);
    if (strcmp ("NONE", filter_new) == 0) {
        filter_type = PVL_NONE;
    } else if (strcmp (FILTER_NAME_FD, filter_new) == 0) {
        filter_type = PVL_FD;
    } else if (strcmp (FILTER_NAME_FR, filter_new) == 0) {
        filter_type = PVL_FR;
    } else if (strcmp (FILTER_NAME_OT, filter_new) == 0) {
        filter_type = PVL_OT;
    } else if (strcmp (FILTER_NAME_PD, filter_new) == 0) {
        filter_type = PVL_PD;
    }
    g_free (filter_new);
    return filter_type;
}

const gint
get_pvl_filters_type (gchar** filters_str)
{
    gint filters_type = 0;
    int i;
    for (i = 0; filters_str[i] != NULL ; i++) {
        gint filter_type = 0;
        g_debug ("i: %d, filters_str : %s", i, filters_str[i]);

        if (strcmp (FILTER_NAME_FD, filters_str[i]) == 0) {
            filter_type = PVL_FD;
        } else if (strcmp (FILTER_NAME_FR, filters_str[i]) == 0) {
            filter_type = PVL_FR;
        } else if (strcmp (FILTER_NAME_OT, filters_str[i]) == 0) {
            filter_type = PVL_OT;
        } else if (strcmp (FILTER_NAME_PD, filters_str[i]) == 0) {
            filter_type = PVL_PD;
        }
        filters_type |= filter_type;
    }

    return filters_type;
}

const gchar*
get_format_name (Format fmt)
{
    switch (fmt) {
        case FMT_YV12: return "YV12";
        case FMT_YUY2: return "YUY2";
        case FMT_NV21: return "NV21";
        case FMT_NV12: return "NV12";
        case FMT_GRAY8: return "GRAY8";
        case FMT_RGBx: return "RGBx";
        case FMT_RGBA: return "RGBA";
        default: return "V21";
    }
}

const Format
get_format_enum (const gchar* fmt_str)
{
    Format format = FMT_YV12;
    if (fmt_str == NULL) {
        return format;
    }

    gchar* fmt_new = g_ascii_strup(fmt_str, -1);

    if (strcmp ("YV12", fmt_new) == 0) {
        format = FMT_YV12;
    } else if (strcmp ("YUY2", fmt_new) == 0) {
        format = FMT_YUY2;
    } else if (strcmp ("NV21", fmt_new) == 0) {
        format = FMT_NV21;
    } else if (strcmp ("NV12", fmt_new) == 0) {
        format = FMT_NV12;
    } else if (strcmp ("GRAY8", fmt_new) == 0) {
        format = FMT_GRAY8;
    } else if (strcmp ("RGBX", fmt_new) == 0) {
        format = FMT_RGBx;
    }

    g_free (fmt_new);

    return format;
}

/*
 * Sample code for GST
 */

//guint           gst_value_list_get_size         (const GValue   *value);
//const GValue *  gst_value_list_get_value        (const GValue   *value,

/* Functions below print the Capabilities in a human-friendly format */
static gboolean print_field(GQuark a_field, const GValue *a_value, gpointer a_pfx) {
    gchar *gstring = gst_value_serialize (a_value);
    g_print("%s  %15s: %s\n", (gchar *) a_pfx, g_quark_to_string(a_field), gstring);

    g_free(gstring);

    return TRUE;
}

static void print_caps (const GstCaps *the_caps, const gchar *a_pfx) {
    guint index;

    g_return_if_fail(the_caps != NULL);

    if (gst_caps_is_any(the_caps)) {
        g_print ("%sANY\n", a_pfx);
        return;
    }
    if (gst_caps_is_empty(the_caps)) {
        g_print ("%sEMPTY\n", a_pfx);
        return;
    }

    for (index = 0; index < gst_caps_get_size(the_caps); index++) {
        gint width, height, fps_num, fps_den;
        GstStructure *structure = gst_caps_get_structure(the_caps, index);

        g_print ("%s%s\n", a_pfx, gst_structure_get_name(structure));
        gst_structure_foreach (structure, print_field, (gpointer) a_pfx);

        gst_structure_get_int(structure, "width", &width);
        gst_structure_get_int(structure, "height", &height);
        gst_structure_get_fraction(structure, "framerate", &fps_num, &fps_den);
        const gchar *format = gst_structure_get_string(structure, "format");
        g_print ("%s: w(%d) h(%d) fps(%d/%d) format(%s)\n", a_pfx, width, height, fps_num, fps_den, format);
    }
}

/* how to use.
 * GstElementFactory *factory = gst_element_factory_find ("v4l2src");
 * print_pad_templates_information (factory);
 */
/* Prints information about a Pad Template, including its Capabilities */
void print_pad_templates_information (const gchar *element_name) {
    const GList *padtemplates;
    GstStaticPadTemplate *padtempl;
    GstElementFactory *factory = gst_element_factory_find (element_name);
    if (factory == NULL) {
        g_error ("%s can't be found.", element_name);
        return;
    }

    g_print ("Pad Templates for %s:\n", gst_element_factory_get_longname (factory));

    if (!gst_element_factory_get_num_pad_templates (factory)) {
        g_print ("  none\n");
        gst_object_unref (factory);
        return;
    }

    padtemplates = gst_element_factory_get_static_pad_templates(factory);
    while (padtemplates) {
        padtempl = (GstStaticPadTemplate *) (padtemplates->data);
        padtemplates = g_list_next(padtemplates);

        if (padtempl->direction == GST_PAD_SRC)
            g_print ("  src template: '%s'\n", padtempl->name_template);
        else if (padtempl->direction == GST_PAD_SINK)
            g_print ("  sink template: '%s'\n", padtempl->name_template);
        else
            g_print ("  unknown template: '%s'\n", padtempl->name_template);

        if (padtempl->presence == GST_PAD_ALWAYS)
            g_print ("    Availability: Always\n");
        else if (padtempl->presence == GST_PAD_SOMETIMES)
            g_print ("    Availability: Sometimes\n");
        else if (padtempl->presence == GST_PAD_REQUEST) {
            g_print ("    Availability: On request\n");
        } else {
            g_print ("    Availability: UNKNOWN!!!\n");
        }

        if (padtempl->static_caps.string) {
            g_print ("    Capabilities:\n");
            print_caps (gst_static_caps_get (&padtempl->static_caps), "      ");
        }

        g_print ("\n");
    }

    gst_object_unref (factory);
}

#if 0
/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name) {
    GstPad *pad = NULL;
    GstCaps *caps = NULL;

    /* Retrieve pad */
    pad = gst_element_get_static_pad (element, pad_name);
    if (!pad) {
        g_printerr ("Could not retrieve pad '%s'\n", pad_name);
        return;
    }

    /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
    caps = gst_pad_get_current_caps (pad);

    /* Print and free */
    g_print ("Caps for the %s pad:\n", pad_name);
    print_caps (caps, "      ");
    gst_caps_unref (caps);
    gst_object_unref (pad);
}
#endif

