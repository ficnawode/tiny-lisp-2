find_program(NASM_EXECUTABLE nasm)
if(NOT NASM_EXECUTABLE)
    message(FATAL_ERROR "nasm executable not found. Please install NASM.")
endif()

# In test/lisp/CMakeLists.txt

function(add_e2e_test LISP_SOURCE_FILE)
    # --- 1. Define File Paths ---
    get_filename_component(TEST_NAME ${LISP_SOURCE_FILE} NAME_WE)
    set(TEST_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}")
    file(MAKE_DIRECTORY ${TEST_BUILD_DIR})

    set(LISP_FILE_IN_BUILD "${TEST_BUILD_DIR}/${TEST_NAME}.lisp") 
    set(ASM_FILE "${TEST_BUILD_DIR}/${TEST_NAME}.asm")
    set(OBJECT_FILE "${TEST_BUILD_DIR}/${TEST_NAME}.o")
    set(EXECUTABLE_FILE "${TEST_BUILD_DIR}/${TEST_NAME}_exe")
    set(ACTUAL_OUTPUT_FILE "${TEST_BUILD_DIR}/${TEST_NAME}.actual.txt")

    set(EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.expected.txt")
    if(NOT EXISTS ${EXPECTED_OUTPUT_FILE})
        message(FATAL_ERROR "Missing expected output file for test '${TEST_NAME}'.\nRequired: ${EXPECTED_OUTPUT_FILE}")
    endif()


    add_custom_command(
        OUTPUT  ${ASM_FILE}
        COMMAND $<TARGET_FILE:exec_main> ${CMAKE_CURRENT_SOURCE_DIR}/${LISP_SOURCE_FILE}
        DEPENDS $<TARGET_FILE:exec_main> ${LISP_SOURCE_FILE}
        WORKING_DIRECTORY ${TEST_BUILD_DIR}
        COMMENT "Compiling ${LISP_SOURCE_FILE} -> ${TEST_NAME}.asm"
        VERBATIM
    )

    add_custom_command(
        OUTPUT  ${OBJECT_FILE}
        COMMAND ${NASM_EXECUTABLE} -f elf64 -g ${ASM_FILE} -o ${OBJECT_FILE}
        DEPENDS ${ASM_FILE}
        COMMENT "Assembling ${TEST_NAME}.asm -> ${TEST_NAME}.o"
        VERBATIM
    )

    add_custom_command(
        OUTPUT  ${EXECUTABLE_FILE}
        COMMAND ${CMAKE_C_COMPILER} ${OBJECT_FILE} $<TARGET_FILE:runtime> -o ${EXECUTABLE_FILE}
        DEPENDS ${OBJECT_FILE} runtime
        COMMENT "Linking ${TEST_NAME}.o -> ${TEST_NAME}_exe"
        VERBATIM
    )

    add_custom_target(
        ${TEST_NAME}_build_executable ALL
        DEPENDS ${EXECUTABLE_FILE}
    )

    # --- 4. Define the Test Step ---
    # This test will run the executable and compare its output to the expected output.
    add_test(
        NAME ${TEST_NAME}
        # --- FIX IS HERE ---
        # We call 'sh -c' directly, not via 'cmake -E'.
        COMMAND sh -c
                # The command string is now an argument to 'sh -c'.
                # Note the quotes to handle paths with spaces.
                "\"${EXECUTABLE_FILE}\" > \"${ACTUAL_OUTPUT_FILE}\" && ${CMAKE_COMMAND} -E compare_files --ignore-eol \"${ACTUAL_OUTPUT_FILE}\" \"${EXPECTED_OUTPUT_FILE}\""
    )

    # Tell CTest that this test cannot run until its executable is fully built.
    set_tests_properties(${TEST_NAME} PROPERTIES
        DEPENDS ${TEST_NAME}_build_executable
    )

endfunction()


file(GLOB LISP_TEST_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test_*.lisp")

message(STATUS "Discovered Lisp test files: ${LISP_TEST_FILES}")

foreach(lisp_file ${LISP_TEST_FILES})
    add_e2e_test(${lisp_file})
endforeach()