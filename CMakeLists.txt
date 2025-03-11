cmake_minimum_required(VERSION 3.10)

project(TestServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(JSONCPP jsoncpp)

include(FetchContent)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.1
)
FetchContent_MakeAvailable(crow)

add_executable(TestServer server.cpp)

add_compile_definitions(PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

target_include_directories(TestServer PRIVATE
    ${crow_SOURCE_DIR}/include
    ${JSONCPP_INCLUDE_DIRS}
)

target_link_libraries(TestServer PRIVATE
    boost_system
    Threads::Threads
    CURL::libcurl
    OpenSSL::Crypto
    ${JSONCPP_LIBRARIES}
)
