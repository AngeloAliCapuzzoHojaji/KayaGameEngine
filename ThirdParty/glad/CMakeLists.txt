cmake_minimum_required(VERSION 3.10)
project(GLAD C)

add_library(glad STATIC
    src/glad.c
)

target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Note: Generate GLAD from https://glad.dav1d.de/
# Settings: Language=C/C++, API gl=Version 4.6, Profile=Core
message(STATUS "GLAD: Please generate GLAD files from https://glad.dav1d.de/")
