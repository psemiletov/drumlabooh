
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"



typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;


class CAudioProcessorEditor;

class CTimer: public juce::HighResolutionTimer
{
public:
    CTimer() {};
    CAudioProcessorEditor *uplink = 0;
   //void timerCallback() override;
   void hiResTimerCallback() override;

};



/*
class CToggleButton: public juce::ToggleButton
{

void paintButton 	( 	juce::Graphics &  	g,
		bool  	shouldDrawButtonAsHighlighted,
		bool  	shouldDrawButtonAsDown
	) override;

}
*/
class CLed: public juce::Component
{
public:
    juce::Colour cl_on = juce::Colours::gold;
    juce::Colour cl_off = juce::Colours::grey;
    bool is_on = false;

    void paint(Graphics& g);

};

class CDrumLine: public juce::Component/*,
                 public juce::Slider::Listener*/
{
public:

    juce::Font f_samplename_font { 14.0f, juce::Font::bold};

    juce::GroupComponent gr_group;
    juce::Slider sl_pan;
    juce::Slider sl_gain;

    juce::ToggleButton bt_mute;
    CLed led;


    juce::Label label;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_gain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_pan;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att_mute;

    int cell_number;

    CDrumLine ();
    ~CDrumLine() override;

    void attach_params (CAudioProcessorEditor *ed, int cellno);
    void set_name (const std::string &n);

    void paint (juce::Graphics& g) override;
    void resized() override;
    //void sliderValueChanged (juce::Slider* sliderThatWasMoved) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CDrumLine)
};



class CAudioProcessorEditor: public juce::AudioProcessorEditor,
                             public juce::Button::Listener,
                             public juce::ComboBox::Listener,
                             public juce::Slider::Listener
{
//private:

public:

    CAudioProcessor& audioProcessor;

    CDrumKitsScanner kits_scanner;

    juce::AudioProcessorValueTreeState& valueTreeState;

    CTimer tmr_leds;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att_ignore_midi_velocity;
    juce::ToggleButton bt_ignore_midi_velocity;


    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att_pan_mode;
    juce::Label l_pan_mode { {}, "Pan mode" };
    juce::ComboBox cmb_pan_mode;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_base_note;
    juce::Label l_base_note { {}, "Base MIDI note" };
    juce::Slider sl_base_note;


    juce::GroupComponent gr_kitinfo;
    juce::Label l_kitinfo;


    juce::Label l_plugin_name;
    juce::HyperlinkButton hl_homepage;


    juce::Slider testSlider;


    std::unique_ptr<SliderAttachment> gainAttachment;
    juce::ToggleButton invertButton;
    std::unique_ptr<ButtonAttachment> invertAttachment;

    juce::Font f_sitelink { 20.0f, juce::Font::bold};
    juce::Font f_logo { 40.0f, juce::Font::bold};


    juce::Font f_kitname_font { 20.0f, juce::Font::bold};
    juce::ImageComponent kit_image;


    juce::GroupComponent drumlines_group;
    juce::Component drumlines_container;
    juce::Viewport drumlines_viewer;


    juce::TextButton bt_test;

    juce::Label l_drumkit_name;
    juce::TextButton bt_drumkit_info;

    juce::GroupComponent gr_options;


    juce::Label l_drumkit_selector { {}, "Drumkit selector" };
    juce::ComboBox cmb_drumkit_selector;

    CDrumLine drumlines [36];


    CAudioProcessorEditor (CAudioProcessor& parent, juce::AudioProcessorValueTreeState& vts);
    ~CAudioProcessorEditor() override;



    void buttonClicked (juce::Button* button) override;

    void load_kit (const std::string &kitpath);
    void kit_changed();

    void paint (juce::Graphics&) override;
    void resized() override;

//    void panner_modeMenuChanged();

    void sliderValueChanged (juce::Slider* slider) override;


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
