cmake_minimum_required(VERSION 3.22)

project(ARC VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

option(ARC_BUILD_TESTS  "Build the ARC test suite"               ON)
option(ARC_ENABLE_ASAN  "Build with AddressSanitizer"            OFF)
option(ARC_ENABLE_UBSAN "Build with UndefinedBehaviorSanitizer"  OFF)
option(ARC_ENABLE_TSAN  "Build with ThreadSanitizer"             OFF)
option(ARC_ENABLE_LTO   "Link-time optimisation for the plugin"  ON)
set(ARC_JUCE_PATH "" CACHE PATH "Optional path to a local JUCE checkout")
set(ARC_COMPANY_NAME "ARC Instruments" CACHE STRING "Company name reported to hosts")

# ---------------------------------------------------------------------------
# Sanitizers (applied globally so JUCE, engine and tests agree)
# ---------------------------------------------------------------------------
set(ARC_SANITIZER_FLAGS "")
if(ARC_ENABLE_ASAN)
    list(APPEND ARC_SANITIZER_FLAGS -fsanitize=address -fno-omit-frame-pointer)
endif()
if(ARC_ENABLE_UBSAN)
    list(APPEND ARC_SANITIZER_FLAGS -fsanitize=undefined -fno-sanitize-recover=undefined)
endif()
if(ARC_ENABLE_TSAN)
    list(APPEND ARC_SANITIZER_FLAGS -fsanitize=thread)
endif()
if(ARC_SANITIZER_FLAGS)
    add_compile_options(${ARC_SANITIZER_FLAGS})
    add_link_options(${ARC_SANITIZER_FLAGS})
endif()

# ---------------------------------------------------------------------------
# JUCE: local path > external/JUCE > FetchContent (pinned)
# ---------------------------------------------------------------------------
if(ARC_JUCE_PATH)
    add_subdirectory(${ARC_JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt)
    add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.12
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# arc_engine: the complete synthesis engine. Pure C++20, no JUCE dependency.
# Everything that runs on the audio thread lives here and is unit-testable.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ARC_ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Synthesis/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Exciters/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Materials/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Motion/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Nonlinear/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/FX/*.cpp)

add_library(arc_engine STATIC ${ARC_ENGINE_SOURCES})
target_include_directories(arc_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_features(arc_engine PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(arc_engine PRIVATE /W4 /fp:precise)
else()
    target_compile_options(arc_engine PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        -Wno-sign-conversion -Wdouble-promotion -fno-math-errno)
endif()

# ---------------------------------------------------------------------------
# JUCE-facing code shared by the plugin and the tests (processor, state,
# presets, editor). INTERFACE so each consumer compiles its own copy.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ARC_PLUGIN_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Graphics/*.cpp)
list(APPEND ARC_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp)

file(GLOB ARC_FONT_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/*.ttf)
if(ARC_FONT_FILES)
    juce_add_binary_data(arc_binary_data NAMESPACE ArcBinary SOURCES ${ARC_FONT_FILES})
    set(ARC_HAS_BINARY_DATA 1)
else()
    set(ARC_HAS_BINARY_DATA 0)
endif()

add_library(arc_plugin_code INTERFACE)
target_sources(arc_plugin_code INTERFACE ${ARC_PLUGIN_SOURCES})
target_include_directories(arc_plugin_code INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_definitions(arc_plugin_code INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_SILENCE_XCODE_15_LINKER_WARNING=1
    ARC_VERSION_STRING="${PROJECT_VERSION}"
    ARC_HAS_BINARY_DATA=${ARC_HAS_BINARY_DATA})
target_link_libraries(arc_plugin_code INTERFACE
    arc_engine
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_dsp)
if(ARC_HAS_BINARY_DATA)
    target_link_libraries(arc_plugin_code INTERFACE arc_binary_data)
endif()

# ---------------------------------------------------------------------------
# The plugin
# ---------------------------------------------------------------------------
set(ARC_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND ARC_FORMATS AU)
endif()

juce_add_plugin(ARC
    PRODUCT_NAME                "ARC"
    COMPANY_NAME                "${ARC_COMPANY_NAME}"
    BUNDLE_ID                   com.arcinstruments.arc
    VERSION                     ${PROJECT_VERSION}
    PLUGIN_MANUFACTURER_CODE    ArcI
    PLUGIN_CODE                 Arc1
    FORMATS                     ${ARC_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    DESCRIPTION                 "Resonant Network Synthesizer"
    COPY_PLUGIN_AFTER_BUILD     FALSE
    MICROPHONE_PERMISSION_ENABLED FALSE)

target_link_libraries(ARC PRIVATE
    arc_plugin_code
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
if(ARC_ENABLE_LTO)
    target_link_libraries(ARC PRIVATE juce::juce_recommended_lto_flags)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(ARC_BUILD_TESTS)
    file(GLOB ARC_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.cpp)
    juce_add_console_app(ARCTests PRODUCT_NAME "ARCTests")
    target_sources(ARCTests PRIVATE ${ARC_TEST_SOURCES})
    target_include_directories(ARCTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Tests)
    target_compile_definitions(ARCTests PRIVATE ARC_TESTS=1
        ARC_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(ARCTests PRIVATE
        arc_plugin_code
        juce::juce_recommended_config_flags)

    enable_testing()
    add_test(NAME arc_core_tests COMMAND ARCTests --all)
endif()
