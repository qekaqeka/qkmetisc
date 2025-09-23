add_library(log STATIC log.c)

target_compile_options(log PRIVATE -pthread)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/log.h
    ${CMAKE_SOURCE_DIR}/include/log.h
    COPY_ON_ERROR SYMBOLIC
)
