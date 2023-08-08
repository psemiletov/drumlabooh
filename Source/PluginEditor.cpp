/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


#include "kits.h"




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

        addAndMakeVisible (dc);


        setSize (640, 480);

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

    dc.setTopLeftPosition(1, 200);


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
