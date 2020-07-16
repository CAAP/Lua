cmake_minimum_required(VERSION 3.0)

project( LUA_INTS C )

add_library(lmpack SHARED lmpack.c)

find_library(MSGPK_LIBRARY
    NAMES msgpackc)

if(MSGPK_LIBRARY)
	target_link_libraries(lmpack ${MSGPK_LIBRARY})
else(MSGPK_LIBRARY)
	message(FATAL_ERROR "CMake could not find MSGPACK Library")
endif(MSGPK_LIBRARY)

set_target_properties(lmpack PROPERTIES PREFIX "")

install(TARGETS lmpack DESTINATION $ENV{ROCKS_LIB})

