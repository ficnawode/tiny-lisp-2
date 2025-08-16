set(NAME runtime)

file(GLOB_RECURSE SOURCES *.c *.h)

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

