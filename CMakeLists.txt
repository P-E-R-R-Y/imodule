# ----------- general -----------

cmake_minimum_required(VERSION 3.10)
project(imodule)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------- project -----------

# One pure interface, header-only : nothing to compile. Loading and managing
# those modules lives in modulemanager - but the protocol IModule carries,
# the use count and the condemned flag, is testable on its own and is where
# the ordering bugs live.
add_library(${PROJECT_NAME} INTERFACE)

# ----------- include -----------

target_include_directories(${PROJECT_NAME} INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/includes/
)

# ----------- test -----------

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(GoogleTest REQUIRED)

enable_testing()

file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})

target_include_directories(${PROJECT_NAME}_tests PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/tests/includes
)

target_link_libraries(${PROJECT_NAME}_tests
	gtest_main
	${PROJECT_NAME}
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)
