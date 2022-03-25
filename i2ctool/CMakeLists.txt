#Copy this file to the application project folder as CMakeLists.txt
cmake_minimum_required (VERSION 3.12)

# This will set the default RAM used by the application
set(RAM_SIZE 16384)
project(i2ctool CXX C)

#Add sources to the project
sos_sdk_add_subdirectory(SOS_SOURCELIST ${CMAKE_CURRENT_SOURCE_DIR}/src)
list(
	APPEND
	SOS_SOURCELIST
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/sl_settings.json
	${CMAKE_CURRENT_SOURCE_DIR}/README.md)

sos_sdk_app_target(RELEASE ${PROJECT_NAME} "" release ${SOS_ARCH})
add_executable(${RELEASE_TARGET})
target_sources(${RELEASE_TARGET}
	PRIVATE
	src/main.cpp)

target_compile_options(${RELEASE_TARGET}
	PRIVATE
	-O3
	)

set_property(TARGET ${RELEASE_TARGET} PROPERTY CXX_STANDARD 17)


set(DEPENDENCIES HalAPI VarAPI StratifyOS_crt)

sos_sdk_app_add_arch_targets("${RELEASE_OPTIONS}" "${DEPENDENCIES}" ${RAM_SIZE})
