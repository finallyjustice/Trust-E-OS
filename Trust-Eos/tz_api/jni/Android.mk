 LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := debug eng

LOCAL_MODULE := tz_api.so
LOCAL_MODULE_FILENAME := tz_api
LOCAL_C_INCLUDES := ../../include

LOCAL_LDLIBS := 

LOCAL_SRC_FILES := tz_api.c 
include $(BUILD_SHARED_LIBRARY)
