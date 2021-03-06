# Networking protocol

# Project
cmake_minimum_required(VERSION 3.10)
project(network_protocol)
set(CMAKE_CXX_STANDARD 14)

set(LIB_NAME cv_networking)

# option to compile test client and server programs
option(COMPILE_TEST_PROGRAMS "Compile the test client and server programs" ON)

# Boost
find_package(Boost 1.66 REQUIRED COMPONENTS system thread)
include_directories(${Boost_INCLUDE_DIR})

# OpenCV if test programs are set to ON
if (${COMPILE_TEST_PROGRAMS})
    message("Compiling test programs with OpenCV 4.1")
    find_package(OpenCV 4.1 REQUIRED)
endif()

# Core sources
list(APPEND CORE_SOURCES
        src/core/StereoStream.cpp
        src/core/StereoStreamerClient.cpp
        src/core/ReconstructionServer.cpp
)

# Protocol sources
list(APPEND PROTOCOL_SOURCES
        src/protocol/ProtocolStream.cpp
)

# The networking library
add_library(${LIB_NAME} STATIC ${CORE_SOURCES} ${PROTOCOL_SOURCES})
add_library(reconstruction::networking ALIAS ${LIB_NAME})
target_link_libraries(${LIB_NAME} ${Boost_LIBRARIES})

# set include directories
target_include_directories(${LIB_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Test programs
if (${COMPILE_TEST_PROGRAMS})
    # 3D reconstruction server
    add_executable(test_reconstruction_server src/test/test_reconstruction_server.cpp)
    target_link_libraries(test_reconstruction_server ${LIB_NAME} ${OpenCV_LIBS})

    # stereo streaming client
    add_executable(test_stereo_client src/test/test_stereo_client.cpp)
    target_link_libraries(test_stereo_client ${LIB_NAME} ${OpenCV_LIBS})
endif ()
