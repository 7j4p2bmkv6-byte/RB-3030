cmake_minimum_required(VERSION 3.22)
project(TB3030 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# static MSVC runtime - no separate Visual C++ redistributable needed
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE: uses ./JUCE if present (the GitHub workflow clones it), otherwise downloads it
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(TB3030
    COMPANY_NAME "Landgren Audio"
    COMPANY_WEBSITE "https://landgren.audio"
    BUNDLE_ID com.landgrenaudio.tb3030
    PLUGIN_MANUFACTURER_CODE Lgrn
    PLUGIN_CODE Tb33
    FORMATS VST3 Standalone
    PRODUCT_NAME "TB-3030"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.png")

target_sources(TB3030 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SoundBank.cpp)

target_include_directories(TB3030 PRIVATE Source)

target_compile_definitions(TB3030 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

# ASIO for the stand-alone app (only if the workflow managed to fetch the Steinberg ASIO SDK)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/asiosdk/common/iasiodrv.h")
    message(STATUS "TB-3030: ASIO SDK found - ASIO enabled")
    target_compile_definitions(TB3030 PUBLIC JUCE_ASIO=1)
    target_include_directories(TB3030 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/asiosdk/common")
else()
    message(STATUS "TB-3030: no ASIO SDK - building with WASAPI/DirectSound only")
endif()

target_link_libraries(TB3030
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
