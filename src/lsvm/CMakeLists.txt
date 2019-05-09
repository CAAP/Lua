cmake_minimum_required(VERSION 3.0)

project( LUASVM C )

add_library(lsvm SHARED lsvm.c)

set_target_properties(lsvm PROPERTIES PREFIX "")

install(TARGETS lsvm DESTINATION $ENV{ROCKS_LIB})

