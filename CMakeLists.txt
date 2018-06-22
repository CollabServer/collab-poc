cmake_minimum_required(VERSION 2.8.2 FATAL_ERROR)
project(collab-example-poc)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
    message(FATAL_ERROR "in-source build not allowed. Use another directory.")
endif()


# ------------------------------------------------------------------------------
# Various / Dependencies
# ------------------------------------------------------------------------------
set(dependency_dir "${CMAKE_SOURCE_DIR}/bin/")
set(git_submodule_dir "${CMAKE_SOURCE_DIR}/gitmodules/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include("CMake/ExternalProjects/AddClientInterface.cmake")


# ------------------------------------------------------------------------------
# Executable client
# ------------------------------------------------------------------------------
message(STATUS "Building poc client executable")

include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${git_submodule_dir}/collab-data-crdts/include")
include_directories("${git_submodule_dir}/collab-client-interface/include")

file(GLOB_RECURSE srcFilesPOC "${CMAKE_SOURCE_DIR}/client/src/*.cpp")

add_executable(client_poc ${srcFilesPOC})
add_dependencies(client_poc collab-client-interface)
add_custom_target(runClientPOC client_poc)

target_link_libraries(client_poc "zmq" client_interface)


# ------------------------------------------------------------------------------
# Executable database
# ------------------------------------------------------------------------------
message(STATUS "Building poc database executable")

include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${git_submodule_dir}/collab-data-crdts/include")
include_directories("${git_submodule_dir}/collab-client-interface/include")

file(GLOB_RECURSE srcFilesPOC "${CMAKE_SOURCE_DIR}/database/src/*.cpp")

add_executable(database_poc ${srcFilesPOC})
add_dependencies(database_poc collab-client-interface)
add_custom_target(runDatabase database_poc)

target_link_libraries(database_poc "zmq" client_interface)

