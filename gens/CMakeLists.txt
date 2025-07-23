include_directories(.)

add_library(gen STATIC gen.c)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/gen.h
    ${CMAKE_SOURCE_DIR}/include/gen.h
    COPY_ON_ERROR SYMBOLIC
)

add_subdirectory(eq_gen)
add_subdirectory(python_shellcode_gen)
