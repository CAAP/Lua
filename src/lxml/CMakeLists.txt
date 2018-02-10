cmake_minimum_required(VERSION 1.0)

project( LUAXML CXX )

include_directories(${LUA_INC})

add_library(lxml SHARED lxml.cpp tinyxml2.h tinyxml2.cpp)

find_library(LUA_LIBRARY
    NAMES lua53)

if(LUA_LIBRARY)
    target_link_libraries(lxml ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

set_target_properties(lxml PROPERTIES PREFIX "")

install(TARGETS lxml DESTINATION ${ROCKS_LIB})

