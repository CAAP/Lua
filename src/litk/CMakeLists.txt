cmake_minimum_required(VERSION 3.0)

project( LUAITKLIB CXX )

find_package(ITK)
include(${ITK_USE_FILE})

add_library(litk SHARED litk.cxx)

target_link_libraries(litk ${ITK_LIBRARIES})

set_target_properties(litk PROPERTIES PREFIX "")

install(TARGETS litk DESTINATION $ENV{ROCKS_LIB})

