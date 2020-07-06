cmake_minimum_required(VERSION 3.0)

project( LUAZEROMQ C )

add_library(lzmq SHARED lzmq.c)

find_library(ZMQ_LIBRARY
    NAMES zmq)

if(ZMQ_LIBRARY)
	target_link_libraries(lzmq ${ZMQ_LIBRARY})
else(ZMQ_LIBRARY)
	message(FATAL_ERROR "CMake could not find ZEROMQ Library")
endif(ZMQ_LIBRARY)

set_target_properties(lzmq PROPERTIES PREFIX "")

install(TARGETS lzmq DESTINATION $ENV{ROCKS_LIB})

