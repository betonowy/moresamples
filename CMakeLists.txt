cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF)

option(BUILD_TESTING OFF)

project(moresamples)

add_subdirectory(thirdparty)

if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_library(moresamples_modules
    src/audio/audio.cpp
    src/Ctx.cpp
    src/nodes/impl/unity.cpp
    src/nodes/unity.cpp
    src/ui/unity.cpp
)

target_include_directories(moresamples_modules PUBLIC src)
target_link_libraries(moresamples_modules PRIVATE nuklear PUBLIC nlohmann_json)

add_executable(moresamples app/main.cpp)

target_link_libraries(moresamples moresamples_modules SDL3-static nuklear)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
