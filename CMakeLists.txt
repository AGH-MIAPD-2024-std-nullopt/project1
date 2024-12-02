cmake_minimum_required(VERSION 3.12)
project(project1)

# compiler options
set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Wextra)

# output dirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)

# Restinio, libfmt
set(RESTINIO_EXPLICIT_CPPSTD "23")
set(RESTINIO_ASIO_SOURCE "standalone")
set(RESTINIO_ASIO_PATH_HINT "${CMAKE_SOURCE_DIR}/includes/asio/include")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/includes/asio-cmake")
find_package(asio REQUIRED)
add_subdirectory(includes/nodejs/llhttp)
add_subdirectory(includes/fmt)
add_subdirectory(includes/expected-lite)
add_subdirectory(includes/restinio)

# Eigen
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/includes/eigen-3.4.0/cmake")
set(EIGEN3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/includes/eigen-3.4.0")
find_package(Eigen3 REQUIRED)

# simplejson
add_subdirectory(includes/simpleson-1.1.0)

# webserver executable
add_executable(webserver "src/main.cpp" "src/json_handling.cpp" "src/AHP.cpp" "src/webserver.cpp")
target_include_directories(webserver PRIVATE ${CMAKE_SOURCE_DIR}/includes)
target_link_libraries(webserver PRIVATE restinio::restinio fmt::fmt Eigen3::Eigen simpleson)