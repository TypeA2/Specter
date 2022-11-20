add_library(
	specter_util
	"mapped_file.hpp" "mapped_file.cpp"
    "formatting.hpp"  "aligned_memory.hpp"
)

target_max_warnings(TARGET specter_util)

get_filename_component(PARENT_DIR "../" ABSOLUTE)
target_include_directories(specter_util PUBLIC ${PARENT_DIR})

target_link_libraries(specter_util PUBLIC fmt::fmt)

set_target_properties(specter_util PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)
