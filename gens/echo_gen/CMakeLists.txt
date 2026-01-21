add_library(echo_gen SHARED echo_gen.c)

target_link_libraries(echo_gen PRIVATE rcmem)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/echo_gen.h
    ${CMAKE_SOURCE_DIR}/include/gens/echo_gen.h
    COPY_ON_ERROR SYMBOLIC
)
