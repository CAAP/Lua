cmake_minimum_required(VERSION 3.0)

project( LUA_LAPACKE C )

include_directories(${LUA_INC})

add_library(llapack SHARED llapack.c)

find_library(LPK_LIBRARY
    NAMES lapack lapacke)

find_library(LUA_LIBRARY
    NAMES liblua.a
    PATHS ${LUA_LIB})

if(LUA_LIBRARY)
    target_link_libraries(llapack ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

if(LPK_LIBRARY)
    target_link_libraries(llapack ${LPK_LIBRARY})
else(LPK_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lapacke Library")
endif(LPK_LIBRARY)

set_target_properties(llapack PROPERTIES PREFIX "")

install(TARGETS llapack DESTINATION "${LUA_LIB}/lua/5.3")

