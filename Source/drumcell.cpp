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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "drumcell.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
CDrumCellProto::CDrumCellProto ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    gr_group.reset (new juce::GroupComponent ("new group",
                                              TRANS("drum name")));
    addAndMakeVisible (gr_group.get());
    gr_group->setTextLabelPosition (juce::Justification::centredLeft);

    gr_group->setBounds (0, 0, 180, 144);

    sl_pan.reset (new juce::Slider ("new slider"));
    addAndMakeVisible (sl_pan.get());
    sl_pan->setRange (0, 1, 0.01);
    sl_pan->setSliderStyle (juce::Slider::RotaryHorizontalDrag);
    sl_pan->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    sl_pan->addListener (this);

    sl_pan->setBounds (8, 16, 104, 120);

    sl_gain.reset (new juce::Slider ("new slider"));
    addAndMakeVisible (sl_gain.get());
    sl_gain->setRange (0, 10, 0);
    sl_gain->setSliderStyle (juce::Slider::LinearVertical);
    sl_gain->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    sl_gain->addListener (this);

    sl_gain->setBounds (112, 16, 56, 120);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (180, 144);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

CDrumCellProto::~CDrumCellProto()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    gr_group = nullptr;
    sl_pan = nullptr;
    sl_gain = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void CDrumCellProto::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void CDrumCellProto::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void CDrumCellProto::sliderValueChanged (juce::Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == sl_pan.get())
    {
        //[UserSliderCode_sl_pan] -- add your slider handling code here..
        //[/UserSliderCode_sl_pan]
    }
    else if (sliderThatWasMoved == sl_gain.get())
    {
        //[UserSliderCode_sl_gain] -- add your slider handling code here..
        //[/UserSliderCode_sl_gain]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="CDrumCellProto" componentName=""
                 parentClasses="public juce::Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="180" initialHeight="144">
  <BACKGROUND backgroundColour="ff323e44"/>
  <GROUPCOMPONENT name="new group" id="26e746b5a4e672d9" memberName="gr_group"
                  virtualName="" explicitFocusOrder="0" pos="0 0 180 144" title="drum name"
                  textpos="33"/>
  <SLIDER name="new slider" id="62310cabfe4b0c55" memberName="sl_pan" virtualName=""
          explicitFocusOrder="0" pos="8 16 104 120" min="0.0" max="1.0"
          int="0.01" style="RotaryHorizontalDrag" textBoxPos="TextBoxBelow"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
  <SLIDER name="new slider" id="ad0bdbc5d4956c13" memberName="sl_gain"
          virtualName="" explicitFocusOrder="0" pos="112 16 56 120" min="0.0"
          max="10.0" int="0.0" style="LinearVertical" textBoxPos="TextBoxBelow"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

