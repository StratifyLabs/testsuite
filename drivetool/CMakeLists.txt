cmake_minimum_required (VERSION 3.12)

set(RAM_SIZE 32768)
project(drivetool CXX C)
cmsdk2_add_executable(
	NAME ${PROJECT_NAME}
	CONFIG release
	ARCH ${CMSDK_ARCH}
	SUFFIX .elf
	TARGET RELEASE_TARGET)
target_sources(${RELEASE_TARGET}
	PRIVATE
	src/main.cpp
	sl_settings.json
	README.md)
set_property(TARGET ${RELEASE_TARGET} PROPERTY CXX_STANDARD 17)
cmsdk2_app_add_dependencies(
	TARGET ${RELEASE_TARGET}
	DEPENDENCIES HalAPI SysAPI TestAPI
	RAM_SIZE ${RAM_SIZE}
	ARCHITECTURES ${CMSDK_ARCH_LIST})

