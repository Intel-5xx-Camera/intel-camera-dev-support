#intel-camera-dev-support

_intel-camera-dev-support_ containts the following components:

##  android_support

Sources and headers needed for using the AOSP camera3 API in linux. Essentially
an implementation of the dependencies of the AOSP camera3 API.

##  icamera_adapter

Adapter between the gstreamer icamerasrc element and the AOSP camera3 API.
Allows running an AOSP camera3 implementation in a gstreamer pipeline.

##  unittests

Google Test (gtest) based camera subsystem tests which also show how to use the
camera3 API to do e.g. streaming or jpeg captures.

Tailored for the Intel 5xx module. Run with "camera\_subsys\_test_linux.sh".

##  pvl

Gstreamer wrapper plugin for the Photographic Vision Libraries (PVL) and a GUI
application for testing them.

#Documentation

The AOSP camera3 HAL API is documented at:

https://source.android.com/devices/camera/camera3.html

#Build

Use the bitbake recipes from:

https://github.com/01org/meta-intel-camera

#Dependencies

You will need these to use _intel-camera-dev-support_:

https://github.com/01org/meta-intel-camera

https://github.com/01org/intel-camera-drivers

https://github.com/Intel-5xx-Camera/intel-camera-adaptation

https://github.com/01org/icamerasrc

#References

https://github.com/google/googletest

https://gstreamer.freedesktop.org/
