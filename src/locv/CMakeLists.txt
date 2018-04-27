cmake_minimum_required(VERSION 3.0)

project( LUAOPENCV CXX )

include_directories($ENV{INC_GDCM})

add_library(locv SHARED locv.cpp)

find_library(OCV_LIBRARY
    NAMES opencv_videoio opencv_imgcodecs opencv_imgproc opencv_core)

if(OCV_LIBRARY)
    target_link_libraries(locv ${OCV_LIBRARY})
else(OCV_LIBRARY)
    message(FATAL_ERROR "CMake could not find OpenCV Library")
endif(OCV_LIBRARY)

set_target_properties(locv PROPERTIES PREFIX "")

install(TARGETS locv DESTINATION "$ENV{ROCKS_LIB}")

