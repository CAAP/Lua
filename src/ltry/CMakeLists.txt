cmake_minimum_required(VERSION 3.0)

project( LUAZEROMQ C )

add_library(ltry SHARED ltry.c)

target_link_libraries(ltry "/usr/local/lib/libzmq.so.4.2")

set_target_properties(ltry PROPERTIES PREFIX "")

install(TARGETS ltry DESTINATION $ENV{ROCKS_LIB})

