include_directories(.)

add_library(plot STATIC plot.c)
target_link_libraries(plot PRIVATE gen)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/plot.h
    ${CMAKE_SOURCE_DIR}/include/plot.h
    COPY_ON_ERROR SYMBOLIC
)

add_subdirectory(troll_eq_plot)
add_subdirectory(templates)
