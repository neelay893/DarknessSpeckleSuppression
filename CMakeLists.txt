cmake_minimum_required(VERSION 2.8)
project(SpeckleDestruction)
find_package(OpenCV 3.3 REQUIRED)
find_package(Boost REQUIRED)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

set (CMAKE_CXX_FLAGS "-pg -std=c++11")
set (CMAKE_CXX_COMPILER "/opt/rh/devtoolset-4/root/usr/bin/g++")

add_executable(SpeckleDestruction ${SOURCES})
target_link_libraries(SpeckleDestruction ${OpenCV_LIBS} -lrt -lpthread -lssh)

file(GLOB SIMSOURCES "sim/*.cpp")
add_executable(PacketMasterSim ${SIMSOURCES})
target_link_libraries(PacketMasterSim -lrt -lpthread)
