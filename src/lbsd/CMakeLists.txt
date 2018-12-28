cmake_minimum_required(VERSION 3.0)

project( LUABSD C )

add_library(lbsd SHARED lbsd.c)

set_target_properties(lbsd PROPERTIES PREFIX "")

install(TARGETS lbsd DESTINATION $ENV{ROCKS_LIB})

