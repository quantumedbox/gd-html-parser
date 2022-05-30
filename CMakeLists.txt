# todo: Move to SCons based solution, for easier integration with the rest of Godot tooling

cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
include(ExternalProject)
project(gd-gumbo)

# todo: Automatically set target version in godot-cpp submodule
# if(NOT DEFINED $ENV{GODOT_VERSION})
#   set(GODOT_VERSION 3.4)
# endif()

# todo:
# add_subdirectory(godot-cpp)
# todo: Invoke scons in godot-cpp?

set(GUMBO_STATIC_LIB 1)
add_subdirectory(sigil-gumbo)

if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-O0 -Wall -Wextra -g3 -ggdb)
  elseif(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(-O3 -Wall -Wextra -g0)
  endif()
else()
  message(STATUS "Your compiler isn't recognised to provide proper compile options, please edit CMakeLists :)")
endif()

set(INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/sigil-gumbo/src
  ${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/include
  ${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/include/core
  ${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/include/gen
  ${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/godot-headers)

add_library(gd-gumbo-lib SHARED src/module.cpp)

target_link_directories(gd-gumbo-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/bin)
target_link_libraries(gd-gumbo-lib PRIVATE gumbo godot-cpp.windows.release.64) # todo: Should link without assumption
target_link_options(gd-gumbo-lib PRIVATE -static-libgcc -static-libstdc++) # todo: Could potentially include standard lib as separate file? It's quite chunky to include
target_include_directories(gd-gumbo-lib PRIVATE ${INCLUDES})

set_target_properties(gd-gumbo-lib PROPERTIES
  OUTPUT_NAME "gd-gumbo"
  PREFIX ""
  VERSION "${PROJECT_VERSION}"
  PUBLIC_HEADER src/module.hpp)

if(CMAKE_EXPORT_COMPILE_COMMANDS MATCHES ON)
  add_custom_command(TARGET gd-gumbo-lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR})
endif()
