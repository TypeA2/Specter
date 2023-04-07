add_library(
	specter_recompilation
    "ir.hpp" "ir.cpp"
)

target_max_warnings(TARGET specter_recompilation)

get_filename_component(PARENT_DIR "../" ABSOLUTE)
target_include_directories(specter_recompilation PUBLIC ${PARENT_DIR})

target_link_libraries(
    specter_recompilation PRIVATE
    specter_util
)

set_target_properties(specter_recompilation PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)
