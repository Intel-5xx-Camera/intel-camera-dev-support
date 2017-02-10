# name of your binary
bin_PROGRAMS = CVTraceGUI

# list of source files
# the prefix is the name of the binary
CVTraceGUI_SOURCES = common.c cv_trace_gui.c

# list of headers we're not going to install
noinst_HEADERS = common.h

# our CFLAGS and LDFLAGS used for compiling and linking
# make sure you prefix these with the name of your binary
CVTraceGUI_CFLAGS = $(GST_CFLAGS) $(GTK3_CFLAGS)
CVTraceGUI_LDFLAGS = $(GST_LIBS) -lgstvideo-1.0 $(GTK3_LIBS)

