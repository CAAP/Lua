cmake_minimum_required(VERSION 3.0)

project( LUAZEROMQ C )

add_library(lzmq SHARED lzmq.c)

target_link_libraries(lzmq "/usr/local/lib/libzmq.so.5.1")

set_target_properties(lzmq PROPERTIES PREFIX "")

install(TARGETS lzmq DESTINATION $ENV{ROCKS_LIB})

