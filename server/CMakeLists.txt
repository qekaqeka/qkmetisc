add_library(server STATIC
    server.c 
    server_worker.c 
    server_worker_pool.c
)

target_link_libraries(server PRIVATE plot log listc utils gen pthread_assert bstream)
target_compile_options(server PRIVATE -pthread)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/server.h
    ${CMAKE_SOURCE_DIR}/include/server.h
    COPY_ON_ERROR SYMBOLIC
)
