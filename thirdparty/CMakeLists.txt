add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(json EXCLUDE_FROM_ALL)

if(BUILD_TESTING)
    add_subdirectory(Catch2 EXCLUDE_FROM_ALL)
endif()

add_library(nuklear nuklear_impl/nuklear_impl.c)

target_include_directories(nuklear PUBLIC Nuklear nuklear_impl)

target_link_libraries(nuklear PUBLIC SDL3-static)

target_compile_definitions(nuklear PUBLIC
    -DNK_INCLUDE_FIXED_TYPES
    -DNK_INCLUDE_STANDARD_IO
    -DNK_INCLUDE_STANDARD_VARARGS
    -DNK_INCLUDE_STANDARD_BOOL
    -DNK_INCLUDE_DEFAULT_ALLOCATOR
    -DNK_INCLUDE_VERTEX_BUFFER_OUTPUT
    -DNK_INCLUDE_FONT_BAKING
    -DNK_INCLUDE_DEFAULT_FONT
)
