cmake_minimum_required(VERSION 3.8)
project(AllSatEnumerAIG
        LANGUAGES CXX)

# C++20
set(CMAKE_CXX_STANDARD 20)

# Custom install prefix for libraries
set(LIB_PREFIX ${CMAKE_SOURCE_DIR}/libs)

# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
# set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
    

# Set release mode to default
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()


# Include header files
include_directories(src/allsatenumer)

# Find source files
file(GLOB_RECURSE SOURCES src/allsatenumer/*.cpp src/allsatenumer/*.hpp)

# Build executable with aiger
add_executable(allsatenumer-aig ${SOURCES})

# Include header files
include_directories(${LIB_PREFIX}/intel_sat_solver) 

add_custom_target(intel_sat_solver

   COMMAND make libr

   WORKING_DIRECTORY ${LIB_PREFIX}/intel_sat_solver/
)

add_subdirectory(${LIB_PREFIX})

target_link_libraries(allsatenumer-aig PRIVATE lorina ${LIB_PREFIX}/intel_sat_solver/libintel_sat_solver.a)

add_dependencies(allsatenumer-aig intel_sat_solver)
