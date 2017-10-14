cmake_minimum_required(VERSION 3.0)

project( LUA_DGST C )

include_directories(${LUA_INC})

add_library(ldgst SHARED ldgst.c)

find_library(DGST_LIBRARY
    NAMES crypto)

find_library(LUA_LIBRARY
    NAMES liblua.a
    PATHS ${LUA_LIB})

if(LUA_LIBRARY)
    target_link_libraries(ldgst ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

if(DGST_LIBRARY)
    target_link_libraries(ldgst ${DGST_LIBRARY})
else(DGST_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lapacke Library")
endif(DGST_LIBRARY)

set_target_properties(ldgst PROPERTIES PREFIX "")

install(TARGETS ldgst DESTINATION "${LUA_LIB}/lua/5.3")

