LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	montest.cpp \
	gpscollector.cpp \
	sensorcollector.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils libhardware

LOCAL_MODULE:= montest

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
