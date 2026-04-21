cmake_minimum_required(VERSION 3.22)
project(MultiColorComp VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MultiColorComp
    PLUGIN_MANUFACTURER_CODE  Mclr
    PLUGIN_CODE               Mcc1
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "MultiColor Comp"
    COMPANY_NAME              "Custom"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

target_sources(MultiColorComp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Compressors/LA4Compressor.cpp
    Source/Compressors/VSC3Compressor.cpp
    Source/Compressors/DistressorCompressor.cpp
    Source/Compressors/Fairchild670.cpp
    Source/Compressors/SSLOverdrive.cpp
    Source/Compressors/LA2ACompressor.cpp
    Source/Compressors/Spanky1176.cpp
)

juce_generate_juce_header(MultiColorComp)
target_include_directories(MultiColorComp PRIVATE Source)

target_compile_definitions(MultiColorComp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(MultiColorComp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
