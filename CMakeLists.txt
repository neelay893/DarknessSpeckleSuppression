cmake_minimum_required(VERSION 2.8)
project(SpeckleDestruction)
find_package(OpenCV REQUIRED)

include_directories(include)
include_directories(/usr/local/boost_1_65_0)
file(GLOB SOURCES "src/*.cpp")

set (CMAKE_CXX_FLAGS "-pg -std=c++0x")

add_executable(SpeckleDestruction ${SOURCES})
target_link_libraries(SpeckleDestruction ${OpenCV_LIBS} -lrt -lpthread)

file(GLOB SIMSOURCES "sim/*.cpp")
add_executable(PacketMasterSim ${SIMSOURCES})
target_link_libraries(PacketMasterSim -lrt -lpthread)
