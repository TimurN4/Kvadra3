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

# Start client A
add_executable(clientA src/clientA.cpp)
target_link_libraries(clientA Boost::system)
target_compile_features(clientA PRIVATE cxx_std_17)

# Start Server
add_executable(server src/server.cpp)
target_link_libraries(server Boost::system)
target_compile_features(server PRIVATE cxx_std_17)

# Start client B
add_executable(clientB src/clientB.cpp)
target_link_libraries(clientB Boost::system)
target_compile_features(clientB PRIVATE cxx_std_17)

# add_executable(my_app main.cpp)
# target_link_libraries(my_app Boost::system)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)  # Подключает сгенерированный файл
# conan_basic_setup()  # Вызывает функцию, которая добавляет пути к include

# find_package(Protobuf CONFIG REQUIRED)
# find_package(gRPC CONFIG REQUIRED)

# add_executable(grpc_test
#     src/main.cpp
# )

# target_link_libraries(grpc_test
#     gRPC::grpc++
#     protobuf::libprotobuf
# )