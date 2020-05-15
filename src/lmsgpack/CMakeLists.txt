cmake_minimum_required(VERSION 3.0)

project( LUA_INTS C )

add_library(lmpack SHARED lmsgpack.c)

target_link_libraries(lzmq msgpackc)

set_target_properties(lmpack PROPERTIES PREFIX "")

install(TARGETS lmpackkk DESTINATION $ENV{ROCKS_LIB})

