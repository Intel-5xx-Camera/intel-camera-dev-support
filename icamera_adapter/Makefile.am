AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libicamera_adapter.la

headerinstalldir = ${prefix}/include
headerinstall_HEADERS = \
    ICamera.h \
    Parameters.h \
    ScopedAtrace.h

ALLSRC = LibraryFunctions.cpp \
         ICameraAdapter.cpp \
         ParameterAdapter.cpp \
         Parameters.cpp \
         LogHelper.cpp

libicamera_adapter_la_SOURCES = $(ALLSRC)

libicamera_adapter_la_LIBADD = -lcamerahal

libicamera_adapter_la_CPPFLAGS = -std=c++11

#includes
libicamera_adapter_la_CPPFLAGS += \
    $(LIBUTILS_CFLAGS) \
    -DICAMERA_DEBUG
