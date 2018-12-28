cmake_minimum_required(VERSION 3.0)

project( LUASQLITE C )

add_library(lsql SHARED lsql.c sqlite3.c sqlite3.h)

target_link_libraries(lsql pthread)

set_target_properties(lsql PROPERTIES PREFIX "")

install(TARGETS lsql DESTINATION $ENV{ROCKS_LIB})

