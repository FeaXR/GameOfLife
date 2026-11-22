cmake_minimum_required(VERSION 3.16)
project(GameOfLife LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# By default GLFW and GLEW come from the system (pacman/apt/vcpkg...).
# If they can't be found, or GOL_FETCH_DEPS is ON, they are downloaded and built.
option(GOL_FETCH_DEPS "Always download and build GLFW and GLEW" OFF)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

if(NOT GOL_FETCH_DEPS)
  find_package(glfw3 3.3 CONFIG QUIET)
  find_package(GLEW QUIET)
endif()

include(FetchContent)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW) # Use extraction time as timestamps for downloaded archives
endif()

if(NOT TARGET glfw)
  message(STATUS "GLFW not found on the system, fetching it")
  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw
    URL https://github.com/glfw/glfw/releases/download/3.5.1/glfw-3.5.1.zip
    URL_HASH SHA256=ea79bc5feffc254c87291980c2d0bce9acebb68c4983b79f961dcd2cb8a611a0)
  FetchContent_MakeAvailable(glfw)
endif()

set(GOL_GLEW_FETCHED OFF)
if(NOT TARGET GLEW::GLEW)
  message(STATUS "GLEW not found on the system, fetching it")
  set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(glew-cmake_BUILD_STATIC ON  CACHE BOOL "" FORCE)
  set(ONLY_LIBS               ON  CACHE BOOL "" FORCE)
  FetchContent_Declare(glew
    URL https://github.com/Perlmint/glew-cmake/archive/refs/tags/glew-cmake-2.3.1.tar.gz
    URL_HASH SHA256=70c2ce0d96d0de1199e20306ad55fdd3eef291dd776af07c270b39545906f755)
  FetchContent_MakeAvailable(glew)
  set(GOL_GLEW_FETCHED ON)
endif()

# Nuklear (single-header immediate mode GUI) for the toolbar
FetchContent_Declare(nuklear
  URL https://github.com/Immediate-Mode-UI/Nuklear/archive/refs/tags/v4.13.4.tar.gz
  URL_HASH SHA256=1ce8e78346017518810e3307d85e0b8f701e5ee2f5c34ab9203d99f2ab73ff77)
FetchContent_MakeAvailable(nuklear)

add_executable(gol
  Source/src/main.c
  Source/src/gol.c
  Source/src/shader.c
  Source/src/ui.c
  Source/src/patterns.c
)
target_include_directories(gol PRIVATE
  "${nuklear_SOURCE_DIR}"
  "${nuklear_SOURCE_DIR}/demo/glfw_opengl3")
# Third-party header implementation, don't drown our warnings in its own
if(MSVC)
  set_source_files_properties(Source/src/ui.c PROPERTIES COMPILE_OPTIONS "/wd4116;/wd5287;/wd4244;/wd4267")
else()
  set_source_files_properties(Source/src/ui.c PROPERTIES COMPILE_OPTIONS "-Wno-unused-function;-Wno-unused-parameter;-Wno-unused-but-set-variable")
endif()

if(GOL_GLEW_FETCHED)
  target_link_libraries(gol PRIVATE libglew_static)
  target_compile_definitions(gol PRIVATE GLEW_STATIC)
else()
  target_link_libraries(gol PRIVATE GLEW::GLEW)
endif()
target_link_libraries(gol PRIVATE glfw OpenGL::GL)
if(UNIX)
  target_link_libraries(gol PRIVATE m)
endif()

if(MSVC)
  target_compile_options(gol PRIVATE /W3)
  # Run from the output dir in the Visual Studio debugger so res/ is found
  set_property(TARGET gol PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:gol>")
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT gol)
else()
  target_compile_options(gol PRIVATE -Wall)
endif()

if(MINGW)
  # Self-contained exe without libgcc/winpthread DLLs
  target_link_options(gol PRIVATE -static)
endif()

# Copy the shaders and init files next to the executable
add_custom_command(TARGET gol POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/Source/res" "$<TARGET_FILE_DIR:gol>/res")

# Install with --component gol: a fetched GLEW adds install rules for its own headers and libraries
install(TARGETS gol RUNTIME DESTINATION . COMPONENT gol)
install(DIRECTORY Source/res DESTINATION . COMPONENT gol)
