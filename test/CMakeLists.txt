enable_testing()

macro(add_glib_test TEST_SRC)
    get_filename_component(TEST_NAME "${TEST_SRC}" NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_include_directories(${TEST_NAME} PRIVATE ${GLIB2_INCLUDE_DIRS})
    target_link_libraries(${TEST_NAME} PRIVATE ${GLIB2_LIBRARIES} exec)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.c")


foreach(TEST_SRC IN LISTS TEST_SOURCES)
    add_glib_test(${TEST_SRC})
endforeach()
