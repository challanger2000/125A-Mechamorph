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
        GIT_SUBMODULES vstgui4
        GIT_SUBMODULES_RECURSE TRUE
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

add_executable(machine_real_render
    tools/render_machine_set.cpp
)
target_link_libraries(machine_real_render PRIVATE machine_sampler_core)
target_compile_features(machine_real_render PRIVATE cxx_std_17)

add_executable(measure_machine_controls
    tools/measure_machine_controls.cpp
)
target_link_libraries(measure_machine_controls PRIVATE machine_sampler_core)
target_compile_features(measure_machine_controls PRIVATE cxx_std_17)


option(MECHAMORPH_BUILD_MACHINE_VST3 "Build the standalone Machine sampler instrument" OFF)
set(MECHAMORPH_MACHINE_GENERATED_DIR "" CACHE PATH "Directory containing generated machine_assets_generated.h/.rc")

if(MECHAMORPH_BUILD_MACHINE_VST3)
    if(NOT WIN32)
        message(FATAL_ERROR "MechamorphMachine prototype currently embeds samples with Windows resources")
    endif()

    if(MECHAMORPH_MACHINE_GENERATED_DIR STREQUAL "")
        message(FATAL_ERROR "Set MECHAMORPH_MACHINE_GENERATED_DIR to generated machine resource directory")
    endif()

    if(NOT EXISTS "${MECHAMORPH_MACHINE_GENERATED_DIR}/machine_assets_generated.h")
        message(FATAL_ERROR "machine_assets_generated.h missing")
    endif()
    if(NOT EXISTS "${MECHAMORPH_MACHINE_GENERATED_DIR}/machine_assets_generated.rc")
        message(FATAL_ERROR "machine_assets_generated.rc missing")
    endif()

    if(NOT TARGET sdk)
        include(FetchContent)
        set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
        set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            vst3sdk_machine
            GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
            GIT_TAG 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
            GIT_SHALLOW FALSE
        )
        FetchContent_MakeAvailable(vst3sdk_machine)
        smtg_enable_vst3_sdk()
    endif()

    add_executable(measure_machine_space
        tools/measure_machine_space.cpp
        source-machine/MachineSpaceEngine.h
        source-machine/MachineSpaceEngine.cpp
    )
    target_include_directories(measure_machine_space PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source-machine"
        "${MECHAMORPH_MACHINE_GENERATED_DIR}"
        "${CMAKE_BINARY_DIR}"
    )
    target_compile_features(measure_machine_space PRIVATE cxx_std_17)

    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/resource/125A_Logo_Master_FINAL.svg" MECHAMORPH_BRANDING_SVG)
    string(REGEX MATCHALL "<path d=\\\"[^\\\"]+\\\" fill=\\\"#[0-9A-Fa-f]+\\\"/>" MECHAMORPH_BRANDING_PATH_TAGS "${MECHAMORPH_BRANDING_SVG}")
    list(LENGTH MECHAMORPH_BRANDING_PATH_TAGS MECHAMORPH_BRANDING_PATH_COUNT)
    if(NOT MECHAMORPH_BRANDING_PATH_COUNT EQUAL 30)
        message(FATAL_ERROR "125A branding master must contain exactly 30 vector paths; found ${MECHAMORPH_BRANDING_PATH_COUNT}.")
    endif()
    set(MECHAMORPH_BRANDING_PATHS "")
    foreach(MECHAMORPH_BRANDING_TAG IN LISTS MECHAMORPH_BRANDING_PATH_TAGS)
        if(NOT MECHAMORPH_BRANDING_TAG MATCHES "^<path d=\\\"([^\\\"]+)\\\" fill=\\\"([^\\\"]+)\\\"/>$")
            message(FATAL_ERROR "Could not parse 125A branding path tag.")
        endif()
        set(MECHAMORPH_BRANDING_D "${CMAKE_MATCH_1}")
        set(MECHAMORPH_BRANDING_FILL "${CMAKE_MATCH_2}")
        if(MECHAMORPH_BRANDING_FILL STREQUAL "#D71920")
            set(MECHAMORPH_BRANDING_RED "true")
        elseif(MECHAMORPH_BRANDING_FILL STREQUAL "#D9D9D9")
            set(MECHAMORPH_BRANDING_RED "false")
        else()
            message(FATAL_ERROR "Unexpected 125A branding fill: ${MECHAMORPH_BRANDING_FILL}")
        endif()
        string(APPEND MECHAMORPH_BRANDING_PATHS "    {\"${MECHAMORPH_BRANDING_D}\", ${MECHAMORPH_BRANDING_RED}},\n")
    endforeach()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/source-machine/branding_master.h.in"
        "${CMAKE_BINARY_DIR}/branding_master.h"
        @ONLY
    )

    smtg_add_vst3plugin(MechamorphMachine
        source/version.h
        source-machine/ids.h
        source-machine/parameters.h
        source-machine/MachineAssetBank.h
        source-machine/MachineAssetBank.cpp
        source-machine/processor.h
        source-machine/processor.cpp
        source-machine/controller.h
        source-machine/controller.cpp
        source-machine/editor.cpp
        source-machine/MechamorphGui.h
        source-machine/MechamorphGui.cpp
        source-machine/MachineSpaceEngine.h
        source-machine/MachineSpaceEngine.cpp
        source-machine/entry.cpp
        include/MachineSamplerEngine.h
        src/MachineSamplerEngine.cpp
        "${MECHAMORPH_MACHINE_GENERATED_DIR}/machine_assets_generated.rc"
    )

    target_include_directories(MechamorphMachine PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_CURRENT_SOURCE_DIR}/source-machine"
        "${MECHAMORPH_MACHINE_GENERATED_DIR}"
    )

    target_link_libraries(MechamorphMachine PRIVATE sdk vstgui_support)

    add_executable(mechamorph_editor_lifecycle_probe
        tests/editor_lifecycle_zoom_probe.cpp
        "${vst3sdk_machine_SOURCE_DIR}/public.sdk/source/vst/hosting/module_win32.cpp"
    )
    target_link_libraries(mechamorph_editor_lifecycle_probe PRIVATE
        sdk_hosting sdk_common base pluginterfaces user32 ole32
    )
    target_compile_definitions(mechamorph_editor_lifecycle_probe PRIVATE
        _CRT_SECURE_NO_WARNINGS NOMINMAX UNICODE _UNICODE
    )
    target_compile_features(mechamorph_editor_lifecycle_probe PRIVATE cxx_std_17)

    add_executable(mechamorph_state_recall_probe
        tests/machine_state_recall_probe.cpp
        "${vst3sdk_machine_SOURCE_DIR}/public.sdk/source/common/memorystream.cpp"
        "${vst3sdk_machine_SOURCE_DIR}/public.sdk/source/vst/hosting/module_win32.cpp"
    )
    target_link_libraries(mechamorph_state_recall_probe PRIVATE
        sdk_hosting sdk_common base pluginterfaces
    )
    target_compile_definitions(mechamorph_state_recall_probe PRIVATE
        _CRT_SECURE_NO_WARNINGS NOMINMAX
    )
    target_compile_features(mechamorph_state_recall_probe PRIVATE cxx_std_17)

    add_executable(mechamorph_process_contract_probe
        tests/machine_process_contract_probe.cpp
        "${vst3sdk_machine_SOURCE_DIR}/public.sdk/source/vst/hosting/eventlist.cpp"
        "${vst3sdk_machine_SOURCE_DIR}/public.sdk/source/vst/hosting/parameterchanges.cpp"
        "${vst3sdk_machine_SOURCE_DIR}/public.sdk/source/vst/hosting/module_win32.cpp"
    )
    target_link_libraries(mechamorph_process_contract_probe PRIVATE
        sdk_hosting sdk_common base pluginterfaces
    )
    target_compile_definitions(mechamorph_process_contract_probe PRIVATE
        _CRT_SECURE_NO_WARNINGS NOMINMAX
    )
    target_compile_features(mechamorph_process_contract_probe PRIVATE cxx_std_17)
    smtg_target_add_plugin_resources(MechamorphMachine RESOURCES
        "resource/mechamorph.uidesc"
        "resource/mechamorph-faceplate-runtime.png"
    )
    smtg_target_add_plugin_resources(MechamorphMachine
        OUTPUT_SUBDIRECTORY "mechamorph-controls"
        RESOURCES
            "resource/mechamorph-controls/knob-machine-64.png"
            "resource/mechamorph-controls/knob-main-64.png"
            "resource/mechamorph-controls/knob-scale-64.png"
            "resource/mechamorph-controls/knob-utility-64.png"
            "resource/mechamorph-controls/mechamorph-dangerous-sign.png"
    )
    target_compile_features(MechamorphMachine PRIVATE cxx_std_17)
    smtg_target_configure_version_file(MechamorphMachine)
endif()
