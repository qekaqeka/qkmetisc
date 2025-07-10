include_directories(.)

add_library(gen_base OBJECT gen_base.c)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_base.h
    ${CMAKE_SOURCE_DIR}/include/gen_base.h
    COPY_ON_ERROR SYMBOLIC
)

add_subdirectory(eq_gen)
