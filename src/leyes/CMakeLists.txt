cmake_minimum_required(VERSION 3.0)

project( LUAFINDEYES CXX )

find_package( OpenCV REQUIRED )

add_library(leyes SHARED leyes.cpp)

target_link_libraries(leyes ${OpenCV_LIBS})

set_target_properties(leyes PROPERTIES PREFIX "")

install(TARGETS leyes DESTINATION "$ENV{ROCKS_LIB}")

