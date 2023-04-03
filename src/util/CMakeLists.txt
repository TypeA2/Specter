add_library(
	specter_util
    "elf_file.hpp" "elf_file.cpp"
	"mapped_file.hpp" "mapped_file.cpp"
    "formatting.hpp"  "aligned_memory.hpp"
)

target_max_warnings(TARGET specter_util)

get_filename_component(PARENT_DIR "../" ABSOLUTE)
target_include_directories(specter_util PUBLIC ${PARENT_DIR})

find_package(cxxopts CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

target_link_libraries(specter_util PRIVATE cxxopts::cxxopts)
target_link_libraries(specter_util PRIVATE magic_enum::magic_enum)
target_link_libraries(specter_util PUBLIC fmt::fmt)
target_link_libraries(specter_util PUBLIC range-v3 range-v3-meta range-v3::meta range-v3-concepts)

find_path(CPPTOML_INCLUDE_DIRS "cpptoml.h")
target_include_directories(specter_util PUBLIC ${CPPTOML_INCLUDE_DIRS})

set_target_properties(specter_util PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)
