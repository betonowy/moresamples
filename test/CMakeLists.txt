add_executable(moresamples_tests
    nodes.cpp
    filters.cpp
)

target_link_libraries(moresamples_tests moresamples_modules Catch2WithMain)

add_test(
    NAME moresamples_tests
    COMMAND moresamples_tests
)
