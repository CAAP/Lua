cmake_minimum_required(VERSION 3.0)

project( LUASQLITE C )

add_library(lsql SHARED lsql.c)

find_library(SQLITE_LIBRARY
    NAMES sqlite3)

if(SQLITE_LIBRARY)
	target_link_libraries(lsql ${SQLITE_LIBRARY})
else(SQLITE_LIBRARY)
	message(FATAL_ERROR "CMake could not find SQLITE Library")
endif(SQLITE_LIBRARY)

set_target_properties(lsql PROPERTIES PREFIX "")

install(TARGETS lsql DESTINATION $ENV{ROCKS_LIB})

