add_library(utils INTERFACE)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.h
    ${CMAKE_SOURCE_DIR}/include/utils.h
    COPY_ON_ERROR SYMBOLIC
)
