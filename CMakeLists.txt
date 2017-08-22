cmake_minimum_required(VERSION 2.8)
project(SpeckleDestruction)
find_package(OpenCV REQUIRED)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(SpeckleDestruction ${SOURCES})
target_link_libraries(SpeckleDestruction ${OpenCV_LIBS})
