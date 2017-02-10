AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

# Unit test app
bin_PROGRAMS = camera_subsys_test
bin_SCRIPTS = camera_subsys_test_linux.sh

camera_subsys_test_SOURCES = src/main.cpp \
                         src/hal_module_basic_tests.cpp \
                         src/raw_hal_tests.cpp \
                         src/raw_hal_test_yuv_resolution.cpp \
                         src/raw_hal_test_jpeg_resolution.cpp \
                         src/supported_streams_test.cpp \
                         src/test_parameterization.cpp \
                         src/test_stream_factory.cpp \
                         src/valgrind_test.cpp

camera_subsys_test_CPPFLAGS = -std=c++11 \
                          $(LIBUTILS_CFLAGS)

camera_subsys_test_LDADD = -lgtest \
                       -lcamerahal \
                       -lpthread \
                       -lcamera_metadata \
                       -lcamera_client \
                       -lutils \
                       -lui

# install suppression file
suppressiondir = /usr/share/camera_subsys_test
suppression_DATA = unittest.supp
