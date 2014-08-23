LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := debug eng

LOCAL_MODULE := test_communication_task.elf

LOCAL_C_INCLUDES := ../../../include

LOCAL_LDLIBS := ../../../tz_api/libs/armeabi/tz_api.so

LOCAL_SRC_FILES = main.c

include $(BUILD_EXECUTABLE)
