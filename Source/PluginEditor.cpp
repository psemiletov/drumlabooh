/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"




#include "kits.h"

#define XFILLER 4
#define YFILLER 16


//#define TESTKIT "/home/rox/sfz-kits/Vintage Kit/Vintage Kit.sfz"

//#define TESTKIT "/home/rox/.hydrogen/data/drumkits/AVL-BlackPearl-5-1.1/drumkit.xml"

//#define TESTKIT "/home/rox/.hydrogen/data/drumkits/HipHop-1/drumkit.xml"


#define TESTKIT "/home/rox/drumrox-kits/Lel-DR8/drumkit.txt"

//#define TESTKIT "/home/rox/drumrox-kits/tamil/drumkit.txt"

//#define TESTKIT "/home/rox/sfz-kits/BLONDE_BOP_SFZ/BLONDE_BOP.sfz"


//#define TESTKIT "/home/rox/sfz-kits/Drums/Drums V2.sfz"

//#define TESTKIT "/home/rox/sfz-kits/SCC Taiko Drums/SCC Taiko Drums.sfz

//==============================================================================

#define KNOB_WIDTH 50
#define KNOB_HEIGHT 50
#define BETWEEN 3


    enum
    {
        paramControlHeight = 40,
        paramLabelWidth    = 80,
        paramSliderWidth   = 300
    };



void CAudioProcessorEditor::panner_modeMenuChanged()
{
        /*switch (cmb_panner_mode.getSelectedId())
        {
            case 1: textFont.setStyleFlags (juce::Font::plain);  break;
            case 2: textFont.setStyleFlags (juce::Font::bold);   break;
            case 3: textFont.setStyleFlags (juce::Font::italic); break;
            default: break;
        }

        textLabel.setFont (textFont);*/

      std::cout << "cmb_panner_mode: " <<  cmb_panner_mode.getSelectedId() << std::endl;

}

//NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
  //  : AudioProcessorEditor (&p), audioProcessor (p)
CAudioProcessorEditor::CAudioProcessorEditor (CAudioProcessor& parent, juce::AudioProcessorValueTreeState& vts)
: AudioProcessorEditor (&parent), audioProcessor (parent),
          valueTreeState (vts)
{



 // addAndMakeVisible (top_header);
   // top_header.setTopLeftPosition(1, 1);





    addAndMakeVisible (bt_test);
     bt_test.setButtonText ("Test");

       bt_test.addListener (this);
/*
    // these define the parameters of our slider object
    midiVolume.setSliderStyle (juce::Slider::LinearBarVertical);
    midiVolume.setRange (0.0, 127.0, 1.0);
    midiVolume.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
    midiVolume.setPopupDisplayEnabled (true, false, this);
    midiVolume.setTextValueSuffix (" Volume");
    midiVolume.setValue(1.0);

    // this function adds the slider to the editor
    addAndMakeVisible (&midiVolume);
*/


   //addAndMakeVisible (l_panner_mode);
//  l_panner_mode.setFont (f_default_font);
/*
    addAndMakeVisible (cmb_panner_mode);
        cmb_panner_mode.addItem ("Plain",  1);
        cmb_panner_mode.addItem ("Bold",   2);
        cmb_panner_mode.addItem ("Italic", 3);

        cmb_panner_mode.onChange = [this] { panner_modeMenuChanged(); };
        //cmb_panner_mode.setSelectedId (1);

        //setSize (400, 200);

        l_panner_mode.setBounds (1, 50, 50, 50);

        cmb_panner_mode.setBounds (50, 50, 100, 50);

*/
        gainLabel.setText ("Gain", juce::dontSendNotification);
        addAndMakeVisible (gainLabel);

        addAndMakeVisible (gainSlider);
                gainSlider.setRange (0.0, 1.0, 0.01);
   gainSlider.setSliderStyle(juce:: Slider::SliderStyle::RotaryVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox , false, 0, 0);

        gainAttachment.reset (new SliderAttachment (valueTreeState, "gain", gainSlider));


        invertButton.setButtonText ("Invert Phase");
        addAndMakeVisible (invertButton);
        invertAttachment.reset (new ButtonAttachment (valueTreeState, "invertPhase", invertButton));

//        setSize (paramSliderWidth + paramLabelWidth, juce::jmax (100, paramControlHeight * 2));


        drumlines_container.setSize (780, 720);
        drumlines_container.addAndMakeVisible (dl);

        dl.setTopLeftPosition(1, 1);
        dl.attach_params (this, 0);



        drumlines_viewer.setViewedComponent (&drumlines_container, false);
        drumlines_viewer.setSize (780, 480);
        addAndMakeVisible (drumlines_viewer);

        setSize (780, 720);

}

CAudioProcessorEditor::~CAudioProcessorEditor()

{
}

//==============================================================================
void CAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    /*g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
    */

        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));


   //  g.fillAll (juce::Colours::white);

   //set the current drawing colour to black
//    g.setColour (juce::Colours::black);

    // set the font size and draw text to the screen
 //   g.setFont (15.0f);

   // g.drawFittedText ("Midi Volume", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}

void CAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
     // sets the position and size of the slider with arguments (x, y, width, height)
    //midiVolume.setBounds (40, 30, 20, getHeight() - 60);


  bt_test.setBounds (1, 100, 50, 50);
  invertButton.setBounds (1, 160, 70, 70);

  //auto r = getLocalBounds();

  juce::Rectangle <int> r (1, 1, KNOB_WIDTH, KNOB_HEIGHT);
/*
        auto gainRect = r.removeFromTop (paramControlHeight);
        gainLabel .setBounds (gainRect.removeFromLeft (paramLabelWidth));
        gainSlider.setBounds (gainRect);

        invertButton.setBounds (r.removeFromTop (paramControlHeight));
*/



    gainLabel.setBounds (1, 1, 70, 80);

    //r.translate (50, 0);
    gainSlider.setBounds (73, 1, 100, 100);



        //gainSlider.setBounds (gainRect);

//    r.translate (50, 00);


}


void CAudioProcessorEditor::buttonClicked (juce::Button* button)
  {
  if (button == &bt_test)
    {
     std::cout << "NewProjectAudioProcessorEditor::buttonClicked " << std::endl;


   //   CDrumLayer l ("/home/rox/.hydrogen/data/drumkits/acustica_percussoes/01_ganza_forte_44100.flac", 44100);

//        CDrumSample *s = new CDrumSample   (44100);
/*
        CDrumSample s (44100);

        CDrumLayer l (&s);
        l.load("/home/rox/.hydrogen/data/drumkits/BigMono/CLudwigKick-Dyn01.wav");
        l.print();
*/
       // delete s;

//     formatManager = new juce::AudioFormatManager();
     //formatManager->registerBasicFormats();
/*
     CDrumSample s (44100);
     s.add_layer();
//d:
//loading kit: /home/rox/.hydrogen/data/drumkits/AVL-BlackPearl-5-1.1/drumkit.xml
     if (s.v_layers.size() != 0)
         s.v_layers.back()->load ("/home/rox/drumrox-kits/Lel-DR8/bongo.wav");

     s.add_layer();
     s.v_layers.back()->load ("/home/rox/drumrox-kits/Lel-DR8/clhat.wav");


     s.add_layer();
     s.v_layers.back()->load ("/home/rox/drumrox-kits/Lel-DR8/cowbell.wav");

     //cout << "s.v_layers.size: " << s.v_layers.size() << endl;

     //("/home/rox/.hydrogen/data/drumkits/acustica_percussoes/01_ganza_forte_44100.flac", 48000);

     s.print();
*/
/*

   CHydrogenKit k;
   k.load (TESTKIT, 48000);
   k.print();
*/
    // CHydrogenKitsScanner sc;
//     sc.scan();
  //   sc.print();



    // // delete formatManager;

  }
  }

/*
CDrumCells::CDrumCells()
{
  cellholder.setBounds (1, 1, 720, 720);
  setViewedComponent (&cellholder);

  size_t i = 0;

  for (size_t row = 0; row < 6; row++)
  for (size_t col = 0; col < 6; col++)

      {
       CDrumCell *dc = new CDrumCell (i);
       drumscells[i++] = dc;

      }

}

CDrumCells::~CDrumCells()
{
  for (size_t i = 0; i < 36; iow++)
      delete drumscells[i];


}*/



CDrumCell::CDrumCell ()
{

    addChildComponent (gr_group);
    //gr_group.setTextLabelPosition (juce::Justification::centredLeft);


    addAndMakeVisible (sl_pan);
    sl_pan.setRange (0.0f, 1.0f, 0.01f);
    sl_pan.setSliderStyle (juce::Slider::RotaryHorizontalDrag);
    sl_pan.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    sl_pan.addListener (this);

    sl_pan.setBounds (8, 16, 104, 120);

    addAndMakeVisible (sl_gain);
    sl_gain.setRange (-60, 6, 1);
    sl_gain.setSliderStyle (juce::Slider::LinearVertical);
    sl_gain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    sl_gain.addListener (this);

    sl_gain.setBounds (112, 16, 56, 120);

    gr_group.setVisible (true);
    gr_group.setBounds (0, 0, 180, 144);


    setSize (180, 144);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

CDrumCell::~CDrumCell()
{

}

//==============================================================================
void CDrumCell::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void CDrumCell::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void CDrumCell::sliderValueChanged (juce::Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == &sl_pan)
    {
        //[UserSliderCode_sl_pan] -- add your slider handling code here..
        //[/UserSliderCode_sl_pan]
    }
    else if (sliderThatWasMoved == &sl_gain)
    {
        //[UserSliderCode_sl_gain] -- add your slider handling code here..
        //[/UserSliderCode_sl_gain]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}


void  CDrumCell::attach_params (CAudioProcessorEditor *ed, int cellno)
{

  if (! ed)
     std::cout << "!!!!! " << std::endl;

  cell_number = cellno;

  std::string param_name = "gain" + std::to_string (cell_number);

  std::cout << "param_name:" << param_name << std::endl;


  att_gain.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_gain));

  param_name = "pan" + std::to_string (cell_number);

  att_pan.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_pan));

}




//////////




CDrumLine::CDrumLine ()
{

    addChildComponent (gr_group);
  //  gr_group.setTextLabelPosition (juce::Justification::centredLeft);

//    gr_group.setBounds (0, 0, 180, 144);

    int xoffs = 0;
    //int yoffs = 4;

    addAndMakeVisible (label);
    label.setTopLeftPosition (xoffs, YFILLER);
    label.setSize (160, 32);

    label.setText ("TEST", juce::dontSendNotification);

    xoffs += label.getWidth();
    xoffs += XFILLER;

    addAndMakeVisible (sl_pan);
    sl_pan.setTopLeftPosition (xoffs, YFILLER);
    sl_pan.setSize (104, 32);
    sl_pan.setRange (0.0f, 1.0f, 0.01f);

    sl_pan.setSliderStyle (juce::Slider::LinearHorizontal);
    sl_pan.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    sl_pan.setTooltip ("Pan\n");

    sl_pan.addListener (this);


    xoffs += sl_pan.getWidth();
    xoffs += XFILLER;


    addAndMakeVisible (sl_gain);

    sl_gain.setTopLeftPosition (xoffs, YFILLER);
    sl_gain.setSize (104, 32);

    sl_gain.setRange (-60, 6, 1);
    sl_gain.setSliderStyle (juce::Slider::LinearHorizontal);
    sl_gain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    sl_gain.addListener (this);
    sl_pan.setTooltip ("Gain\n");


    xoffs += sl_gain.getWidth();
    xoffs += XFILLER;


 //   setSize (32, label.getWidth() + FILLER + sl_pan.getWidth() + FILLER + sl_gain.getWidth());


     gr_group.setVisible (true);
     gr_group.setSize (xoffs + XFILLER, 32 + YFILLER + YFILLER);

     setSize (xoffs + XFILLER, 32 + YFILLER + YFILLER);



    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

CDrumLine::~CDrumLine()
{

}

//==============================================================================
void CDrumLine::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void CDrumLine::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void CDrumLine::sliderValueChanged (juce::Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == &sl_pan)
    {
        //[UserSliderCode_sl_pan] -- add your slider handling code here..
        //[/UserSliderCode_sl_pan]
    }
    else if (sliderThatWasMoved == &sl_gain)
    {
        //[UserSliderCode_sl_gain] -- add your slider handling code here..
        //[/UserSliderCode_sl_gain]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}


void  CDrumLine::attach_params (CAudioProcessorEditor *ed, int cellno)
{

  //if (! ed)
    // std::cout << "!!!!! " << std::endl;

  cell_number = cellno;

  std::string param_name = "gain" + std::to_string (cell_number);

  std::cout << "param_name:" << param_name << std::endl;


  att_gain.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_gain));

  param_name = "pan" + std::to_string (cell_number);

  att_pan.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_pan));

}

