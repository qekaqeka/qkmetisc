add_library(utils INTERFACE)
add_library(pthread_assert INTERFACE)
target_compile_options(pthread_assert INTERFACE -pthread)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.h
    ${CMAKE_SOURCE_DIR}/include/utils.h
    COPY_ON_ERROR SYMBOLIC
)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/pthread_assert.h
    ${CMAKE_SOURCE_DIR}/include/pthread_assert.h
    COPY_ON_ERROR SYMBOLIC
)
