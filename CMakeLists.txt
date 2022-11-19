cmake_minimum_required(VERSION 3.25)

find_program(VCPKG_EXECUTABLE vcpkg)
if(VCPKG_EXECUTABLE)
    cmake_path(GET VCPKG_EXECUTABLE PARENT_PATH VCPKG_ROOT)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

project(Specter VERSION 1.0.0)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

add_subdirectory(src)
