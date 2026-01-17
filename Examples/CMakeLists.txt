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
