cmake_minimum_required(VERSION 3.10)

project(TestSWFRecompiled)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")

set(RUNTIME_INCLUDES
    ${CMAKE_SOURCE_DIR}/../SWFModernRuntime/include
    ${CMAKE_SOURCE_DIR}/../SWFModernRuntime/include/actionmodern
    ${CMAKE_SOURCE_DIR}/../SWFModernRuntime/include/libswf
)



# RecompiledTags - Library containing the primary recompiler output
add_library(RecompiledTags STATIC)

target_include_directories(RecompiledTags PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/RecompiledScripts
    ${RUNTIME_INCLUDES}
)

file(GLOB FUNC_C_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledTags/*.c)

target_sources(RecompiledTags PRIVATE ${FUNC_C_SOURCES})



# RecompiledScripts - Library containing the secondary recompiler output for actions/abc
add_library(RecompiledScripts STATIC)

target_include_directories(RecompiledScripts PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${RUNTIME_INCLUDES}
)

file(GLOB FUNC_C_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledScripts/*.c)

target_sources(RecompiledScripts PRIVATE ${FUNC_C_SOURCES})



add_executable(${PROJECT_NAME})

target_compile_options(${PROJECT_NAME} PRIVATE -O2)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/main.c
)

set(RENAME_ZCONF OFF)

target_link_libraries(${PROJECT_NAME} PRIVATE
    RecompiledTags
    RecompiledScripts
    ${PROJECT_SOURCE_DIR}/SWFModernRuntime.lib
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${RUNTIME_INCLUDES}
)