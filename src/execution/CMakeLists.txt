add_library(
	specter_execution
    "elf_file.hpp" "elf_file.cpp"
	"executor.hpp" "executor.cpp"
    "rv64_executor.hpp" "rv64_executor.cpp"
)

target_max_warnings(TARGET specter_execution)

get_filename_component(PARENT_DIR "../" ABSOLUTE)
target_include_directories(specter_execution PUBLIC ${PARENT_DIR})

target_link_libraries(specter_execution PRIVATE specter_util specter_memory specter_arch)

set_target_properties(specter_execution PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)
