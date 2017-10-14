cmake_minimum_required(VERSION 3.0)

project( LUAGDCM CXX )

include_directories(${LUA_INC} ${GDCM_INC})

add_library(lgdcm SHARED anonymizer.cpp lgdcm.cpp)

find_library(GDCM_LIBRARY
    NAMES gdcmCommon gdcmMSFF
    PATHS ${GDCM_LIB})

find_library(LUA_LIBRARY
    NAMES liblua.a
    PATHS ${LUA_LIB})

if(LUA_LIBRARY)
    target_link_libraries(lgdcm ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

if(GDCM_LIBRARY)
    target_link_libraries(lgdcm ${GDCM_LIBRARY})
else(GDCM_LIBRARY)
    message(FATAL_ERROR "CMake could not find Sqlite3 Library")
endif(GDCM_LIBRARY)

set_target_properties(lgdcm PROPERTIES PREFIX "")

install(TARGETS lgdcm DESTINATION "${LUA_LIB}/lua/5.3")

