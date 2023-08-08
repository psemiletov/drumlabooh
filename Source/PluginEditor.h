/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "drumcell.h"


   typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;



/*

class CDrumCells: public juce::Viewport
{
// ...
public:

    juce::Component cellholder;

    CDrumCell* cells[36];


    CDrumCells();
    ~CDrumCells();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CDrumCellst)
};

*/
class CAudioProcessorEditor: public juce::AudioProcessorEditor,
                             public juce::Button::Listener

{

private:
    juce::AudioProcessorValueTreeState& valueTreeState;

  //  juce::MidiKeyboardState keyboardState;

    juce::Label gainLabel;
    juce::Slider gainSlider;
    std::unique_ptr<SliderAttachment> gainAttachment;

    juce::ToggleButton invertButton;
    std::unique_ptr<ButtonAttachment> invertAttachment;


    CDrumCell dc;

public:
    //NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    CAudioProcessorEditor (CAudioProcessor& parent, juce::AudioProcessorValueTreeState& vts);
    ~CAudioProcessorEditor() override;



    //==============================================================================

    juce::Label l_panner_mode { {}, "Panner mode" };
    juce::Font f_default_font { 12.0f };
    juce::ComboBox cmb_panner_mode;

     juce::Slider midiVolume;

    juce::TextButton bt_test;
    juce::Label timeLabel;

    void buttonClicked (juce::Button* button) override;

  void paint (juce::Graphics&) override;
    void resized() override;

    void panner_modeMenuChanged();


    // These methods handle callbacks from the midi device + on-screen keyboard..
    /*void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        const juce::ScopedValueSetter<bool> scopedInputFlag (isAddingFromMidiInput, true);
        keyboardState.processNextMidiEvent (message);
        postMessageToList (message, source->getName());
    }
*/


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CAudioProcessor& audioProcessor;

     JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CAudioProcessorEditor)
};
