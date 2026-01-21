add_library(cli STATIC cli.c cli_hash.c cli_convert.c)

target_link_libraries(cli PRIVATE listc)
target_compile_features(cli PRIVATE c_std_99)
target_compile_options(cli PRIVATE -Werror -Wextra -Wall -pedantic)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/cli.h
    ${CMAKE_SOURCE_DIR}/include/cli.h
    COPY_ON_ERROR SYMBOLIC
)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_convert.h
    ${CMAKE_SOURCE_DIR}/include/cli_convert.h
    COPY_ON_ERROR SYMBOLIC
)
