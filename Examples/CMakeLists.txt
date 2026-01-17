cmake_minimum_required(VERSION 3.20)

# Sandbox example
add_executable(Sandbox
    Sandbox.cpp
)

target_link_libraries(Sandbox PRIVATE KayaEngine)

target_include_directories(Sandbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
)

# KayaEditor
add_executable(KayaEditor
    KayaEditor.cpp
)

target_link_libraries(KayaEditor PRIVATE KayaEngine)

target_include_directories(KayaEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
)

# FPS Example
add_executable(FPSExample
    FPSExample.cpp
)

target_link_libraries(FPSExample PRIVATE KayaEngine)

target_include_directories(FPSExample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
)

# FPS Game DLL (Hot Reloadable)
add_library(FPSGame SHARED
    FPSGame.cpp
)

target_link_libraries(FPSGame PRIVATE KayaEngine)

target_include_directories(FPSGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
)

# Set output directory to same as executables for easy loading
set_target_properties(FPSGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}
)

# Hot Reload Host
add_executable(HotReloadHost
    HotReloadHost.cpp
)

target_link_libraries(HotReloadHost PRIVATE KayaEngine)

target_include_directories(HotReloadHost PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
)
