cmsdk2_minimum_required(2.1.2)

project(testsuite
  LANGUAGES CXX C
  VERSION 0.0.1)

include(CTest)

add_subdirectory(fstest)
add_subdirectory(dhrystone)
add_subdirectory(mathtest)
add_subdirectory(posixtest)
add_subdirectory(drivetool)
add_subdirectory(i2ctool)
add_subdirectory(signalprocess)

