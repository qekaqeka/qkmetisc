add_library(troll_eq_plot STATIC troll_eq_plot.c)
target_link_libraries(troll_eq_plot PRIVATE gen eq_gen python_shellcode_gen)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/troll_eq_plot.h
    ${CMAKE_SOURCE_DIR}/include/plots/troll_eq_plot.h
    COPY_ON_ERROR SYMBOLIC
)
