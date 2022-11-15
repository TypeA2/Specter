macro(target_max_warnings)
	cmake_parse_arguments(PARSED "" "TARGET" "VISIBILITY" ${ARGN})

	if (NOT PARSED_VISIBILITY)
		set(PARSED_VISIBILITY PRIVATE)
	endif()

	if (NOT PARSED_TARGET)
		message(FATAL_ERROR "Target not provided")
	endif()

	if (MSVC)
		target_compile_options(${PARSED_TARGET} ${PARSED_VISIBILITY} "/W4")
	else()
		target_compile_options(${PARSED_TARGET} ${PARSED_VISIBILITY} "-Wall" "-Wextra" "-Wpedantic")
	endif()
endmacro()
