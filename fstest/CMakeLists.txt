#Copy this file to the application project folder as CMakeLists.txt
cmake_minimum_required (VERSION 3.12)

set(RAM_SIZE 65536)
project(fstest
	LANGUAGES CXX C
	VERSION 1.0)
cmsdk2_add_executable(
	NAME ${PROJECT_NAME}
	CONFIG release
	ARCH ${CMSDK_ARCH}
	SUFFIX .elf
	TARGET RELEASE_TARGET)
target_sources(${RELEASE_TARGET}
	PRIVATE
	src/main.cpp
	src/FileTest.cpp
	src/FileTest.hpp
	src/DirTest.cpp
	src/DirTest.hpp
	sl_settings.json
	README.md)
set_property(TARGET ${RELEASE_TARGET} PROPERTY CXX_STANDARD 17)
set(DEPENDENCIES SysAPI TestAPI FsAPI)
cmsdk2_app_add_dependencies(
	TARGET ${RELEASE_TARGET}
	DEPENDENCIES FsAPI SysAPI TestAPI
	RAM_SIZE ${RAM_SIZE}
	ARCHITECTURES ${CMSDK_ARCH_LIST})


