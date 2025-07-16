add_library(eq_gen OBJECT eq_gen.c eq.c)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/eq_gen.h
    ${CMAKE_SOURCE_DIR}/include/gens/eq_gen.h
    COPY_ON_ERROR SYMBOLIC
)
