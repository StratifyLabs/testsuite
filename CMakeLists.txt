project(testsuite CXX C)
include(CTest)

add_subdirectory(fstest)
add_subdirectory(dhrystone)
add_subdirectory(mathtest)
add_subdirectory(posixtest)
add_subdirectory(signalprocess)

