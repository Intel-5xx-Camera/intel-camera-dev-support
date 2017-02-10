plugin_LTLIBRARIES = libgst_pvl_filter.la

##############################################################################

# sources used to compile this plug-in
libgst_pvl_filter_la_SOURCES = \
        gst_pvl_util.c \
        gst_pvl_filter.c \
        gst_pvl_face_detection_filter.c \
        gst_pvl_face_recognition_filter.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgst_pvl_filter_la_CFLAGS = \
        $(GST_CFLAGS) \
        $(PVL_FACE_OBJECT_CFLAGS) \
        -I$(top_srcdir)/gst-libs/pvl

libgst_pvl_filter_la_LIBADD = \
        -lpvl_face_detection \
        -lpvl_face_recognition \
        $(GST_LIBS) \
        -lgstvideo-1.0 \
        $(PVL_FACE_OBJECT_LIBS)
libgst_pvl_filter_la_LDFLAGS = \
        $(GST_PLUGIN_LIBS) \
        $(PVL_FACE_OBJECT_LIBS) \
        -L$(top_builddir)/gst-libs/pvl/.libs -lgst_pvl_util

libgst_pvl_filter_la_LIBTOOLFLAGS = \
        --tag=disable-static


