# https://stackoverflow.com/questions/16596876/object-file-has-too-many-sections/28372660#28372660
#   /bigobj -Wa,-mbig-obj

cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)
project(nupd)

enable_testing()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.66 REQUIRED COMPONENTS date_time thread filesystem regex unit_test_framework)

add_library(nupd STATIC ext/picosha2.h hasher.cpp hasher.hpp pscon.hpp psnupd.hpp)
target_include_directories(nupd PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_definitions(nupd PUBLIC
	_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
	BOOST_FILESYSTEM_NO_DEPRECATED
	WIN32_LEAN_AND_MEAN #[[ boost chokes (winsock.h already included) unless lean is defined ]]
	$<$<BOOL:${MSVC}>:PS_USE_BCRYPT_WIN _WIN32_WINNT=0x0601 >)
target_compile_options(nupd PUBLIC $<$<BOOL:${MSVC}>:/bigobj> $<$<BOOL:${MINGW}>:-Wa,-mbig-obj>)
target_link_libraries(nupd
	PUBLIC Boost::boost Boost::date_time Boost::filesystem Boost::regex Boost::thread
	PUBLIC $<$<BOOL:${MSVC}>:Bcrypt> $<$<BOOL:${MINGW}>:bcrypt ws2_32>)
set_target_properties(nupd PROPERTIES CXX_STANDARD 17 RUNTIME_OUTPUT_DIRECTORY "$<0:>")

add_executable(test0 test.cpp)
target_link_libraries(test0 nupd Boost::unit_test_framework)
set_target_properties(test0 PROPERTIES CXX_STANDARD 17 RUNTIME_OUTPUT_DIRECTORY "$<0:>")

add_test(test test0)
