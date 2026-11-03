cmake_minimum_required(VERSION 3.22)
project(TB3030 VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(TB3030 COMPANY_NAME "Landgren Audio" BUNDLE_ID com.landgrenaudio.tb3030
 PLUGIN_MANUFACTURER_CODE Lgrn PLUGIN_CODE T303 FORMATS Standalone VST3
 PRODUCT_NAME "TB-3030" NEEDS_MIDI_INPUT TRUE IS_SYNTH TRUE COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(TB3030 PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/AcidEngine.cpp Source/Sequencer.cpp Source/PresetBank.cpp)
target_compile_definitions(TB3030 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(TB3030 PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp juce::juce_gui_extra PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
juce_generate_juce_header(TB3030)
