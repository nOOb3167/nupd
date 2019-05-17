cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)
project(test_nupd)

enable_testing()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.66 REQUIRED COMPONENTS date_time thread filesystem regex unit_test_framework)

add_executable(nupd hasher.cpp hasher.hpp pscon.hpp psnupd.hpp test.cpp)
target_compile_definitions(nupd PRIVATE BOOST_FILESYSTEM_NO_DEPRECATED WIN32_LEAN_AND_MEAN #[[ boost chokes (winsock.h already included) unless lean is defined ]])
target_include_directories(nupd PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(nupd Boost::boost Boost::date_time Boost::filesystem Boost::regex Boost::unit_test_framework Bcrypt)
set_target_properties(nupd PROPERTIES CXX_STANDARD 17 RUNTIME_OUTPUT_DIRECTORY "$<0:>")

add_test(nupd nupd)
