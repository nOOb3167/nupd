cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)
project(test_nupd)

enable_testing()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.66 REQUIRED COMPONENTS thread filesystem unit_test_framework)

add_executable(nupd nupd.cpp hasher.hpp orderedincs.hpp)
target_compile_definitions(nupd PRIVATE BOOST_FILESYSTEM_NO_DEPRECATED)
target_include_directories(nupd PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(nupd Boost::boost Boost::filesystem Boost::unit_test_framework Bcrypt)
set_target_properties(nupd PROPERTIES CXX_STANDARD 17 RUNTIME_OUTPUT_DIRECTORY "$<0:>")

add_test(nupd nupd)
