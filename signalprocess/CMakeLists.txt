#Copy this file to the application project folder as CMakeLists.txt
cmake_minimum_required (VERSION 3.12)


set(RAM_SIZE 8192)
project(signalprocess CXX C)

sos_sdk_add_subdirectory(SOURCE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src)
sos_sdk_app_target(RELEASE ${PROJECT_NAME} "" release ${SOS_ARCH})
message(STATUS "Stratify API Test Executable ${RELEASE_TARGET}")
add_executable(${RELEASE_TARGET})
target_sources(${RELEASE_TARGET}
	PRIVATE
	${SOURCE_LIST}
	${CMAKE_CURRENT_SOURCE_DIR}/sl_settings.json
	)

target_compile_options(${RELEASE_TARGET}
	PRIVATE
	-Os
	)

set_property(TARGET ${RELEASE_TARGET} PROPERTY CXX_STANDARD 17)

set(DEPENDENCIES SosAPI ThreadAPI SysAPI TestAPI FsAPI)

sos_sdk_app_add_arch_targets("${RELEASE_OPTIONS}" "${DEPENDENCIES}" ${RAM_SIZE})
if(SOS_IS_LINK)
	sos_sdk_app_add_arch_targets("${COVERAGE_OPTIONS}" "${DEPENDENCIES}" ${RAM_SIZE})
endif()

target_compile_options(${RELEASE_TARGET}
	PRIVATE
	-Os
	)

