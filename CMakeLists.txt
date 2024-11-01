cmake_minimum_required(VERSION 3.4)

project(SWFRecomp)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")

add_executable(SWFRecomp)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/swf.cpp
)

target_sources(SWFRecomp PRIVATE ${SOURCES})
target_include_directories(SWFRecomp PRIVATE ${CMAKE_SOURCE_DIR}/include)

if (WIN32)
    target_link_libraries(SWFRecomp ws2_32)
endif(WIN32)