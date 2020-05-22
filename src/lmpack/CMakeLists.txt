cmake_minimum_required(VERSION 3.0)

project( LUA_INTS C )

add_library(lmpack SHARED lmpack.c)

target_link_libraries(lmpack msgpackc)

set_target_properties(lmpack PROPERTIES PREFIX "")

install(TARGETS lmpack DESTINATION $ENV{ROCKS_LIB})

