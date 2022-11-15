include(max_warnings)

add_executable(
    specter
    "main.cpp"
)

target_max_warnings(TARGET specter)

set_target_properties(specter PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
