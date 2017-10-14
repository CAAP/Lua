cmake_minimum_required(VERSION 3.0)

project( LUA_CDF C )

include_directories(${LUA_INC})

add_library(lcdf SHARED dcdflib.c ipmpar.c lcdf.c)

find_library(LUA_LIBRARY
    NAMES liblua.a
    PATHS ${LUA_LIB})

if(LUA_LIBRARY)
    target_link_libraries(lcdf ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

set_target_properties(lcdf PROPERTIES PREFIX "")

install(TARGETS lcdf DESTINATION "${LUA_LIB}/lua/5.3")

