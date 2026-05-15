cmake_minimum_required(VERSION 3.16)

project(Kvadra3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

find_package(Boost REQUIRED)

set(KVADRA_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(clientA src/clientA.cpp src/client_settings.cpp)
target_link_libraries(clientA PRIVATE ${Boost_LIBRARIES})
target_compile_features(clientA PRIVATE cxx_std_17)
target_include_directories(clientA PRIVATE ${KVADRA_INCLUDE} ${Boost_INCLUDE_DIRS})

add_executable(server src/server.cpp)
target_link_libraries(server PRIVATE ${Boost_LIBRARIES})
target_compile_features(server PRIVATE cxx_std_17)
target_include_directories(server PRIVATE ${KVADRA_INCLUDE} ${Boost_INCLUDE_DIRS})

add_executable(clientB src/clientB.cpp src/client_settings.cpp)
target_link_libraries(clientB PRIVATE ${Boost_LIBRARIES})
target_compile_features(clientB PRIVATE cxx_std_17)
target_include_directories(clientB PRIVATE ${KVADRA_INCLUDE} ${Boost_INCLUDE_DIRS})

enable_testing()
add_executable(test_protocol tests/test_protocol.cpp)
target_include_directories(test_protocol PRIVATE ${KVADRA_INCLUDE})
add_test(NAME protocol COMMAND test_protocol)
