/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"



#include "kits.h"
#include "utl.h"

#define XFILLER 4
#define YFILLER 16

#define WINDOW_HEIGHT 580
#define WINDOW_WIDTH 780


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




CDrumLine::CDrumLine ()
{
  addChildComponent (gr_group);

  int xoffs = XFILLER * 2;

  addAndMakeVisible (label);
  label.setTopLeftPosition (xoffs, YFILLER);
  label.setSize (160, 32);

  label.setColour (juce::Label::textColourId, juce::Colours::black);
  label.setColour (juce::Label::backgroundColourId, juce::Colour (255, 222, 89));
  label.setFont (f_samplename_font);
//  label.setEditable (true);

  label.setText ("EMPTY CELL", juce::dontSendNotification);

  xoffs += label.getWidth();
  xoffs += XFILLER;

  addAndMakeVisible (sl_pan);
  sl_pan.setTopLeftPosition (xoffs, YFILLER);
  sl_pan.setSize (104, 32);
  sl_pan.setRange (0.0f, 1.0f, 0.01f);


  sl_pan.setSliderStyle (juce::Slider::LinearHorizontal);
  sl_pan.setRange (-96, 6, 1);

  sl_pan.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
  sl_pan.setTooltip ("Pan\n");

    //sl_pan.addListener (this);

  xoffs += sl_pan.getWidth();
  xoffs += XFILLER;

  addAndMakeVisible (sl_gain);

  sl_gain.setTopLeftPosition (xoffs, YFILLER);
  sl_gain.setSize (104, 32);

  sl_gain.setSliderStyle (juce::Slider::LinearHorizontal);
  sl_gain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);

  sl_gain.setRange (-96, 12, 1);
  sl_gain.setSkewFactor (4);

    //sl_gain.addListener (this);

  sl_pan.setTooltip ("Gain\n");

  xoffs += sl_gain.getWidth();
  xoffs += XFILLER;

  gr_group.setVisible (true);
  gr_group.setSize (xoffs + XFILLER, 32 + YFILLER + YFILLER);

  setSize (xoffs + XFILLER, 32 + YFILLER + YFILLER);
}


CDrumLine::~CDrumLine()
{
}


void CDrumLine::paint (juce::Graphics& g)
{
  g.fillAll (juce::Colour (0xff323e44));
}


void CDrumLine::resized()
{
}

/*
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
*/

void CDrumLine::attach_params (CAudioProcessorEditor *ed, int cellno)
{
  cell_number = cellno;

  std::string param_name = "gain" + std::to_string (cell_number);

  att_gain.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_gain));

  param_name = "pan" + std::to_string (cell_number);

  att_pan.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_pan));
}


void CDrumLine::set_name (const std::string &n)
{
  juce::String s (juce::CharPointer_UTF8((n.c_str())));
  label.setText (s, juce::dontSendNotification);

  //label.setText (juce::CharPointer_UTF8((n.c_str()), juce::dontSendNotification);

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

  CDrumKit *k = 0;

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

   juce::String kitname = k->kit_name.c_str();

   l_kitinfo.setText (kitname, juce::dontSendNotification );

   if (file_exists (k->image_fname))
      {
        juce::File fl (k->image_fname);
        juce::Image im = juce::ImageFileFormat::loadFrom (fl);
        kit_image.setImage(im);

      }
   else
       kit_image.setImage(juce::Image ());
}



void CAudioProcessorEditor::panner_modeMenuChanged()
{
       std::cout << "cmb_panner_mode.getSelectedId(): " <<  cmb_pan_mode.getSelectedId() << std::endl;
       std::cout << "cmb_panner_mode.getSelectedItemIndex(): " <<  cmb_pan_mode.getSelectedItemIndex() << std::endl;
}


CAudioProcessorEditor::CAudioProcessorEditor (CAudioProcessor& parent, juce::AudioProcessorValueTreeState& vts)
: AudioProcessorEditor (&parent), audioProcessor (parent),
          valueTreeState (vts)
{

  kits_scanner.scan();

  for (size_t i = 0; i < kits_scanner.v_kits_names.size(); i++)
     {
      cmb_drumkit_selector.addItem (kits_scanner.v_kits_names[i], i + 1);
     }


  int yoffs = 0;
  int xoffs = XFILLER;


  cmb_drumkit_selector.setTextWhenNothingSelected ("CLICK HERE TO SELECT THE DRUMKIT");
  cmb_drumkit_selector.setTextWhenNoChoicesAvailable ("NO DRUMKITS FOUND");
  cmb_drumkit_selector.setScrollWheelEnabled (true);

  addAndMakeVisible (cmb_drumkit_selector);


   //cmb_drumkit_selector.onChange = [this] { kit_changed(); };
  cmb_drumkit_selector.addListener (this);
  cmb_drumkit_selector.setTopLeftPosition (xoffs, 0);

  yoffs += 48;
/*
   addAndMakeVisible (bt_test);
   bt_test.setButtonText ("Test");
   bt_test.addListener (this);
*/



  drumlines_container.setSize (drumlines[0].getWidth() + XFILLER, drumlines[0].getHeight() * 36);

  for (size_t i = 0; i < 36; i++)
      {
       drumlines[i].setTopLeftPosition (0, i * drumlines[i].getHeight());
       drumlines_container.addAndMakeVisible (drumlines[i]);
       drumlines[i].attach_params (this, i);
      }


  load_kit (audioProcessor.drumkit_path);

  addAndMakeVisible (drumlines_group);

  drumlines_viewer.setViewedComponent (&drumlines_container, false);
  drumlines_viewer.setScrollBarsShown	(true, false);
  drumlines_viewer.setSize (drumlines_container.getWidth() + (XFILLER * 5), 480);
  drumlines_viewer.setScrollBarThickness (24);


  drumlines_group.setTopLeftPosition (xoffs, yoffs);
  drumlines_group.setSize (drumlines_viewer.getWidth() + (XFILLER * 3), 480 + (YFILLER * 2));

  xoffs += XFILLER;
  yoffs += YFILLER;

  drumlines_viewer.setTopLeftPosition (xoffs, yoffs);

  cmb_drumkit_selector.setSize (drumlines_group.getWidth(), 48);


  addAndMakeVisible (drumlines_viewer);


  //KIT INFO

  addAndMakeVisible (gr_kitinfo);
  gr_kitinfo.setSize (332, 264);
  gr_kitinfo.setTopLeftPosition (drumlines_viewer.getX() + drumlines_viewer.getWidth() + (XFILLER * 3), 0);


  addAndMakeVisible (l_kitinfo);
  l_kitinfo.setFont (f_kitname_font);
  l_kitinfo.setTopLeftPosition (gr_kitinfo.getX() + XFILLER, gr_kitinfo.getY());
  l_kitinfo.setSize (300, 48);


  addAndMakeVisible (kit_image);
  kit_image.setSize (300, 200);
  kit_image.setCentrePosition ((gr_kitinfo.getX() + gr_kitinfo.getWidth() / 2), (gr_kitinfo.getY() + gr_kitinfo.getHeight() / 2));



  // PAN MODE

  addAndMakeVisible (l_pan_mode);
  l_pan_mode.setTopLeftPosition (drumlines_viewer.getWidth() + XFILLER, 450);
  l_pan_mode.setSize (100, 48);

  addAndMakeVisible (cmb_pan_mode);
      //  cmb_pan_mode.onChange = [this] { panner_modeMenuChanged(); };
  cmb_pan_mode.setSize (180, 48);
  cmb_pan_mode.setTopLeftPosition (l_pan_mode.getX() + l_pan_mode.getWidth() + XFILLER, l_pan_mode.getY());

  cmb_pan_mode.addItem ("sin/cos panner, law: -3 dB", 1);
  cmb_pan_mode.addItem ("square root panner, law: -3 dB", 2);
  cmb_pan_mode.addItem ("linear panner, law: 0 dB", 3);
  cmb_pan_mode.addItem ("linear panner, law: -6 dB", 4);

  att_pan_mode.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (valueTreeState, "panner_mode", cmb_pan_mode));



  //BASE NOTE
  addAndMakeVisible (l_base_note);
  l_base_note.setSize (120, 48);
  l_base_note.setTopLeftPosition (gr_kitinfo.getX() + XFILLER, gr_kitinfo.getY() + gr_kitinfo.getHeight() + YFILLER);

  addAndMakeVisible (sl_base_note);
  sl_base_note.setSliderStyle (juce::Slider::SliderStyle::IncDecButtons);
  sl_base_note.setTopLeftPosition (l_base_note.getX() + l_base_note.getWidth() + XFILLER, l_base_note.getY());
  sl_base_note.setSize (120, 48);
  sl_base_note.setRange (0, 127, 1.0);
  sl_base_note.setValue (audioProcessor.int_base_note_number);


  sl_base_note.addListener (this);

//  sl_base_note.setNumDecimalPlacesToDisplay (0);
  //sl_base_note.setValue (*audioProcessor.base_note_number);


//   sl_base_note.onChange = [this] { panner_modeMenuChanged(); };

  //std::cout << "1" << std::endl;
 // att_base_note.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (valueTreeState, "base_note_number", sl_base_note));

  //       std::cout << "2" << std::endl;

   setSize (WINDOW_WIDTH, WINDOW_HEIGHT);

//bt_test.setBounds (drumlines[0].getWidth() + 50, 400, 50, 50);
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

      //    audioProcessor.val_panner_mode .setValue (4);

    // *audioProcessor.panner_mode = 1.0f;
     //*audioProcessor.panner_mode = 4;
   //  std::cout << "*audioProcessor.panner_mode: "  << *audioProcessor.panner_mode << std::endl;

    //         load_kit (audioProcessor.drumkit_path);

  //   audioProcessor.drumkit->print_stats();



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


void CAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
    {
        if (slider == &sl_base_note)
           {
             std::cout << "sl_base_note.getValue()" << std::endl;
             audioProcessor.int_base_note_number = sl_base_note.getValue();


              std::cout << "audioProcessor.int_base_note_number: " << audioProcessor.int_base_note_number << std::endl;

            // juce::AudioProcessorParameterWithID* pParam = audioProcessor.parameters.getParameter ( "base_note_number"  );
            // pParam->setValueNotifyingHost( sl_base_note.getValue() );
             //durationSlider.setValue (1.0 / c, juce::dontSendNotification);
            //juce::Value v = audioProcessor.parameters.getParameterAsValue ("base_note_number");
             //juce::RangedAudioParameter *p = audioProcessor.parameters.getParameter("base_note_number");
             //p->setValue (sl_base_note.getValue());
           //v.setValue (sl_base_note.getValue());
           // *audioProcessor.base_note_number = sl_base_note.getValue();
          }


    }