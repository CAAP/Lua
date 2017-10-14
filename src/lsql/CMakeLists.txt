cmake_minimum_required(VERSION 3.0)

project( LUASQLITE C )

include_directories(${LUA_INC} ${SQL_INC})

add_library(lsql SHARED lsql.c)

find_library(SQL_LIBRARY
    NAMES sqlite3
    PATHS ${SQL_LIB})

find_library(LUA_LIBRARY
    NAMES liblua.a
    PATHS ${LUA_LIB})

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

install(TARGETS lsql DESTINATION "${LUA_LIB}/lua/5.3")

