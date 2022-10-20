project(testsuite CXX C)
include(CTest)

add_subdirectory(fstest)
add_subdirectory(dhrystone)
add_subdirectory(mathtest)
add_subdirectory(posixtest)
add_subdirectory(drivetool)
add_subdirectory(i2ctool)
add_subdirectory(signalprocess)

