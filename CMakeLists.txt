cmake_minimum_required(VERSION 3.15)
include(FetchContent)


project(DRUMLABOOH LANGUAGES CXX C VERSION 7.0.0)

#add_definitions(-DVERSION_NUMBER="\\"${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}\\"")

add_definitions(-DVERSION_NUMBER="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")


message("Compiler ID: ${CMAKE_CXX_COMPILER_ID}")

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3 ")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3 -ffast-math")


option(MULTI "Build with multy-channel support" ON)
option(INSTALLKITS "Bundle and install kits" ON)
option(LOCALINSTALL "Install to the $HOME subdirs" OFF)

if (LOCALINSTALL)

set(LV2_INSTALL_DIR "$ENV{HOME}$ENV{USERNAME}/.lv2" CACHE PATH "Specifies where the LV2 libraries should be installed")
set(VST3_INSTALL_DIR "$ENV{HOME}$ENV{USERNAME}/.vst3" CACHE PATH "Specifies where the VST3 libraries should be installed")


message("LOCALINSTALL YES")
message(${LV2_INSTALL_DIR})


else()

set(LV2_INSTALL_DIR lib/lv2 CACHE PATH "Specifies where the LV2 libraries should be installed")
set(VST3_INSTALL_DIR lib/vst3 CACHE PATH "Specifies where the VST3 libraries should be installed")


endif()


IF (WIN32)
set (JUCETAG 7.0.10)
  # set stuff for windows
ELSE()
set (JUCETAG 8.0.1)
  # set stuff for other systems
ENDIF()



set(FETCHCONTENT_QUIET FALSE)


#FetchContent_Declare(
#  juce
#  GIT_REPOSITORY git@github.com:juce-framework/JUCE.git
#  GIT_TAG        d24c2729268e322f3ba1b5070eb96ab232d7f6ba # release-7.0.6
#)


#static linking in Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


if (INSTALLKITS)

if(CMAKE_VERSION VERSION_LESS "3.2.8") 

message("CMake less than 3.2.8")

FetchContent_Declare(
  drum_sklad
  URL https://github.com/psemiletov/drum_sklad/archive/refs/heads/main.zip
  GIT_PROGRESS TRUE
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
#since cmake 3.28
#  EXCLUDE_FROM_ALL
#  
)

FetchContent_GetProperties(drum_sklad)
if(NOT drum_sklad_POPULATED)
  FetchContent_Populate(drum_sklad)
  message (${drum_sklad_SOURCE_DIR})
endif()


else()    

FetchContent_Declare(
  drum_sklad
  URL https://github.com/psemiletov/drum_sklad/archive/refs/heads/main.zip
  GIT_PROGRESS TRUE
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
#since cmake 3.28
  EXCLUDE_FROM_ALL
#  
)

FetchContent_MakeAvailable(drum_sklad)

endif()

endif()


if(CMAKE_VERSION VERSION_LESS "3.2.8") 

FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
#  GIT_TAG        2a27ebcfae7ca7f6eb62b29d5f002ceefdaadbdb # release-7.0.7
#  GIT_TAG         7.0.10 # release-7.0.10
#  GIT_TAG         8.0.1 
   GIT_TAG ${JUCETAG}
#since cmake 3.28
#  EXCLUDE_FROM_ALL
#  
  GIT_PROGRESS TRUE
)

FetchContent_GetProperties(juce)
if(NOT juce_POPULATED)
  FetchContent_Populate(juce)
  add_subdirectory(${juce_SOURCE_DIR} ${juce_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


else()

FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG ${JUCETAG}

#  GIT_TAG         7.0.10 # release-7.0.10

#GIT_TAG         8.0.1 
#since cmake 3.28
  EXCLUDE_FROM_ALL
#  
  GIT_PROGRESS TRUE
)


FetchContent_MakeAvailable(juce)

endif()



# default plugin formats
#set(JUCE_FORMATS AU VST3 Standalone)
set(JUCE_FORMATS VST3 Standalone)
# Build LV2 only on Linux
if(UNIX AND NOT APPLE)
    message(STATUS "Building LV2 plugin format")
    list(APPEND JUCE_FORMATS LV2)
endif()

add_definitions(-DJUCE_DISPLAY_SPLASH_SCREEN=0 -DJUCE_USE_MP3AUDIOFORMAT=1)


juce_add_plugin(drumlabooh
    # VERSION ...                               # Set this if the plugin version is different to the project version
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
     NEEDS_WEB_BROWSER FALSE
     JUCE_DISPLAY_SPLASH_SCREEN=0
     LV2URI https://github.com/psemiletov/drumlabooh
     LV2_SHARED_LIBRARY_NAME drumlabooh
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
     PRODUCT_NAME "drumlabooh")        # The name of the final executable, which can differ from the target name

# `juce_generate_juce_header` will create a JuceHeader.h for a given target, which will be generated
# into your build tree. This should be included with `#include <JuceHeader.h>`. The include path for
# this header will be automatically added to the target. The main function of the JuceHeader is to
# include all your JUCE module headers; if you're happy to include module headers directly, you
# probably don't need to call this.

juce_generate_juce_header(drumlabooh)

# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.

target_sources(drumlabooh
    PRIVATE
        ./Source/PluginEditor.cpp
        ./Source/PluginProcessor.cpp
        ./Source/pugixml.cpp
        ./Source/dsp.cpp
        ./Source/kits.cpp
        ./Source/utl.cpp
        ./Source/fx-resofilter.cpp
        ./Source/exceptions.cpp
        ./Source/resampler_class.cpp
        ./Source/resampler_function.cpp
        ./Source/speex_resampler.c
)


target_compile_definitions(drumlabooh
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_JACK=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")


target_link_libraries(drumlabooh
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        )


install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/drumlabooh_artefacts/LV2/drumlabooh.lv2 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lv2)
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/drumlabooh_artefacts/VST3/drumlabooh.vst3 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/vst3)

if (INSTALLKITS)
if (LOCALINSTALL)
install (DIRECTORY ${drum_sklad_SOURCE_DIR}/ DESTINATION $ENV{HOME}$ENV{USERNAME}/drum_sklad)
else()
install (DIRECTORY ${drum_sklad_SOURCE_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share/drumlabooh-kits)
endif()
endif()



if(MULTI)
#  add_definitions(-DMULTICHANNEL=1)
  message("+ Build with multi channel support")

  juce_add_plugin(drumlabooh-multi
     JUCE_DISPLAY_SPLASH_SCREEN=0
     NEEDS_WEB_BROWSER FALSE
     LV2URI https://github.com/psemiletov/drumlabooh-multi
     LV2_SHARED_LIBRARY_NAME drumlabooh-multi
     COMPANY_NAME Semiletov                     # Specify the name of the plugin's author
     IS_SYNTH TRUE                       # Is this a synth or an effect?
     NEEDS_MIDI_INPUT TRUE               # Does the plugin need midi input?
     VST3_CATEGORIES Instrument Sampler Synth Drum
     EDITOR_WANTS_KEYBOARD_FOCUS TRUE    # Does the editor need keyboard focus?
     PLUGIN_MANUFACTURER_CODE PEtR               # A four-character manufacturer id with at least one upper-case character
     PLUGIN_CODE PetR                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS ${JUCE_FORMATS}                     # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "drumlabooh-multi")        # The name of the final executable, which can differ from the target name

juce_generate_juce_header(drumlabooh-multi)




target_sources(drumlabooh-multi
    PRIVATE
        ./Source/PluginEditor.cpp
        ./Source/PluginProcessor.cpp
        ./Source/pugixml.cpp
        ./Source/dsp.cpp
        ./Source/kits.cpp
        ./Source/utl.cpp
        ./Source/fx-resofilter.cpp
        ./Source/exceptions.cpp
        ./Source/resampler_class.cpp
        ./Source/resampler_function.cpp
        ./Source/speex_resampler.c
)

  target_compile_definitions(drumlabooh-multi
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        -DMULTICHANNEL=1
        JUCE_JACK=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)


target_link_libraries(drumlabooh-multi
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        )


install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/drumlabooh-multi_artefacts/LV2/drumlabooh-multi.lv2 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lv2)
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/drumlabooh-multi_artefacts/VST3/drumlabooh-multi.vst3 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/vst3)

endif(MULTI)
