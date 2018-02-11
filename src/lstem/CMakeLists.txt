cmake_minimum_required(VERSION 3.0)

project( LUA_LIBSTEM C )

include_directories($ENV{LUA_INC})

file(GLOB SNOWBALL
    src_c/*.c
    runtime/api.c
    runtime/utilities.c
    libstemmer/libstemmer.c
)

add_library(lstem SHARED lstem.c ${SNOWBALL})

find_library(LUA_LIBRARY
    NAMES lua53)

if(LUA_LIBRARY)
    target_link_libraries(lstem ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

set_target_properties(lstem PROPERTIES PREFIX "")

install(TARGETS lstem DESTINATION "$ENV{ROCKS_LIB}")

