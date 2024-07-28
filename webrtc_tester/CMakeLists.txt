cmake_minimum_required(VERSION 3.5)
project(webrtc_tester)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-unknown-pragmas -Wno-deprecated-declarations")

# Add compiler options
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(webrtc REQUIRED)
find_package(X11 REQUIRED)

# Include directories
include_directories(
  include
  ${webrtc_INCLUDE_DIRS}
)

add_definitions(${webrtc_DEFINITIONS})

# Add executables and link libraries
add_executable(webrtc_tester src/webrtc_tester.cpp)
target_link_libraries(webrtc_tester ${webrtc_LIBRARIES} ${X11_LIBRARIES})
ament_target_dependencies(webrtc_tester rclcpp std_msgs)

# Install targets
install(TARGETS
  webrtc_tester
  DESTINATION lib/${PROJECT_NAME})

# Ament package
ament_package()