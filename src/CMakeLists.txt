include(max_warnings)

add_executable(
    specter
    "main.cpp"
    "elf_file.hpp" "elf_file.cpp"
)

target_max_warnings(TARGET specter)

find_package(cxxopts CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

target_link_libraries(specter PRIVATE cxxopts::cxxopts)
target_link_libraries(specter PRIVATE magic_enum::magic_enum)
target_link_libraries(specter PRIVATE fmt::fmt)

find_path(CPPTOML_INCLUDE_DIRS "cpptoml.h")
target_include_directories(specter PUBLIC ${CPPTOML_INCLUDE_DIRS})

add_subdirectory(util)
add_subdirectory(memory)
add_subdirectory(execution)

target_link_libraries(
    specter PRIVATE
    specter_util
    specter_memory
    specter_execution
)

set_target_properties(specter PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
