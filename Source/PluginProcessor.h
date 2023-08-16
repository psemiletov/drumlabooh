/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "kits.h"


class CAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif

{
public:

    juce::AudioProcessorValueTreeState parameters;

    std::atomic<float>* pans[36];
    std::atomic<float>* gains[36];

 //   std::atomic<float>* base_note_number = nullptr;
    std::atomic<float>* panner_mode = nullptr;

    int int_base_note_number;


    std::string drumkit_path; //full path!

    CDrumKitsScanner scanner;
    CDrumKit *drumkit;

    int session_samplerate; //sess rate


    CAudioProcessor();
    ~CAudioProcessor() override;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    bool load_kit (const std::string &fullpath);

    void save_string_keyval (const std::string &key, const std::string &val);
    std::string load_string_keyval (const std::string &key);

    void save_int_keyval (const std::string &key, int val);
    int load_int_keyval (const std::string &key);


    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CAudioProcessor)
};
