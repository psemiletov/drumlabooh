/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "topheader.h"

//#include "drumcell.h"


   typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;


class CAudioProcessorEditor;


class CDrumLine  : public juce::Component,
                        public juce::Slider::Listener
{
public:
    //==============================================================================


    juce::GroupComponent gr_group;
    juce::Slider sl_pan;
    juce::Slider sl_gain;

    juce::Label label;

    CDrumLine ();
    ~CDrumLine() override;


    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    int cell_number;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_gain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_pan;

    void attach_params (CAudioProcessorEditor *ed, int cellno);

    //[/UserMethods]

    void set_name (const std::string &n);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void sliderValueChanged (juce::Slider* sliderThatWasMoved) override;


    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CDrumLine)
};



class CAudioProcessorEditor: public juce::AudioProcessorEditor,
                             public juce::Button::Listener,
                             public juce::ComboBox::Listener

{
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CAudioProcessor& audioProcessor;


public:

    CHydrogenKitsScanner kits_scanner;

    juce::AudioProcessorValueTreeState& valueTreeState;

  //  juce::MidiKeyboardState keyboardState;

    juce::Label gainLabel;
    juce::Slider gainSlider;

    std::unique_ptr<SliderAttachment> gainAttachment;
    juce::ToggleButton invertButton;
    std::unique_ptr<ButtonAttachment> invertAttachment;


    //CDrumCell dc;
    //CDrumLine dl;

    CDrumLine drumlines [36];


public:
    //NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    CAudioProcessorEditor (CAudioProcessor& parent, juce::AudioProcessorValueTreeState& vts);
    ~CAudioProcessorEditor() override;



    //==============================================================================

    juce::Component drumlines_container;
    juce::Viewport drumlines_viewer;

    juce::Label l_panner_mode { {}, "Panner mode" };
    juce::Font f_default_font { 12.0f };
    juce::ComboBox cmb_panner_mode;

    juce::Slider midiVolume;

    juce::TextButton bt_test;
    juce::Label timeLabel;


    juce::Label l_drumkit_selector { {}, "Drumkit selector" };
    //juce::Font f_default_font { 12.0f };
    juce::ComboBox cmb_drumkit_selector;


   // void save_string_keyval (const std::string &key, const std::string &val);
 //   std::string load_string_keyval (const std::string &key);

    void buttonClicked (juce::Button* button) override;

    void load_kit (const std::string &kitpath);
    void kit_changed();


  void paint (juce::Graphics&) override;
    void resized() override;

    void panner_modeMenuChanged();

    void comboBoxChanged(juce::ComboBox *comboBox) override;

    // These methods handle callbacks from the midi device + on-screen keyboard..
    /*void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        const juce::ScopedValueSetter<bool> scopedInputFlag (isAddingFromMidiInput, true);
        keyboardState.processNextMidiEvent (message);
        postMessageToList (message, source->getName());
    }
*/


     JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CAudioProcessorEditor)
};
