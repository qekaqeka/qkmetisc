add_library(bstream STATIC bstream.c)
target_link_libraries(bstream PRIVATE utils list)

#add_subdirectory(test)

#target_include_directories(bstream_test INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
#target_link_libraries(bstream_test INTERFACE bstream)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/bstream.h
    ${CMAKE_SOURCE_DIR}/include/bstream.h
    SYMBOLIC COPY_ON_ERROR
)
