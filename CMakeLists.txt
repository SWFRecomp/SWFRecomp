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

set(RENAME_ZCONF OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/zlib)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/lzma)

target_link_libraries(SWFRecomp PRIVATE
    zlib
    lzma
)

target_sources(SWFRecomp PRIVATE ${SOURCES})
target_include_directories(SWFRecomp PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    zlib
    lzma/liblzma/api
)