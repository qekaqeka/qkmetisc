add_library(bstream_test INTERFACE)

add_executable(bstream_only_borrowed bstream_only_borrow.c)
target_link_libraries(bstream_only_borrowed PRIVATE bstream_test)

add_executable(bstream_only_copied bstream_only_copied.c)
target_link_libraries(bstream_only_copied PRIVATE bstream_test)

include(CTest)

add_test(
    NAME bstream_only_borrowed
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/bstream_only_borrowed"
)

add_test(
    NAME bstream_only_copied
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/bstream_only_copied"
)
