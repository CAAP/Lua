cmake_minimum_required(VERSION 3.0)

project( LUAITKLIB CXX )

find_package(ITK)
include(${ITK_USE_FILE})

add_library(litk SHARED litk.cxx)

target_link_libraries(litk ${ITK_LIBRARIES})

find_library(LUA_LIBRARY
    NAMES lua)

if(LUA_LIBRARY)
    target_link_libraries(litk ${LUA_LIBRARY})
else(LUA_LIBRARY)
    message(FATAL_ERROR "CMake could not find Lua Library")
endif(LUA_LIBRARY)

find_library(GDCM_LIBRARY
    NAMES gdcmCommon)

if(GDCM_LIBRARY)
	target_link_libraries(litk ${GDCM_LIBRARY})
else(GDCM_LIBRARY)
	message(FATAL_ERROR "CMake could not find GDCM Library")
endif(GDCM_LIBRARY)

set_target_properties(litk PROPERTIES PREFIX "")

install(TARGETS litk DESTINATION $ENV{ROCKS_LIB})

