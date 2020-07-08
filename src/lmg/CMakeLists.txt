cmake_minimum_required(VERSION 3.0)

project( LUAMONGOOSE C )

add_library(lmg SHARED mongoose.c lmg.c)

set_target_properties(lmg PROPERTIES PREFIX "")

install(TARGETS lmg DESTINATION $ENV{ROCKS_LIB})

