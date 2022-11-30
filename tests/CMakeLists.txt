file(GLOB tests_contents LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/tests/*")

include(ProcessorCount)
ProcessorCount(cpu_count)

foreach(file ${tests_contents})
    # Arch name is directory name
    cmake_path(GET file STEM arch)

    # If there's a makefile add it as a target
    if(EXISTS "${file}/Makefile")
        add_custom_target(${arch}-tests ALL COMMAND make -j${cpu_count} -C ${file})
    endif()

    # Isolate plain non-bin directories
    if(NOT (${file} PATH_EQUAL "${PROJECT_SOURCE_DIR}/tests/bin") AND (IS_DIRECTORY ${file}))
        # Find all microtests config files
        file(GLOB microtests LIST_DIRECTORIES false "${file}/micro/*.toml")
        foreach(test_config ${microtests})
            # Binaries have the same stem
            cmake_path(GET test_config STEM test)
            cmake_path(REPLACE_EXTENSION test_config ${arch} OUTPUT_VARIABLE test_binary)

            set(test_name ${arch}-micro-${test})

            # Test target to compile the test program
            add_test(
                NAME ${test_name}-compile
                COMMAND make -C "${file}/micro" ${test_binary}
            )

            # Actually run it..
            add_test(
                NAME ${test_name}
                COMMAND $<TARGET_FILE:specter> "-c" ${test_config} ${test_binary}
            )

            # And to remove the binary
            # Don't do this for now, maybe make it optional?
            # add_test(
            #     NAME ${test_name}-cleanup
            #     COMMAND rm -f ${test_binary}
            # )

            set_tests_properties(${test_name}-compile PROPERTIES FIXTURES_SETUP    ${test_name})
            set_tests_properties(${test_name}         PROPERTIES FIXTURES_REQUIRED ${test_name})
            # set_tests_properties(${test_name}-cleanup PROPERTIES FIXTURES_CLEANUP  ${test_name})
        endforeach()

        foreach(test_config ${microtests})
            cmake_path(GET test_config STEM test)
            set(test_name ${arch}-micro-${test})

            # Retrieve dependencies from dedicated python script
            execute_process(
                OUTPUT_VARIABLE depends
                COMMAND_ERROR_IS_FATAL ANY
                COMMAND python3 "${PROJECT_SOURCE_DIR}/tests/depends.py" ${arch}-micro- ${test_config}
            )

            if(depends)
                # Require all this test's dependencies as setup
                set_tests_properties(${depends} PROPERTIES FIXTURES_SETUP ${test_name})
            endif()
        endforeach()
    endif()
endforeach()
