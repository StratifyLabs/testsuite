cmake_minimum_required (VERSION 3.12)

if(NOT DEFINED API_IS_SDK)
	include(API)
	include(SosAPI)
endif()

project(testsuite CXX C)
include(CTest)

add_subdirectory(fstest)
add_subdirectory(dhrystone)
add_subdirectory(mathtest)
add_subdirectory(posixtest)
add_subdirectory(drivetool)
add_subdirectory(i2ctool)
add_subdirectory(signalprocess)

