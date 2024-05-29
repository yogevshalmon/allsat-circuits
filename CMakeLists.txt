cmake_minimum_required(VERSION 3.8)
project(AllSatEnumerAIG
        LANGUAGES CXX)

# check gcc version
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0.0")
        message(FATAL_ERROR "Insufficient gcc version")
    endif()
endif()

# define colors for messages
if(NOT WIN32)
    string(ASCII 27 Esc)
    set(ColourReset "${Esc}[m")
    set(ColourBold  "${Esc}[1m")
    set(Red         "${Esc}[31m")
    set(Green       "${Esc}[32m")
    set(Yellow      "${Esc}[33m")
    set(Blue        "${Esc}[34m")
    set(Magenta     "${Esc}[35m")
    set(Cyan        "${Esc}[36m")
    set(White       "${Esc}[37m")
    set(BoldRed     "${Esc}[1;31m")
    set(BoldGreen   "${Esc}[1;32m")
    set(BoldYellow  "${Esc}[1;33m")
    set(BoldBlue    "${Esc}[1;34m")
    set(BoldMagenta "${Esc}[1;35m")
    set(BoldCyan    "${Esc}[1;36m")
    set(BoldWhite   "${Esc}[1;37m")
endif()

# C++20
set(CMAKE_CXX_STANDARD 20)

# Custom install prefix for libraries
set(LIB_PREFIX ${CMAKE_SOURCE_DIR}/libs)

set(CRYPTOMINISAT_LIB_PREFIX ${CMAKE_SOURCE_DIR}/libs/sat/cryptominisat)

OPTION(USE_DEBUG "If to compile the tool with debug information" OFF) # Disabled by default

OPTION(USE_64b_INDEX_SOLVER "If to compile the tool with 64 bit index mode" OFF) # Disabled by default

OPTION(USE_COMPRESS_SOLVER "If to compile the tool with compress mode" OFF) # Disabled by default

IF(USE_DEBUG)
    message(STATUS "${Green}Prepare tool to be build in DEBUG mode${ColourReset}")
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
ENDIF(USE_DEBUG)


# Set release mode to default
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

IF(USE_64b_INDEX_SOLVER)
    message(STATUS "${BoldBlue}Prepare tool to be build in 64b index mode${ColourReset}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSAT_SOLVER_INDEX_64")
ENDIF(USE_64b_INDEX_SOLVER)

IF(USE_COMPRESS_SOLVER)
    message(STATUS "${BoldBlue}Prepare tool to be build in compress mode${ColourReset}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSAT_SOLVER_COMPRESS")
ENDIF(USE_COMPRESS_SOLVER)

# ipasir

set(IPASIR_SAT_SOLVER "CADICAL" CACHE STRING "which sat_solver to use [CADICAL, CRYPTOMINISAT, MERGESAT]")

set_property(CACHE IPASIR_SAT_SOLVER PROPERTY STRINGS CADICAL CRYPTOMINISAT MERGESAT)


# Include header files
# TODO only src use part path with include
include_directories(src)

# Find source files
file(GLOB_RECURSE SOURCES src/main.cpp src/*/*.cpp src/*/*.hpp)

# Build executable with aiger
add_executable(hall_tool ${SOURCES})

# Include header files
include_directories(${LIB_PREFIX}/intel_sat_solver) 

IF(USE_DEBUG)
    set(SOLVER_BUILD_MODE libd)
ELSE()
    set(SOLVER_BUILD_MODE libr)
ENDIF(USE_DEBUG)

add_custom_target(intel_sat_solver

    COMMAND make ${SOLVER_BUILD_MODE}

   WORKING_DIRECTORY ${LIB_PREFIX}/intel_sat_solver/
)

add_subdirectory(${LIB_PREFIX})

# cadical section

if(IPASIR_SAT_SOLVER STREQUAL "CADICAL")
    message(STATUS "${BoldBlue}Use CADICAL as IPASIR SAT solver${ColourReset}")

    add_definitions(-DIPASIR_SAT_SOLVER_CADICAL)

    # Include lib header files
    include_directories(${LIB_PREFIX}/sat/cadical/src) 

    # TODO call this only once instead every make
    # Add a custom target for manual configuration
    add_custom_target(cadical_configure
    
        COMMAND configure

        WORKING_DIRECTORY ${LIB_PREFIX}/sat/cadical
    )

    add_custom_target(ipasir_sat_solver

        COMMAND make

        DEPENDS cadical_configure

        WORKING_DIRECTORY ${LIB_PREFIX}/sat/cadical
    )
    # Important link cadical before intel_sat_solver since both implement also ipasir and we want cadical to be used not intel_sat_solver
    target_link_libraries(hall_tool PRIVATE lorina ${LIB_PREFIX}/sat/cadical/build/libcadical.a ${LIB_PREFIX}/intel_sat_solver/libintel_sat_solver.a)
    # Important link cadical before intel_sat_solver since both implement also ipasir and we want cadical to be used not intel_sat_solver
    add_dependencies(hall_tool ipasir_sat_solver intel_sat_solver)


elseif(IPASIR_SAT_SOLVER STREQUAL "CRYPTOMINISAT")
    message(STATUS "${BoldBlue}Use CRYPTOMINISAT as IPASIR SAT solver${ColourReset}")

    add_definitions(-DIPASIR_SAT_SOLVER_CRYPTOMINISAT)
    
    set(IPASIR ON CACHE BOOL "" FORCE)

    # include all the sat directory should be apllied to all but only cryptominisat have cmake.
    add_subdirectory(${CRYPTOMINISAT_LIB_PREFIX})

    target_link_libraries(hall_tool PRIVATE lorina ipasircryptominisat5 ${LIB_PREFIX}/intel_sat_solver/libintel_sat_solver.a)

    add_dependencies(hall_tool intel_sat_solver)

elseif(IPASIR_SAT_SOLVER STREQUAL "MERGESAT")
    message(STATUS "${BoldBlue}Use MERGESAT as SAT solver${ColourReset}")

    set(CMAKE_THREAD_PREFER_PTHREAD ON)                                                                                                                                                                                                           
    set(THREADS_PREFER_PTHREAD_FLAG ON)                                                                                                                                                                                                           
    find_package(Threads REQUIRED) 

    add_definitions(-DIPASIR_SAT_SOLVER_MERGESAT)

    # Include lib header files
    # include_directories(${LIB_PREFIX}/sat/mergesat/core) 

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

    set(SOLVER_BUILD_FLAGS AR=gcc-ar)

    set(SOLVER_BUILD_MODE r)

    add_custom_target(ipasir_sat_solver

        COMMAND make ${SOLVER_BUILD_FLAGS} ${SOLVER_BUILD_MODE}

        WORKING_DIRECTORY ${LIB_PREFIX}/sat/mergesat
    )
    
    target_link_libraries(hall_tool PRIVATE lorina ${LIB_PREFIX}/sat/mergesat/build/release/lib/libmergesat.a ${LIB_PREFIX}/intel_sat_solver/libintel_sat_solver.a)

    add_dependencies(hall_tool ipasir_sat_solver intel_sat_solver)

else() # default only intel_sat_solver
    message(STATUS "${BoldBlue}Do not use IPASIR SAT solver${ColourReset}")

    target_link_libraries(hall_tool PRIVATE lorina ${LIB_PREFIX}/intel_sat_solver/libintel_sat_solver.a)

    add_dependencies(hall_tool intel_sat_solver)
endif()

