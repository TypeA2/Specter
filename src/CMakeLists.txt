include(max_warnings)

add_executable(
    specter
    "main.cpp"
)

target_max_warnings(TARGET specter)

find_package(cxxopts CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

target_link_libraries(specter PRIVATE cxxopts::cxxopts)
target_link_libraries(specter PRIVATE range-v3)

set_target_properties(specter PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
