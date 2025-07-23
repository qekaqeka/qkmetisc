add_library(rcmem STATIC rcmem.c)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/rcmem.h
    ${CMAKE_SOURCE_DIR}/include/rcmem.h
    COPY_ON_ERROR SYMBOLIC
)
