cmake_minimum_required(VERSION 3.20)
project(MechamorphCore VERSION 0.1.0 LANGUAGES CXX)

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

add_executable(mechamorph_control_measurements
    tools/measure_controls.cpp
)
target_link_libraries(mechamorph_control_measurements PRIVATE mechamorph_core)

add_executable(mechamorph_signal_characterize
    tools/characterize_signals.cpp
)
target_link_libraries(mechamorph_signal_characterize PRIVATE mechamorph_core)

add_executable(mechamorph_component_lab
    tools/component_lab.cpp
)
target_link_libraries(mechamorph_component_lab PRIVATE mechamorph_core)

option(MECHAMORPH_BUILD_VST3 "Build the VST3 prototype wrapper" OFF)

if(MECHAMORPH_BUILD_VST3)
    include(FetchContent)
    set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(vst3sdk)
    smtg_enable_vst3_sdk()

    smtg_add_vst3plugin(Mechamorph
        source/version.h
        source/ids.h
        source/parameters.h
        source/state_format.h
        source/processor.h
        source/processor.cpp
        source/controller.h
        source/controller.cpp
        source/entry.cpp
        include/MechamorphCore.h
        src/MechamorphCore.cpp
    )

    target_include_directories(Mechamorph PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
    )
    target_link_libraries(Mechamorph PRIVATE sdk)
    target_compile_features(Mechamorph PRIVATE cxx_std_17)
    smtg_target_configure_version_file(Mechamorph)
endif()

add_library(machine_sampler_core STATIC
    src/MachineSamplerEngine.cpp
)
target_include_directories(machine_sampler_core PUBLIC include)
target_compile_features(machine_sampler_core PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(machine_sampler_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(machine_sampler_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(machine_sampler_tests
    tests/machine_sampler_tests.cpp
)
target_link_libraries(machine_sampler_tests PRIVATE machine_sampler_core)
add_test(NAME machine_sampler_tests COMMAND machine_sampler_tests)
