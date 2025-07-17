LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= my_server.cpp ILedService.cpp LedService.cpp 
LOCAL_SHARED_LIBRARIES :=  libutils  libbinder
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= my_server
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= my_client.cpp  ILedService.cpp
LOCAL_SHARED_LIBRARIES :=  libutils  libbinder
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= my_client
include $(BUILD_EXECUTABLE)
