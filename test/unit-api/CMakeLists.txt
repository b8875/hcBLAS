# Object libraries require CMAKE 2.8.8 version 
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.8) 
MESSAGE(STATUS "CMAKE VERSION ${CMAKE_VERSION}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# Find HCC compiler
FIND_PACKAGE(HC++ 1.0 REQUIRED)

file(GLOB SRCS *.cpp)

execute_process(COMMAND ${HCC_CONFIG} --install --cxxflags
                            OUTPUT_VARIABLE HCC_CXXFLAGS)
execute_process(COMMAND ${HCC_CONFIG}  --install --ldflags
                            OUTPUT_VARIABLE HCC_LDFLAGS)

SET(HCBLAS_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/")
SET(HCBLAS_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib/src")

string(STRIP "${HCC_CXXFLAGS}" HCC_CXXFLAGS)
string(STRIP "${HCC_LDFLAGS}" HCC_LDFLAGS )
set (HCC_CXXFLAGS "${HCC_CXXFLAGS} -I${HCBLAS_INCLUDE_PATH}")
set (HCC_LDFLAGS "${HCC_LDFLAGS} -amdgpu-target=gfx803 -amdgpu-target=gfx900 -L${HCBLAS_LIBRARY_PATH} -Wl,-rpath,${HIP_PATH}/lib")

SET_PROPERTY(SOURCE ${SRCS} APPEND_STRING PROPERTY COMPILE_FLAGS " ${HCC_CXXFLAGS} -DGTEST_HAS_TR1_TUPLE=0 ")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
ADD_EXECUTABLE(unit-api-test ${SRCS})
SET(LINK "-lhc_am -lblas")
SET_PROPERTY(TARGET unit-api-test APPEND_STRING PROPERTY LINK_FLAGS " ${HCC_LDFLAGS} ${LINK}")

TARGET_LINK_LIBRARIES(unit-api-test hcblas)

add_test(NAME unit-api-test COMMAND ./bin/unit-api-test)
