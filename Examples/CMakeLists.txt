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

# Rendering Demo - Showcases all rendering features
add_executable(RenderingDemo
    RenderingDemo.cpp
)

target_link_libraries(RenderingDemo PRIVATE KayaEngine)

target_include_directories(RenderingDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
)
