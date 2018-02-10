cmake_minimum_required(VERSION 3.0)

project( LUASQLITE C )

add_library(lsql SHARED lsql.c)

find_library(SQL_LIBRARY
    NAMES sqlite3)

find_library(LUA_LIBRARY
    NAMES lua)

if(LUA_LIBRARY)
    target_link_libraries(lsql ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

if(SQL_LIBRARY)
    target_link_libraries(lsql ${SQL_LIBRARY})
else(SQL_LIBRARY)
    message(FATAL_ERROR "CMake could not find Sqlite3 Library")
endif(SQL_LIBRARY)

set_target_properties(lsql PROPERTIES PREFIX "")

install(TARGETS lsql DESTINATION $ENV{ROCKS_LIB})

