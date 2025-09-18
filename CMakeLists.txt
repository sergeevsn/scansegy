cmake_minimum_required(VERSION 3.16)
project(scansegy VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Enable SIMD optimizations if available
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
endif()

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(OpenMP REQUIRED)

# Find matplotplusplus
set(MATPLOTPP_BUILD_EXAMPLES OFF CACHE BOOL "Build matplotplusplus examples")
set(MATPLOTPP_BUILD_TESTS OFF CACHE BOOL "Build matplotplusplus tests")
add_subdirectory(matplotplusplus)

# Include directories
include_directories(src)
include_directories(src/segyread)

# Source files
set(SOURCES
    src/main.cpp
    src/segyscanner.cpp
    src/segyread/SegyReader.cpp
    src/segyread/SegyUtil.cpp
)

# Create executable
add_executable(scansegy ${SOURCES})

# Link libraries
target_link_libraries(scansegy 
    matplot
    OpenMP::OpenMP_CXX
)

# Set target properties
set_target_properties(scansegy PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

# Installation
install(TARGETS scansegy DESTINATION bin)

# Print configuration summary
message(STATUS "Configuration Summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  OpenMP: ${OpenMP_CXX_FOUND}")
message(STATUS "  Matplot++: ${matplot_FOUND}")
