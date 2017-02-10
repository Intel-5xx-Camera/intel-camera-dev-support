lib_LTLIBRARIES = libgst_pvl_util.la

# sources used to compile
libgst_pvl_util_la_SOURCES = \
        pvl_face_detection_meta.c \
        pvl_face_recognition_meta.c

# compiler and linker flags, set in configure.ac
libgst_pvl_util_la_CFLAGS = \
        $(GST_PLUGINS_BASE_CFLAGS) \
        $(GST_BASE_CFLAGS) \
        $(GST_CFLAGS)

libgst_pvl_util_la_LIBADD = \
        $(GST_LIBS)

libgst_pvl_util_la_LDFLAGS = \
        $(GST_LIB_LDFLAGS) \
        $(GST_ALL_LDFLAGS) \
        $(GST_LT_LDFLAGS)

libgst_pvl_util_la_LIBTOOLFLAGS = \
        --tag=disable-static

libgst_pvl_util_includedir = \
        $(includedir)/gstpvl

libgst_pvl_util_include_HEADERS = \
        pvl_face_detection_meta.h \
        pvl_face_recognition_meta.h


