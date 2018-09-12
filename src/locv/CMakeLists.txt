cmake_minimum_required(VERSION 3.0)

project( LUAOPENCV CXX )

find_package( OpenCV REQUIRED )

add_library(locv SHARED locv.cpp)

target_link_libraries(locv ${OpenCV_LIBS})

set_target_properties(locv PROPERTIES PREFIX "")

install(TARGETS locv DESTINATION "$ENV{ROCKS_LIB}")

