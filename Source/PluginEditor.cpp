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

  kits_scanner.scan();

  for (size_t i = 0; i < kits_scanner.v_kits_names.size(); i++)
     {
    //  std::cout << kits_scanner.v_kits_names[i] << std::endl;
    //  std::cout << "i:" << i << std::endl;

      cmb_drumkit_selector.addItem (kits_scanner.v_kits_names[i], i + 1);
    }



  //kits_scanner.v_kits_names [cmb_drumkit_selector.getSelectedId() - 1];

 // addAndMakeVisible (top_header);
   // top_header.setTopLeftPosition(1, 1);
/*
  auto addons = valueTreeState.state.getOrCreateChildWithName ("addons", nullptr);
  auto text = addons.getProperty ("text", juce::String()).toString();
  std::cout << "!!!!!!!!!!!!!!!!!!!!!! " << text << std::endl;
*/

    int yoffs = 0;
    int xoffs = 0;


    cmb_drumkit_selector.setTextWhenNothingSelected ("CLICK HERE TO SELECT THE DRUMKIT");
    cmb_drumkit_selector.setTextWhenNoChoicesAvailable ("NO DRUMKITS FOUND");
    cmb_drumkit_selector.setScrollWheelEnabled (true);

    addAndMakeVisible (cmb_drumkit_selector);

    //cmb_drumkit_selector.onChange = [this] { kit_changed(); };
    cmb_drumkit_selector.addListener (this);

    cmb_drumkit_selector.setTopLeftPosition (0, 0);
    cmb_drumkit_selector.setSize (780, 48);

    yoffs += cmb_drumkit_selector.getHeight();

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
/*
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
*/
//        setSize (paramSliderWidth + paramLabelWidth, juce::jmax (100, paramControlHeight * 2));


        drumlines_container.setSize (780, drumlines[0].getHeight() * 36);
      //  drumlines_container.addAndMakeVisible (dl);

     //   dl.setTopLeftPosition(1, 1);
     //   dl.attach_params (this, 0);


        for (size_t i = 0; i < 36; i++)
            {
             drumlines[i].setTopLeftPosition (0, i * drumlines[i].getHeight());
             drumlines_container.addAndMakeVisible (drumlines[i]);
             drumlines[i].attach_params (this, i);
            }

/////////////*******************8

        load_kit (audioProcessor.drumkit_path);

        /////////////////************8




        drumlines_viewer.setViewedComponent (&drumlines_container, false);
        drumlines_viewer.setScrollBarsShown	(true, false);
        drumlines_viewer.setSize (drumlines[0].getWidth() + 24, 480);
        drumlines_viewer.setScrollBarThickness (24);

        drumlines_viewer.setTopLeftPosition (0, yoffs + YFILLER);

        addAndMakeVisible (drumlines_viewer);


        setSize (780, 720);



        bt_test.setBounds (drumlines[0].getWidth() + 36, 100, 50, 50);


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

}


void CAudioProcessorEditor::buttonClicked (juce::Button* button)
  {
  if (button == &bt_test)
    {
     std::cout << "NewProjectAudioProcessorEditor::buttonClicked " << std::endl;

    //         load_kit (audioProcessor.drumkit_path);

     audioProcessor.drumkit->print_stats();

       // auto addons = valueTreeState.state.getOrCreateChildWithName ("addons", nullptr);
        //addons.setProperty ("text", "Lorem Ipsum", nullptr);


//     std::cout << "SAVE"  << std::endl;

    // audioProcessor.drumkit_name = "BUUUUUU";
  //   audioProcessor.save_string_keyval ("drumkit_name", audioProcessor.drumkit_name);
/*
     std::cout << "LOAD"  << std::endl;

     audioProcessor.drumkit_name = audioProcessor.load_string_keyval ("drumkit_name");

     std::cout << "drumkit_name: " << audioProcessor.drumkit_name  << std::endl;
*/

      // std::cout << "save drumkit_name: " << drumkit_name << std::endl;





//     audioProcessor.save_string_keyval ("drumkit_name", "YAYAYAY");

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





void CAudioProcessorEditor::kit_changed()
    {
        //(cmb_drumkit_selector.getSelectedId()
      std::cout << cmb_drumkit_selector.getSelectedId() - 1<< std::endl;

    }




void CAudioProcessorEditor::comboBoxChanged(juce::ComboBox *comboBox)
{

        std::cout << "CAudioProcessorEditor::comboBoxChanged" << std::endl;


   if (comboBox == &cmb_drumkit_selector)
    {

      std::cout << cmb_drumkit_selector.getSelectedId() - 1<< std::endl;

      int id = cmb_drumkit_selector.getSelectedId();
      if (id == 0)
         return;


      std::string full = kits_scanner.map_kits[kits_scanner.v_kits_names [cmb_drumkit_selector.getSelectedId() - 1]];
      std::cout << "FULL: " << full << std::endl;

      audioProcessor.drumkit_path = full;

      audioProcessor.load_kit (full);

      //update GUI
      load_kit (full);



      //audioProcessor.drumkit_name = kits_scanner.v_kits_names [cmb_drumkit_selector.getSelectedId() - 1];



//      juce::String sk = cmb_drumkit_selector::getItemText(cmb_drumkit_selector.getSelectedId());

      //std::cout << kits_scanner.v_kits_names [cmb_drumkit_selector.getSelectedId() - 1] << std::endl;


        //[UserSliderCode_sl_pan] -- add your slider handling code here..
        //[/UserSliderCode_sl_pan]
    }


}




CDrumLine::CDrumLine ()
{

    addChildComponent (gr_group);
  //  gr_group.setTextLabelPosition (juce::Justification::centredLeft);

//    gr_group.setBounds (0, 0, 180, 144);

    int xoffs = XFILLER * 2;
    //int yoffs = 4;

    addAndMakeVisible (label);
    label.setTopLeftPosition (xoffs, YFILLER);
    label.setSize (160, 32);

    label.setColour (juce::Label::textColourId, juce::Colours::black);
    label.setColour (juce::Label::backgroundColourId, juce::Colour (255, 222, 89));
    label.setFont (f_samplename_font);

    label.setEditable (true);

    label.setText ("EMPTY CELL", juce::dontSendNotification);

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


void CDrumLine::attach_params (CAudioProcessorEditor *ed, int cellno)
{

  //if (! ed)
    // std::cout << "!!!!! " << std::endl;

  cell_number = cellno;

  std::string param_name = "gain" + std::to_string (cell_number);

 // std::cout << "param_name:" << param_name << std::endl;


  att_gain.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_gain));

  param_name = "pan" + std::to_string (cell_number);

  att_pan.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_pan));

}


void CDrumLine::set_name (const std::string &n)
{
  label.setText (n.c_str(), juce::dontSendNotification);

}


void CAudioProcessorEditor::load_kit (const std::string &kitpath)
{
  //STOP PLAY

  //THEN
  //make all drum labels empyy

   for (size_t i = 0; i < 36; i++)
       {
        drumlines[i].set_name ("EMPTY CELL");
       }

  if (kits_scanner.v_scanned_kits.size() == 0)
      return;


  //find kit at v_scanned_kits

  CHydrogenKit *k = 0;

  for (size_t i = 0; i < kits_scanner.v_scanned_kits.size() ; i++)
      {
       if (kits_scanner.v_scanned_kits[i]->kit_filename == kitpath)
          k = kits_scanner.v_scanned_kits[i];
      }

   if (! k)
      return;

  for (size_t i = 0; i < k->v_samples.size(); i++)
       {
        drumlines[i].set_name (k->v_samples[i]->name);
       }


}
