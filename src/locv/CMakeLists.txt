cmake_minimum_required(VERSION 3.0)

project( LUAOPENCV CXX )

include_directories(${LUA_INC} ${OCV_INC} ${DCM_INC})

add_library(locv SHARED locv.cpp)

find_library(OCV_LIBRARY
    NAMES opencv_core opencv_imgcodecs opencv_imgproc opencv_videoio
    PATHS ${OCV_LIB})

find_library(LUA_LIBRARY
    NAMES liblua.a
    PATHS ${LUA_LIB})

if(LUA_LIBRARY)
    target_link_libraries(locv ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

if(OCV_LIBRARY)
    target_link_libraries(locv ${OCV_LIBRARY})
else(OCV_LIBRARY)
    message(FATAL_ERROR "CMake could not find OpenCV Library")
endif(OCV_LIBRARY)

set_target_properties(locv PROPERTIES PREFIX "")

install(TARGETS locv DESTINATION "${LUA_LIB}/lua/5.3")

