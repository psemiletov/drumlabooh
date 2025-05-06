/*
written at 2023-24 by Peter Semiletov
this code is the public domain
 */


#pragma once

#include <JuceHeader.h>


#include "kits.h"
#include "dsp.h"
#include "fx-resofilter.h"

#define MIDIMAPMODE_LABOOH 1
#define MIDIMAPMODE_FROMKIT 2

class CAudioProcessor: public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif

{
public:

  juce::AudioProcessorValueTreeState parameters;

  // indexes (layers) for the bundle
  
//  std::atomic<float>* layer_index[36]; //int

   int layer_index[36]; //нужно только для загрузки параметров

  
  
  //all mixer strip (drumcell) params

#ifndef MULTICHANNEL

  std::atomic<float>* pans[36];
  std::atomic<float>* vols[36];
  std::atomic<float>* mutes[36]; //bool

#endif

  //FX LP, HP, Analog on/off

  std::atomic<float>* lps[36]; //bool
  std::atomic<float>* hps[36]; //bool
  std::atomic<float>* analog[36]; //bool

  //FX controls

  std::atomic<float>* lp_cutoff[36]; //
  std::atomic<float>* lp_reso[36]; //

  std::atomic<float>* hp_cutoff[36]; //
  std::atomic<float>* hp_reso[36]; //

  std::atomic<float>* analog_amount[36]; //


#ifndef MULTICHANNEL

  std::atomic<float>* panner_mode = nullptr;

#endif


  std::atomic<float>* midimap_mode = nullptr;
  std::atomic<float>* ignore_midi_velocity = nullptr;
  std::atomic<float>* global_analog_on = nullptr;
  std::atomic<float>* global_analog_amount = nullptr;

  //Filters

  CResoFilter lp[36];
  CResoFilter hp[36];


  int base_note_number; //default 36, the base for note number mapping to array index (i.e. MIDI note minus base note to get the array index)

  bool fresh_start; //need to determine when we need to load the kit
  //see  CAudioProcessor::processBlock
  //invented as hack to Ardour

  std::string drumkit_path; //full path!

  CDrumKitsScanner scanner;
  CDrumKit *drumkit;

  int session_samplerate; //hot session samlerate, sometimes taken many times at different DAW's


  CAudioProcessor();
  ~CAudioProcessor() override;


//load the drumkit
  bool load_kit (const std::string &fullpath);

//utility functions to deal with parameters
  void save_string_keyval (const std::string &key, const std::string &val);
  std::string load_string_keyval (const std::string &key);
  void save_int_keyval (const std::string &key, int val);
  int load_int_keyval (const std::string &key, int defval);


  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;


//#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
//#endif

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
