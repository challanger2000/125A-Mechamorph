cmake_minimum_required(VERSION 3.20)
project(MechamorphCore LANGUAGES CXX)

add_library(mechamorph_core STATIC
    src/MechamorphCore.cpp
)

target_include_directories(mechamorph_core PUBLIC include)
target_compile_features(mechamorph_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(mechamorph_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(mechamorph_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(mechamorph_core_tests
    tests/core_tests.cpp
)
target_link_libraries(mechamorph_core_tests PRIVATE mechamorph_core)

enable_testing()
add_test(NAME mechamorph_core_tests COMMAND mechamorph_core_tests)

add_executable(mechamorph_smoke_render
    tools/render_smoke.cpp
)
target_link_libraries(mechamorph_smoke_render PRIVATE mechamorph_core)
