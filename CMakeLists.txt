cmake_minimum_required(VERSION 4.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED YES)

project(trollcalc)

if ( PROJECT_IS_TOP_LEVEL )
    file(CREATE_LINK
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
        COPY_ON_ERROR SYMBOLIC
    )
endif()

include_directories(include)

add_subdirectory(gens)
add_subdirectory(log)
add_subdirectory(utils)
