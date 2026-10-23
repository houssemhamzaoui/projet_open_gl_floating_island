cmake_minimum_required(VERSION 3.26)
project(FloatingIsland LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(glfw3 REQUIRED)
find_package(glm   REQUIRED)

add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

add_library(stb STATIC external/stb/stb_impl.c)
target_include_directories(stb PUBLIC external/stb)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE glad stb glfw glm::glm)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -static)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:RESOURCE_ROOT="${CMAKE_SOURCE_DIR}">)

target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:-mwindows>)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS)

add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/assets  $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying shaders and assets next to the executable")
add_dependencies(${PROJECT_NAME} copy_resources)

set(DIST_DIR ${CMAKE_SOURCE_DIR}/dist/FloatingIsland)
add_custom_target(dist
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${DIST_DIR}/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets  ${DIST_DIR}/assets
    COMMAND_EXPAND_LISTS
    COMMENT "Creating ${DIST_DIR}")
add_dependencies(dist ${PROJECT_NAME})
