add_library(
	specter_arch
    "arch.hpp" "arch.cpp"
	"rv64/rv64.hpp" "rv64/rv64.cpp"
    "rv64/decoder.hpp" "rv64/decoder.cpp"
    "rv64/formatter.hpp" "rv64/formatter.cpp"
    "rv64/regfile.hpp" "rv64/regfile.cpp"
    "rv64/alu.hpp" "rv64/alu.cpp"
)

target_max_warnings(TARGET specter_arch)

get_filename_component(PARENT_DIR "../" ABSOLUTE)
target_include_directories(specter_arch PUBLIC ${PARENT_DIR})

target_link_libraries(specter_arch PRIVATE specter_util specter_memory)

set_target_properties(specter_arch PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)
