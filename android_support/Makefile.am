AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = .

lib_LTLIBRARIES = \
    libtimers.la \
    libcutils.la \
    libutils.la \
    libgralloc.la \
    libui.la \
    libcamera_client.la \
    libcamera_metadata.la

COMMON_SOURCES =  cutils/hashmap.c \
    cutils/native_handle.c \
    cutils/config_utils.c \
    cutils/load_file.c \
    cutils/strlcpy.c \
    cutils/open_memstream.c \
    cutils/strdup16to8.c \
    cutils/strdup8to16.c \
    cutils/record_stream.c \
    cutils/process_name.c \
    cutils/threads.c \
    cutils/sched_policy.c \
    cutils/iosched_policy.c \
    cutils/str_parms.c \
    cutils/fs.c \
    cutils/multiuser.c \
    cutils/socket_inaddr_any_server.c \
    cutils/socket_local_client.c \
    cutils/socket_local_server.c \
    cutils/socket_loopback_client.c \
    cutils/socket_loopback_server.c \
    cutils/socket_network_client.c \
    cutils/sockets.c

COMMON_HOST_SOURCES = \
        cutils/ashmem-host.c \
        cutils/trace-host.c \
        cutils/dlmalloc_stubs.c

CUTILS_INCLUDES = \
    -I$(srcdir)/androidheaders \
    -I$(srcdir)/androidheaders/system/core/include

libcutils_la_SOURCES = $(COMMON_SOURCES) $(COMMON_HOST_SOURCES)
libcutils_la_CPPFLAGS = -std=gnu90 $(CUTILS_INCLUDES)

## Export android support headers
androidsupportdir = ${prefix}/include/

nobase_androidsupport_HEADERS = \
    androidheaders/ui/Fence.h \
    androidheaders/ui/GraphicBuffer.h \
    androidheaders/ui/GraphicBufferAllocator.h \
    androidheaders/ui/GraphicBufferMapper.h \
    androidheaders/ui/ANativeObjectBase.h \
    androidheaders/ui/PixelFormat.h \
    androidheaders/ui/Rect.h \
    androidheaders/ui/Point.h \
    androidheaders/camera/CameraMetadata.h \
    androidheaders/hardware/camera3.h \
    androidheaders/hardware/camera_common.h \
    androidheaders/hardware/gralloc.h \
    androidheaders/hardware/hardware.h \
    androidheaders/hardware/fb.h \
    androidheaders/mediautils/SchedulingPolicyService.h \
    androidheaders/utils/Errors.h \
    androidheaders/utils/SortedVector.h \
    androidheaders/utils/KeyedVector.h \
    androidheaders/utils/Vector.h \
    androidheaders/utils/VectorImpl.h \
    androidheaders/utils/TypeHelpers.h \
    androidheaders/utils/String8.h \
    androidheaders/utils/String16.h \
    androidheaders/utils/Unicode.h \
    androidheaders/utils/Debug.h \
    androidheaders/utils/CallStack.h \
    androidheaders/utils/RefBase.h \
    androidheaders/utils/StrongPointer.h \
    androidheaders/utils/SharedBuffer.h \
    androidheaders/utils/Flattenable.h \
    androidheaders/utils/Log.h \
    androidheaders/utils/List.h \
    androidheaders/utils/Timers.h \
    androidheaders/utils/Printer.h \
    androidheaders/utils/Compat.h \
    androidheaders/utils/threads.h \
    androidheaders/utils/Thread.h \
    androidheaders/utils/AndroidThreads.h \
    androidheaders/utils/ThreadDefs.h \
    androidheaders/utils/Trace.h \
    androidheaders/utils/Singleton.h \
    androidheaders/utils/Condition.h \
    androidheaders/utils/Mutex.h \
    androidheaders/utils/RWLock.h \
    androidheaders/cutils/log.h \
    androidheaders/cutils/compiler.h \
    androidheaders/cutils/atomic.h \
    androidheaders/cutils/intel_atomic.h \
    androidheaders/stdatomic.h \
    androidheaders/cutils/native_handle.h \
    androidheaders/cutils/trace.h \
    androidheaders/cutils/properties.h \
    androidheaders/log/log.h \
    androidheaders/log/logd.h \
    androidheaders/log/uio.h \
    androidheaders/system/camera_vendor_tags.h \
    androidheaders/system/camera_metadata_tags.h \
    androidheaders/system/camera_metadata.h \
    androidheaders/system/core/include/backtrace/backtrace_constants.h \
    androidheaders/system/core/include/backtrace/Backtrace.h \
    androidheaders/system/core/include/backtrace/BacktraceMap.h \
    androidheaders/system/media/private/camera/include/camera_metadata_hidden.h \
    androidheaders/system/window.h \
    androidheaders/system/camera.h \
    androidheaders/system/thread_defs.h \
    androidheaders/system/graphics.h \
    androidheaders/sys/system_properties.h \
    androidheaders/binder/MemoryHeapBase.h \
    androidheaders/binder/IMemory.h \
    androidheaders/binder/IBinder.h \
    androidheaders/binder/Binder.h \
    androidheaders/binder/IInterface.h \
    androidheaders/binder/IServiceManager.h \
    androidheaders/binder/IPermissionController.h \
    androidheaders/binder/ProcessState.h \
    androidheaders/android/log.h \
    androidheaders/android/rect.h

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = libutils.pc

libutils_la_SOURCES = \
    Threads.cpp \
    VectorImpl.cpp \
    String8.cpp \
    String16.cpp \
    Unicode.cpp \
    RefBase.cpp \
    SharedBuffer.cpp \
    Static.cpp \
    Printer.cpp \
    Properties.cpp \
    Log.cpp \
    Scheduling.cpp \
    CallStack.cpp

libutils_la_LIBADD = -lpthread libcutils.la libtimers.la

libutils_la_CPPFLAGS = \
    -std=gnu++11 \
    -DOS_PATH_SEPARATOR=\'/\' \
    -I$(srcdir)/androidheaders \
    -I$(srcdir)/androidheaders/system/media/camera/include \
    -I$(srcdir)/androidheaders/system/media/private/camera/include \
    -I$(srcdir)/androidheaders/external/safe-iop/include \
    -I$(srcdir)/androidheaders/system/core/include

libtimers_la_SOURCES = \
    Timers.cpp

# We have to compile Timers.cpp separately
# to get proper timespace for non-Android env as well.
libtimers_la_CPPFLAGS = \
    $(libutils_la_CPPFLAGS) \
    -DHAVE_ANDROID_OS

#### GRALLOC ####
libgralloc_la_SOURCES = \
    gralloc/gralloc.cpp \
    gralloc/framebuffer.cpp \
    gralloc/mapper.cpp

libgralloc_la_CPPFLAGS = \
    -std=gnu++11 \
    -I$(srcdir)/androidheaders

libgralloc_la_LIBADD = libcutils.la

#### LIB UI #####
libui_la_SOURCES = \
    ui/GraphicBuffer.cpp \
    ui/GraphicBufferAllocator.cpp \
    ui/GraphicBufferMapper.cpp \
    ui/Rect.cpp \
    ui/PixelFormat.cpp \
    ui/Fence.cpp

libui_la_CPPFLAGS = \
    -std=gnu++11 \
    -I$(srcdir)/androidheaders \
    -I$(srcdir)/gralloc

libui_la_LIBADD = libgralloc.la libcutils.la libutils.la

# Unit test app
bin_PROGRAMS = libui_unittests
libui_unittests_SOURCES = ui/tests/src/heap-gralloc-test.cpp

GTEST_SRC = /usr/src/gtest
TEST_INCLUDES = -I$(srcdir)/androidheaders
libui_unittests_CPPFLAGS = $(CPPHACKS) $(TEST_INCLUDES)
libui_unittests_LDADD = \
    libui.la -lutils -lpthread -lgtest -lgtest_main

libcamera_client_la_SOURCES = CameraMetadata.cpp
libcamera_client_la_CPPFLAGS = \
    -D__u32=uint32_t \
    -D__u64=uint64_t \
    -D__s32=int32_t \
    -D__s64=int64_t \
    -D__u8=uint8_t \
    -std=c++11 \
    -I$(srcdir)/androidheaders \
    -I$(srcdir)/androidheaders/system/media/camera/include \
    -I$(srcdir)/androidheaders/system/media/private/camera/include

libcamera_metadata_la_SOURCES = camera_metadata.c
libcamera_metadata_la_CPPFLAGS = \
    -std=c99 \
    -I$(srcdir)/androidheaders \
    -I$(srcdir)/androidheaders/system/media/camera/include \
    -I$(srcdir)/androidheaders/system/media/private/camera/include
