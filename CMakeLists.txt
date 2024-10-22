cmake_minimum_required(VERSION 3.12)
project(project1)

# compiler options
set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Wextra)

# output dirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)

# defines for restinio and its dependencies
set(RESTINIO_EXPLICIT_CPPSTD "23")
set(RESTINIO_ASIO_SOURCE "standalone")
set(RESTINIO_ASIO_PATH_HINT "${CMAKE_BINARY_DIR}/includes/asio/include")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/includes/asio-cmake")

# includes for restinio and its dependencies
find_package(asio REQUIRED)
add_subdirectory(includes/nodejs/llhttp)
add_subdirectory(includes/fmt)
add_subdirectory(includes/expected-lite)
add_subdirectory(includes/restinio-0.7.0)

add_executable(webserver "src/main.cpp")
target_link_libraries(webserver PRIVATE restinio::restinio)