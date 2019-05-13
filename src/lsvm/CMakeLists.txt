cmake_minimum_required(VERSION 3.0)

project( LUASVM C CXX)

add_library(lsvm SHARED svm.cpp lsvm.c)


set_target_properties(lsvm PROPERTIES PREFIX "")

install(TARGETS lsvm DESTINATION $ENV{ROCKS_LIB})

