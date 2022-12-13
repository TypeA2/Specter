include(max_warnings)

add_executable(
    specter
    "main.cpp"
)

target_max_warnings(TARGET specter)

add_subdirectory(util)
add_subdirectory(memory)
add_subdirectory(arch)
add_subdirectory(execution)

target_link_libraries(
    specter PRIVATE
    specter_util
    specter_memory
    specter_arch
    specter_execution
)

set_target_properties(specter PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
