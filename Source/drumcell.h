/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 7.0.5

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include <JuceHeader.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class CDrumCell  : public juce::Component,
                   public juce::Slider::Listener
{
public:


    //==============================================================================
    CDrumCell ();
    ~CDrumCell() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    int cell_number;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_gain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_pan;

    void attach_params();

    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void sliderValueChanged (juce::Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<juce::GroupComponent> gr_group;
    std::unique_ptr<juce::Slider> sl_pan;
    std::unique_ptr<juce::Slider> sl_gain;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CDrumCell)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

