cmake_minimum_required(VERSION 3.20)
project(KayaGameEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(KAYA_BUILD_EXAMPLES "Build example applications" ON)

# Third-party dependencies
add_subdirectory(ThirdParty)

# Engine library
file(GLOB_RECURSE ENGINE_SOURCES
    "Source/Core/*.cpp"
    "Source/Rendering/*.cpp"
    "Source/Physics/*.cpp"
    "Source/Input/*.cpp"
    "Source/Math/*.cpp"
    "Source/Utils/*.cpp"
    "Source/Editor/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "Source/Core/*.h"
    "Source/Rendering/*.h"
    "Source/Physics/*.h"
    "Source/Input/*.h"
    "Source/Math/*.h"
    "Source/Utils/*.h"
    "Source/Editor/*.h"
)

add_library(KayaEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(KayaEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/ImGuizmo
)

target_link_libraries(KayaEngine PUBLIC
    glfw
    glad
    glm::glm
    Jolt
    imgui
    ImGuizmo
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(KayaEngine PUBLIC opengl32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(KayaEngine PUBLIC GL X11 pthread dl)
elseif(APPLE)
    find_library(OPENGL_LIBRARY OpenGL)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(KayaEngine PUBLIC
        ${OPENGL_LIBRARY}
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
endif()

# Examples
if(KAYA_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()
