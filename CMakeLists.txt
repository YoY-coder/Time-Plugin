# =============================================================================
#  Time — plugin VST3 de chronométrage du temps de travail réel
#  JUCE 8 · CMake · VST3 uniquement · Windows (Visual Studio 2022) & macOS (Xcode)
# =============================================================================
cmake_minimum_required(VERSION 3.22)

# Réglages macOS (doivent être définis AVANT project()) :
#  - binaire universel : fonctionne sur Mac Intel et Mac Apple Silicon (M1/M2/M3/M4)
#  - compatible macOS 10.13 et plus récent
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Architectures macOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Version minimale de macOS")
endif()

project(Time VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
#  JUCE : téléchargé automatiquement au premier "cmake" (il faut Git + Internet).
#  Si vous avez déjà JUCE sur votre disque, vous pouvez à la place passer
#  -DTIME_JUCE_DIR=/chemin/vers/JUCE
# -----------------------------------------------------------------------------
set(TIME_JUCE_DIR "" CACHE PATH "Dossier JUCE local (optionnel)")

if(TIME_JUCE_DIR)
    add_subdirectory("${TIME_JUCE_DIR}" JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.17-security
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# -----------------------------------------------------------------------------
#  Le plugin
# -----------------------------------------------------------------------------
juce_add_plugin(Time
    PRODUCT_NAME                "Time"
    COMPANY_NAME                "YoY"
    BUNDLE_ID                   "com.yoy.time"
    VERSION                     "${PROJECT_VERSION}"
    PLUGIN_MANUFACTURER_CODE    Yoyc
    PLUGIN_CODE                 Tme1
    FORMATS                     VST3
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES             Fx Tools
    COPY_PLUGIN_AFTER_BUILD     FALSE)

target_sources(Time PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/TimeLookAndFeel.cpp
    Source/LiveClock.cpp)

# Détection de l'activité : un fichier par système
if(WIN32)
    target_sources(Time PRIVATE Source/ActivityProbe_win.cpp)
elseif(APPLE)
    target_sources(Time PRIVATE Source/ActivityProbe_mac.mm)
    target_link_libraries(Time PRIVATE "-framework AppKit" "-framework CoreGraphics")
else()
    target_sources(Time PRIVATE Source/ActivityProbe_other.cpp)
endif()

target_compile_definitions(Time PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# Les textes de l'interface sont en français (UTF-8) : on le dit à Visual Studio.
if(MSVC)
    target_compile_options(Time PRIVATE /utf-8)
endif()

target_link_libraries(Time
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# -----------------------------------------------------------------------------
#  macOS : signature "ad-hoc" automatique du .vst3 après compilation,
#  pour que le plugin se charge sans blocage de sécurité.
# -----------------------------------------------------------------------------
if(APPLE)
    add_custom_command(TARGET Time_VST3 POST_BUILD
        COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:Time_VST3>"
        COMMENT "Signature ad-hoc de Time.vst3")
endif()

# -----------------------------------------------------------------------------
#  Tests de la logique de chronométrage (optionnel, pour développeurs)
#  cmake -B build -DTIME_BUILD_TESTS=ON
# -----------------------------------------------------------------------------
option(TIME_BUILD_TESTS "Compiler les tests de la logique du chrono" OFF)
if(TIME_BUILD_TESTS)
    add_executable(TimeTests Tests/LiveClockTests.cpp Source/LiveClock.cpp)
    target_include_directories(TimeTests PRIVATE Source)
endif()
