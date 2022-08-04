#Copy this file to the application project folder as CMakeLists.txt
cmake_minimum_required (VERSION 3.12)

set(RAM_SIZE 65536)
project(dhrystone CXX C)

cmsdk_app_target(RELEASE ${PROJECT_NAME} "" release ${CMSDK_ARCH})
cmsdk_add_subdirectory(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src)
message(STATUS "Stratify API Test Executable ${RELEASE_TARGET}")
add_executable(${RELEASE_TARGET})
target_sources(${RELEASE_TARGET}
	PRIVATE
	${SOURCE_FILES}
	${CMAKE_CURRENT_SOURCE_DIR}/sl_settings.json
	${CMAKE_CURRENT_SOURCE_DIR}/README.md
	)

target_compile_options(${RELEASE_TARGET}
	PRIVATE
	-Os
	)

set_property(TARGET ${RELEASE_TARGET} PROPERTY CXX_STANDARD 17)

set(DEPENDENCIES SysAPI TestAPI FsAPI)

cmsdk_app_add_arch_targets("${RELEASE_OPTIONS}" "${DEPENDENCIES}" ${RAM_SIZE})
if(SOS_IS_LINK)
	cmsdk_app_add_arch_targets("${COVERAGE_OPTIONS}" "${DEPENDENCIES}" ${RAM_SIZE})
endif()

target_compile_options(${RELEASE_TARGET}
	PRIVATE
	-Os
	)

