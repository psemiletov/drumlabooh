# Example Audio Plugin CMakeLists.txt

# To get started on a new plugin, copy this entire folder (containing this file and C++ sources) to
# a convenient location, and then start making modifications.

# The first line of any CMake project should be a call to `cmake_minimum_required`, which checks
# that the installed CMake will be able to understand the following CMakeLists, and ensures that
# CMake's behaviour is compatible with the named version. This is a standard CMake command, so more
# information can be found in the CMake docs.

cmake_minimum_required(VERSION 3.15)
include(FetchContent)
# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.

project(DRUMPECKER LANGUAGES CXX C VERSION 0.0.1 )


#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3 ")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")


FetchContent_Declare(
  juce
  GIT_REPOSITORY git@github.com:juce-framework/JUCE.git
  GIT_TAG        d24c2729268e322f3ba1b5070eb96ab232d7f6ba # release-7.0.6
)

FetchContent_MakeAvailable(juce)

#FetchContent_Declare(
#  libsamplerate
#  GIT_REPOSITORY git@github.com:libsndfile/libsamplerate.git
#  GIT_TAG        c96f5e3de9c4488f4e6c97f59f5245f22fda22f7 # release-0.2.2
#)

#FetchContent_MakeAvailable(libsamplerate)


#FetchContent_GetProperties(libsamplerate)
    #if(NOT libsamplerate_POPULATED)
      #message("Cloning libsamplerate")
#      FetchContent_Populate(libsamplerate)
      #add_subdirectory(
      #  ${libsamplerate_SOURCE_DIR}
        #${libsamplerate_BINARY_DIR})
    #endif()


#static linking in Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# include JUCE *AFTER* the MSVC runtime and so on is set up

# If you've installed JUCE somehow (via a package manager, or directly using the CMake install
# target), you'll need to tell this project that it depends on the installed copy of JUCE. If you've
# included JUCE directly in your source tree (perhaps as a submodule), you'll need to tell CMake to
# include that subdirectory as part of the build.

# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# or
#add_subdirectory(${juce_SOURCE_DIR} ${juce_BINARY_DIR})

# If you are building a VST2 or AAX plugin, CMake needs to be told where to find these SDKs on your
# system. This setup should be done before calling `juce_add_plugin`.

# juce_set_vst2_sdk_path(...)
# juce_set_aax_sdk_path(...)

# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (AudioPluginExample here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.


# default plugin formats
#set(JUCE_FORMATS AU VST3 Standalone)
set(JUCE_FORMATS VST3 Standalone)
# Build LV2 only on Linux
if(UNIX AND NOT APPLE)
    message(STATUS "Building LV2 plugin format")
    list(APPEND JUCE_FORMATS LV2)
endif()


juce_add_plugin(drumpecker
    # VERSION ...                               # Set this if the plugin version is different to the project version
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
     JUCE_DISPLAY_SPLASH_SCREEN=0
     LV2URI https://github.com/psemiletov/drumpecker
     LV2_SHARED_LIBRARY_NAME drumpecker
     COMPANY_NAME Semiletov                          # Specify the name of the plugin's author
     IS_SYNTH TRUE                       # Is this a synth or an effect?
     NEEDS_MIDI_INPUT TRUE               # Does the plugin need midi input?
     VST3_CATEGORIES Instrument Sampler Synth Drum
    # NEEDS_MIDI_OUTPUT TRUE/FALSE              # Does the plugin need midi output?
    # IS_MIDI_EFFECT TRUE/FALSE                 # Is this plugin a MIDI effect?
     EDITOR_WANTS_KEYBOARD_FOCUS TRUE    # Does the editor need keyboard focus?
    # COPY_PLUGIN_AFTER_BUILD TRUE/FALSE        # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE PetR               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Petr                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS ${JUCE_FORMATS}                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "drumpecker")        # The name of the final executable, which can differ from the target name

# `juce_generate_juce_header` will create a JuceHeader.h for a given target, which will be generated
# into your build tree. This should be included with `#include <JuceHeader.h>`. The include path for
# this header will be automatically added to the target. The main function of the JuceHeader is to
# include all your JUCE module headers; if you're happy to include module headers directly, you
# probably don't need to call this.

 juce_generate_juce_header(drumpecker)

# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.

target_sources(drumpecker
    PRIVATE
        ./Source/PluginEditor.cpp
        ./Source/PluginProcessor.cpp
        ./Source/pugixml.cpp
        ./Source/dsp.cpp
        ./Source/kits.cpp
        ./Source/utl.cpp
./Source/exceptions.cpp
./Source/resampler_class.cpp
./Source/resampler_function.cpp
./Source/speex_resampler.c
)

# `target_compile_definitions` adds some preprocessor definitions to our target. In a Projucer
# project, these might be passed in the 'Preprocessor Definitions' field. JUCE modules also make use
# of compile definitions to switch certain features on/off, so if there's a particular feature you
# need that's not on by default, check the module header for the correct flag to set here. These
# definitions will be visible both to your code, and also the JUCE module code, so for new
# definitions, pick unique names that are unlikely to collide! This is a standard CMake command.

target_compile_definitions(drumpecker
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_JACK=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built.

# juce_add_binary_data(AudioPluginData SOURCES ...)

# `target_link_libraries` links libraries and JUCE modules to other libraries or executables. Here,
# we're linking our executable target to the `juce::juce_audio_utils` module. Inter-module
# dependencies are resolved automatically, so `juce_core`, `juce_events` and so on will also be
# linked automatically. If we'd generated a binary data target above, we would need to link to it
# here too. This is a standard CMake command.



#target_include_directories(drumpecker
#                           PUBLIC
#                            ${libsamplerate_INCLUDE_DIR})

#add_library(libsamplerate STATIC
            #${libsamplerate_SOURCE_DIR})


#set_target_properties(libsamplerate PROPERTIES LINKER_LANGUAGE C)


target_link_libraries(drumpecker
    PRIVATE
#        libsamplerate
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)