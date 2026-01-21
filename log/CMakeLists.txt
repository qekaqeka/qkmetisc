add_library(log STATIC log.c)

target_compile_features(log PRIVATE c_std_17 c_static_assert )

include(CMakeDependentOption)

if ( UNIX )
    option(LOG_MT_SAFETY "MT safe logging" OFF)
    if ( LOG_MT_SAFETY )
        target_compile_definitions(log PRIVATE LOG_MT_SAFETY)
        target_compile_options(log PRIVATE -pthread)
    endif()
endif()

option(LOG_PRINT_ONCE "Merge parts of log line into one string and print" OFF)
cmake_dependent_option(LOG_PRINT_LINE_LEN "Len of log line in chars" 1024 LOG_PRINT_ONCE NONE)

if ( LOG_PRINT_ONCE ) 
    target_compile_definitions(log PRIVATE LOG_PRINT_ONCE LOG_PRINT_LINE_LEN=${LOG_PRINT_LINE_LEN})
endif()

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/log.h
    ${CMAKE_SOURCE_DIR}/include/log.h
    COPY_ON_ERROR SYMBOLIC
)
