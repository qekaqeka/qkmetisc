add_library(listc INTERFACE listc.h)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/listc.h
    ${CMAKE_SOURCE_DIR}/include/listc.h
    COPY_ON_ERROR SYMBOLIC
)
