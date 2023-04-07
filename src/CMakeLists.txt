include(max_warnings)

#add_executable(
#    specter_emu
#    "specter_emu.cpp"
#)
add_executable(
    specter_rec
    "specter_rec.cpp"
)

#target_max_warnings(TARGET specter_emu)
target_max_warnings(TARGET specter_rec)

add_subdirectory(util)
add_subdirectory(memory)
add_subdirectory(recompilation)
add_subdirectory(arch)
#add_subdirectory(execution)

#target_link_libraries(
#    specter_emu PRIVATE
#    specter_util
#    specter_memory
#    specter_arch
#    specter_execution
#)
target_link_libraries(
    specter_rec PRIVATE
    specter_util
    specter_memory
    specter_arch
    specter_recompilation
)

#set_target_properties(specter_emu PROPERTIES
#    CXX_STANDARD 23
#    CXX_STANDARD_REQUIRED ON
#    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
#    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
#)

set_target_properties(specter_rec PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
