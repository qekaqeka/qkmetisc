add_library(eq_gen SHARED eq_gen.c eq.c)
target_link_libraries(eq_gen PRIVATE rcmem list)

find_package(PkgConfig REQUIRED)
pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)

target_link_libraries(eq_gen PUBLIC PkgConfig::gmp)
target_include_directories(eq_gen PUBLIC "${gmp_INCLUDE_DIRS}")

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/eq_gen.h
    ${CMAKE_SOURCE_DIR}/include/gens/eq_gen.h
    COPY_ON_ERROR SYMBOLIC
)

file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/eq_flags.h
    ${CMAKE_SOURCE_DIR}/include/gens/eq_flags.h
    COPY_ON_ERROR SYMBOLIC
)
