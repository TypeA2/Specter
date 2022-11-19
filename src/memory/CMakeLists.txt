add_library(
	specter_memory
	"vmem.hpp" "vmem.cpp"
)

target_max_warnings(TARGET specter_memory)

get_filename_component(PARENT_DIR "../" ABSOLUTE)
target_include_directories(specter_memory PUBLIC ${PARENT_DIR})

target_link_libraries(specter_memory PRIVATE specter_util)

set_target_properties(specter_memory PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)
