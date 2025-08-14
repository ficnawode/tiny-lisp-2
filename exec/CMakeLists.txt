set(NAME exec)

file(GLOB_RECURSE SOURCES *.c *.h)
list(FILTER SOURCES EXCLUDE REGEX "main.c")


add_library(${NAME} STATIC)

target_link_libraries(${NAME} PUBLIC
    ${GLIB_LIBRARIES}
)

target_include_directories(${NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_sources(${NAME} PRIVATE
    ${SOURCES}
)

add_executable(exec_main main.c)

target_link_libraries(exec_main 
    ${NAME}
)
