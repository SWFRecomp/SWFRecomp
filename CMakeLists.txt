cmake_minimum_required(VERSION 3.10)

project(SWFRecomp)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")

add_executable(${PROJECT_NAME})

set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/recompilation.cpp
    ${CMAKE_SOURCE_DIR}/src/swf.cpp
    ${CMAKE_SOURCE_DIR}/src/tag.cpp
    ${CMAKE_SOURCE_DIR}/src/field.cpp
    ${CMAKE_SOURCE_DIR}/src/action/action.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE)

set(RENAME_ZCONF OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/zlib)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/lzma)

target_link_libraries(${PROJECT_NAME} PRIVATE
    zlibstatic
    lzma
)

# frick u ninja
if (${CMAKE_GENERATOR} MATCHES "Ninja")
set(CONFIG_DIR .)
else()
set(CONFIG_DIR $<CONFIG>)
endif()

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/action
    zlib
    lzma/liblzma/api
)