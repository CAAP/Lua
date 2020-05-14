cmake_minimum_required(VERSION 3.0)

project( LUA_INTS C )

add_library(lints SHARED lints.c)

set_target_properties(lints PROPERTIES PREFIX "")

install(TARGETS lints DESTINATION $ENV{ROCKS_LIB})

