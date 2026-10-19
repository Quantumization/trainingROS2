cmake_minimum_required(VERSION 3.8)
project(training_diagnostics)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

add_executable(diagnostics_node src/diagnostics_node.cpp)
target_compile_features(diagnostics_node PUBLIC cxx_std_17)
ament_target_dependencies(diagnostics_node rclcpp std_msgs std_srvs)

install(TARGETS diagnostics_node
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
