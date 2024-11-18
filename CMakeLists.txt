# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(MySPIM C)

# Set the C standard
set(CMAKE_C_STANDARD 99)

# Add source files
set(SOURCES
        spimcore.c
        project.c
)

# Include directories for header files
include_directories(${CMAKE_SOURCE_DIR})

# Add the executable target
add_executable(MySPIM ${SOURCES})

# Ensure the linker uses the console subsystem (important for avoiding WinMain errors)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem,console")
