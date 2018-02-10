cmake_minimum_required(VERSION 3.0)

project( LUASNAPPY CXX )

include_directories($ENV{LUA_INC})

add_library(lsnap SHARED snappy.cc)

find_library(LSP_LIBRARY
    NAMES snappy)

find_library(LUA_LIBRARY
    NAMES lua53)

if(LUA_LIBRARY)
    target_link_libraries(lsnap ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

if(LSP_LIBRARY)
	target_link_libraries(lsnap ${LSP_LIBRARY})
else(LSP_LIBRARY)
	message(FATAL_ERROR "CMake could not find Snappy Library")
endif(LSP_LIBRARY)

set_target_properties(lsnap PROPERTIES PREFIX "")

install(TARGETS lsnap DESTINATION $ENV{ROCKS_LIB})

