/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


//#include "drumcell.h"


   typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
   typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;


class CAudioProcessorEditor;


class CDrumLine: public juce::Component,
                public juce::Slider::Listener
{
public:

    juce::Font f_samplename_font { 14.0f, juce::Font::bold};



    juce::GroupComponent gr_group;
    juce::Slider sl_pan;
    juce::Slider sl_gain;

    juce::Label label;
    int cell_number;

    CDrumLine ();
    ~CDrumLine() override;


    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_gain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_pan;


    void attach_params (CAudioProcessorEditor *ed, int cellno);
    void set_name (const std::string &n);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void sliderValueChanged (juce::Slider* sliderThatWasMoved) override;

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

    CDrumKitsScanner kits_scanner;

    juce::AudioProcessorValueTreeState& valueTreeState;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att_pan_mode;
    juce::Label l_pan_mode { {}, "Pan mode" };
    juce::ComboBox cmb_pan_mode;

    juce::GroupComponent gr_kitinfo;
    juce::Label l_kitinfo;

  //  juce::MidiKeyboardState keyboardState;

    juce::Label gainLabel;
    juce::Slider gainSlider;

    juce::Slider testSlider;


    std::unique_ptr<SliderAttachment> gainAttachment;
    juce::ToggleButton invertButton;
    std::unique_ptr<ButtonAttachment> invertAttachment;


    juce::Font f_kitname_font { 20.0f, juce::Font::bold};
    juce::ImageComponent kit_image;


    juce::GroupComponent drumlines_group;
    juce::Component drumlines_container;
    juce::Viewport drumlines_viewer;


    juce::TextButton bt_test;

    juce::Label l_drumkit_name;
    juce::TextButton bt_drumkit_info;



    juce::Label l_drumkit_selector { {}, "Drumkit selector" };
    juce::ComboBox cmb_drumkit_selector;



    CDrumLine drumlines [36];


    CAudioProcessorEditor (CAudioProcessor& parent, juce::AudioProcessorValueTreeState& vts);
    ~CAudioProcessorEditor() override;


    //==============================================================================

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
