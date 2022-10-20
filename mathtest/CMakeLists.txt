#Copy this file to the application project folder as CMakeLists.txt
cmake_minimum_required (VERSION 3.12)

set(RAM_SIZE 32768)
project(mathtest CXX C)
cmsdk2_add_executable(
	NAME ${PROJECT_NAME}
	CONFIG release
	ARCH ${CMSDK_ARCH}
	SUFFIX .elf
	TARGET RELEASE_TARGET)
cmsdk2_add_sources(
	TARGET ${RELEASE_TARGET}
	DIRECTORY src)
target_sources(${RELEASE_TARGET}
	PRIVATE
	sl_settings.json)
set_property(TARGET ${RELEASE_TARGET} PROPERTY CXX_STANDARD 17)
cmsdk2_app_add_dependencies(
	TARGET ${RELEASE_TARGET}
	DEPENDENCIES HalAPI SysAPI TestAPI
	RAM_SIZE ${RAM_SIZE}
	ARCHITECTURES ${CMSDK_ARCH_LIST})

