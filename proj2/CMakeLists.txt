cmake_minimum_required(VERSION 3.15)
project(proj2)

set(CMAKE_CXX_STANDARD 14)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

add_executable(proj2 mm.cpp)

target_link_libraries(proj2 ${MPI_LIBRARIES})

if(MPI_COMPILE_FLAGS)
    set_target_properties(proj2 PROPERTIES
            COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(proj2 PROPERTIES
            LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()