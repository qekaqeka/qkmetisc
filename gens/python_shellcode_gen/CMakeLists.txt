add_library(python_shellcode_gen STATIC python_shellcode_gen.c)
target_link_libraries(python_shellcode_gen PRIVATE utils rcmem)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/python_shellcode_gen.h
    ${CMAKE_SOURCE_DIR}/include/gens/python_shellcode.h
    COPY_ON_ERROR SYMBOLIC
)
