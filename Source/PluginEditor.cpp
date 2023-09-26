#include "PluginProcessor.h"
#include "PluginEditor.h"


#include "kits.h"
#include "utl.h"
#include "dsp.h"

#define XFILLER 4
#define YFILLER 16

#define WINDOW_HEIGHT 580
#define WINDOW_WIDTH 790


/*
CFx::CFx(): juce::TopLevelWindow::TopLevelWindow ("FX", false)
{
  addAndMakeVisible (l_test);
  l_test.setSize (160, 32);
  l_test.setText ("LABEL TEST", juce::dontSendNotification);

  setSize (640, 480);


}
*/



CFx::CFx()
{
  int xoffs = XFILLER * 2;
  int yoffs = YFILLER * 2;

  //LP ///////////////////////////////

  addChildComponent (gr_group_lp);
  gr_group_lp.setText ("LP");

  bt_lp.setButtonText ("ON");
  addAndMakeVisible (bt_lp);
  bt_lp.setTopLeftPosition (xoffs, yoffs);
  bt_lp.setSize (48, 32);

  xoffs += bt_lp.getWidth();
  xoffs += XFILLER;

  addAndMakeVisible (sl_lp_cutoff);

  sl_lp_cutoff.setTopLeftPosition (xoffs, yoffs);
  sl_lp_cutoff.setSize (68, 32);

  sl_lp_cutoff.setSliderStyle (juce::Slider::LinearHorizontal);
  sl_lp_cutoff.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);

  sl_lp_cutoff.setTooltip ("Cutoff");

  xoffs += sl_lp_cutoff.getWidth();
  xoffs += XFILLER;


  addAndMakeVisible (sl_lp_reso);
  sl_lp_reso.setTopLeftPosition (xoffs, yoffs);
  sl_lp_reso.setSize (68, 32);

  sl_lp_reso.setSliderStyle (juce::Slider::LinearHorizontal);

  sl_lp_reso.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
  sl_lp_reso.setTooltip ("Reso");

  xoffs += sl_lp_reso.getWidth();
  xoffs += XFILLER;


  int x = XFILLER;
  int y = YFILLER;

  gr_group_lp.setTopLeftPosition (x, y);
  gr_group_lp.setSize (xoffs, yoffs + sl_lp_reso.getHeight());

  gr_group_lp.setVisible (true);


  //HP ///////////////////////////////


  xoffs = XFILLER * 2;
  yoffs = gr_group_lp.getBottom() + YFILLER * 2;

  addChildComponent (gr_group_hp);
  gr_group_hp.setText ("HP");

  bt_hp.setButtonText ("ON");
  addAndMakeVisible (bt_hp);
  bt_hp.setTopLeftPosition (xoffs, yoffs);
  bt_hp.setSize (48, 32);

  xoffs += bt_hp.getWidth();
  xoffs += XFILLER;

  addAndMakeVisible (sl_hp_cutoff);

  sl_hp_cutoff.setTopLeftPosition (xoffs, yoffs);
  sl_hp_cutoff.setSize (68, 32);

  sl_hp_cutoff.setSliderStyle (juce::Slider::LinearHorizontal);
  sl_hp_cutoff.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);

  sl_hp_cutoff.setTooltip ("Cutoff");

  xoffs += sl_hp_cutoff.getWidth();
  xoffs += XFILLER;


  addAndMakeVisible (sl_hp_reso);
  sl_hp_reso.setTopLeftPosition (xoffs, yoffs);
  sl_hp_reso.setSize (68, 32);
  sl_hp_reso.setSliderStyle (juce::Slider::LinearHorizontal);

  sl_hp_reso.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
  sl_hp_reso.setTooltip ("Reso");

  xoffs += sl_hp_reso.getWidth();
  xoffs += XFILLER;


  x = XFILLER;
  y = gr_group_lp.getBottom() + YFILLER;

  gr_group_hp.setTopLeftPosition (x, y);
  gr_group_hp.setSize (xoffs, sl_hp_reso.getHeight() + YFILLER * 2);

  gr_group_hp.setVisible (true);



  //setSize (gr_group_lp.getRight() + XFILLER, gr_group_hp.getHeight() + gr_group_lp.getHeight() + YFILLER);

  setSize (gr_group_lp.getRight() + XFILLER, gr_group_hp.getHeight() + gr_group_lp.getHeight() + YFILLER * 2);


//  setSize (640, 480);


}



CFx::~CFx()
{


  att_lp =  nullptr ;
  att_lp_cutoff =  nullptr ;
  att_lp_reso =  nullptr ;

  att_hp =  nullptr ;
  att_hp_cutoff =  nullptr ;
  att_hp_reso =  nullptr ;

}


void CFx::resized()
{

}



void CLed::paint(Graphics& g)
{
  if (is_on)
     g.fillAll(cl_on.withLightness(velocity));
  else
      g.fillAll(cl_off);
}


CDrumLine::CDrumLine()
{
  //INIT FX
/*
  int xoffs = XFILLER * 2;

  bt_lp.setButtonText ("LP");
  //fx.addAndMakeVisible (bt_lp);

  addChildComponent (bt_lp);


  bt_lp.setTopLeftPosition (xoffs, YFILLER);
  bt_lp.setSize (48, 32);

  xoffs += bt_lp.getWidth();
  xoffs += XFILLER;


  //fx.addAndMakeVisible (sl_lp_cutoff);
  addChildComponent(sl_lp_cutoff);

  sl_lp_cutoff.setTopLeftPosition (xoffs, YFILLER);
  sl_lp_cutoff.setSize (68, 32);
  sl_lp_cutoff.setRange (0.1f, 0.99f, 0.001f);

  sl_lp_cutoff.setSliderStyle (juce::Slider::LinearHorizontal);
  sl_lp_cutoff.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
  sl_lp_cutoff.setTooltip ("Cutoff");

  xoffs += sl_lp_cutoff.getWidth();
  xoffs += XFILLER;


  //fx.addAndMakeVisible (sl_lp_reso);
  addChildComponent(sl_lp_cutoff);

  sl_lp_reso.setTopLeftPosition (xoffs, YFILLER);
  sl_lp_reso.setSize (68, 32);
  sl_lp_reso.setRange (0.0f, 1.0f, 0.001f);

  sl_lp_reso.setSliderStyle (juce::Slider::LinearHorizontal);

  sl_lp_reso.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
  sl_lp_reso.setTooltip ("Reso");


  fx.setSize (480, 200);
*/
  addChildComponent (fx);



  //
  addChildComponent (gr_group);

  int xoffs = XFILLER * 2;

  addAndMakeVisible (led);
  led.setTopLeftPosition (xoffs, YFILLER);
  led.setSize (16, 32);

  xoffs += led.getWidth();
  xoffs += XFILLER;


  addAndMakeVisible (label);
  label.setTopLeftPosition (xoffs, YFILLER);
  label.setSize (160, 32);

  label.setColour (juce::Label::textColourId, juce::Colours::black);
  label.setColour (juce::Label::backgroundColourId, juce::Colour (180, 209, 220));

//  label.setColour (juce::Label::backgroundColourId, juce::Colour (255, 222, 89));
  label.setFont (f_samplename_font);

  label.setText ("EMPTY CELL", juce::dontSendNotification);

  xoffs += label.getWidth();
  xoffs += XFILLER;


  wnd_fx = new juce::ResizableWindow ("FX", false);
 wnd_fx->setContentNonOwned (&fx, true);
  wnd_fx->setUsingNativeTitleBar (false);

  addAndMakeVisible (bt_fx);

  bt_fx.setButtonText ("FX");

  bt_fx.setTopLeftPosition (xoffs, YFILLER);
  bt_fx.setSize (42, 32);


   //wnd_fx->setTopLeftPosition (bt_fx.getScreenX(), bt_fx.getScreenY() + bt_fx.getHeight());



   bt_fx.onClick = [this] {
                          std::cout << "CLICK" << std::endl;

      /*                    if (fx.isVisible())
                             {
                              fx.setVisible (false);
                              fx.toBack();
                            }
                          else
                               {
                                fx.setVisible (true);
                                fx.toFront(true);
                               }

*/
//                          std::cout << "bt_fx.getScreenX() " << bt_fx.getScreenX() << std::endl;


                           if (wnd_fx->isOnDesktop())
                               wnd_fx->removeFromDesktop();
                           else
                               wnd_fx->addToDesktop();

                         // wnd_fx.addToDesktop (juce::ComponentPeer::StyleFlags::windowHasCloseButton |
                                         //      juce::ComponentPeer::StyleFlags::windowHasTitleBar | juce::ComponentPeer::StyleFlags::windowIsTemporary);

                          wnd_fx->setVisible (! wnd_fx->isVisible());

                         // wnd_fx->setTopLeftPosition (1, 1);
                            wnd_fx->setTopLeftPosition (bt_fx.getScreenX() + bt_fx.getWidth(), bt_fx.getScreenY() + bt_fx.getHeight());



                         };



  xoffs += bt_fx.getWidth();
  xoffs += XFILLER;


  addAndMakeVisible (sl_pan);
  sl_pan.setTopLeftPosition (xoffs, YFILLER);
  sl_pan.setSize (68, 32);
  sl_pan.setRange (0.0f, 1.0f, 0.01f);

  sl_pan.setSliderStyle (juce::Slider::LinearHorizontal);

  sl_pan.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
  sl_pan.setTooltip ("Pan");

  xoffs += sl_pan.getWidth();
  xoffs += XFILLER;

  addAndMakeVisible (sl_vol);

  sl_vol.setTopLeftPosition (xoffs, YFILLER);
  sl_vol.setSize (104, 32);

  sl_vol.setSliderStyle (juce::Slider::LinearHorizontal);
  sl_vol.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);

  sl_vol.setRange (-96, 12, 1);
  sl_vol.setSkewFactor (4);

    //sl_gain.addListener (this);

  sl_vol.setTooltip ("Volume");

  xoffs += sl_vol.getWidth();
  xoffs += XFILLER;

  bt_mute.setButtonText ("M");
  addAndMakeVisible (bt_mute);
  bt_mute.setTopLeftPosition (xoffs, YFILLER);
  bt_mute.setSize (48, 32);


  xoffs += bt_mute.getWidth();
  xoffs += XFILLER;


  gr_group.setVisible (true);
  gr_group.setSize (xoffs, 32 + YFILLER + YFILLER);

  setSize (xoffs + XFILLER, 32 + YFILLER + YFILLER);

   //testlabel.setSize (32, 32);
  //testlabel.setText ("testlabel", juce::dontSendNotification);
  //testlabel.setVisible (true);

  //dsp_box = new juce::CallOutBox (testlabel, label.getBounds(), this);


}


CDrumLine::~CDrumLine()
{
//  delete dsp_box;

 /*  att_lp =  nullptr ;
  att_lp_cutoff =  nullptr ;
  att_lp_reso =  nullptr ;
*/

 delete wnd_fx;
}


void CDrumLine::paint (juce::Graphics& g)
{
  // g.fillAll (juce::Colour (0xff323e44));
}


void CDrumLine::resized()
{
}


void CDrumLine::attach_params (CAudioProcessorEditor *ed, int cellno)
{
  cell_number = cellno;

  std::string param_name = "vol" + std::to_string (cell_number);
  att_vol.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_vol));

  param_name = "pan" + std::to_string (cell_number);
  att_pan.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, sl_pan));

  param_name = "mute" + std::to_string (cell_number);
  att_mute.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (ed->valueTreeState, param_name, bt_mute));

  param_name = "lp" + std::to_string (cell_number);
  fx.att_lp.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (ed->valueTreeState, param_name, fx.bt_lp));

  param_name = "lp_cutoff" + std::to_string (cell_number);
  fx.att_lp_cutoff.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, fx.sl_lp_cutoff));

  param_name = "lp_reso" + std::to_string (cell_number);
  fx.att_lp_reso.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, fx.sl_lp_reso));

  param_name = "hp" + std::to_string (cell_number);
  fx.att_hp.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (ed->valueTreeState, param_name, fx.bt_hp));

  param_name = "hp_cutoff" + std::to_string (cell_number);
  fx.att_hp_cutoff.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, fx.sl_hp_cutoff));

  param_name = "hp_reso" + std::to_string (cell_number);
  fx.att_hp_reso.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (ed->valueTreeState, param_name, fx.sl_hp_reso));


}


void CDrumLine::set_name (const std::string &n)
{
  juce::String s (juce::CharPointer_UTF8((n.c_str())));
  label.setText (s, juce::dontSendNotification);
}


void CAudioProcessorEditor::load_kit (const std::string &kitpath)
{
  //make all drum labels empty

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

/*
void CAudioProcessorEditor::panner_modeMenuChanged()
{
       std::cout << "cmb_panner_mode.getSelectedId(): " <<  cmb_pan_mode.getSelectedId() << std::endl;
       std::cout << "cmb_panner_mode.getSelectedItemIndex(): " <<  cmb_pan_mode.getSelectedItemIndex() << std::endl;
}
*/


CAudioProcessorEditor::CAudioProcessorEditor (CAudioProcessor& parent, juce::AudioProcessorValueTreeState& vts)
                                             : AudioProcessorEditor (&parent),
                                               audioProcessor (parent),
                                               valueTreeState (vts)
{
//std::cout << "CAudioProcessorEditor::CAudioProcessorEditor - 1" << std::endl;


  kits_scanner.scan();

  for (size_t i = 0; i < kits_scanner.v_kits_names.size(); i++)
     {
      cmb_drumkit_selector.addItem (kits_scanner.v_kits_names[i], i + 1);
     }

//std::cout << "CAudioProcessorEditor::CAudioProcessorEditor - 2" << std::endl;

 // getLookAndFeel().setColour (juce::LookAndFeel::backgroundColourId, juce::Colours::red);



  int yoffs = 0;
  int xoffs = XFILLER;

  cmb_drumkit_selector.setTextWhenNothingSelected ("CLICK HERE TO SELECT THE DRUMKIT");
  cmb_drumkit_selector.setTextWhenNoChoicesAvailable ("NO DRUMKITS FOUND");
  cmb_drumkit_selector.setScrollWheelEnabled (true);

  cmb_drumkit_selector.setTooltip ("Select from installed drum kits");


  cmb_drumkit_selector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (53, 84, 100));
  cmb_drumkit_selector.setColour (juce::ComboBox::textColourId, juce::Colour (255, 254, 255));

  addAndMakeVisible (cmb_drumkit_selector);


   //cmb_drumkit_selector.onChange = [this] { kit_changed(); };
  cmb_drumkit_selector.addListener (this);
  cmb_drumkit_selector.setTopLeftPosition (xoffs, 8);

  yoffs += 52;

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


  addAndMakeVisible (gr_options);
  gr_options.setTopLeftPosition (gr_kitinfo.getX(), gr_kitinfo.getY() + gr_kitinfo.getHeight());



  //BASE NOTE

  l_base_note.setColour (juce::Slider::thumbColourId, juce::Colour (53, 84, 100));
  l_base_note.setColour (juce::Slider::trackColourId, juce::Colour (255, 254, 255));

  addAndMakeVisible (l_base_note);
  l_base_note.setSize (120, 48);
  l_base_note.setTopLeftPosition (gr_options.getX() + XFILLER, gr_kitinfo.getY() + gr_kitinfo.getHeight() + YFILLER);

  addAndMakeVisible (sl_base_note);
  sl_base_note.setSliderStyle (juce::Slider::SliderStyle::IncDecButtons);
  sl_base_note.setTopLeftPosition (l_base_note.getX() + l_base_note.getWidth() + XFILLER, l_base_note.getY());
  sl_base_note.setSize (120, 48);
  sl_base_note.setRange (0, 127, 1.0);
  sl_base_note.setValue (audioProcessor.base_note_number, dontSendNotification );
  sl_base_note.addListener (this);

  sl_base_note.setTooltip ("Number of MIDI note from which\n we start to map instruments, \n default 36");


  // PAN MODE

  addAndMakeVisible (l_pan_mode);
  l_pan_mode.setTopLeftPosition (l_base_note.getX(), sl_base_note.getBottom() + YFILLER);
  l_pan_mode.setSize (100, 48);


  cmb_pan_mode.setColour (juce::ComboBox::backgroundColourId, juce::Colour (53, 84, 100));
  cmb_pan_mode.setColour (juce::ComboBox::textColourId, juce::Colour (255, 254, 255));


  addAndMakeVisible (cmb_pan_mode);
      //  cmb_pan_mode.onChange = [this] { panner_modeMenuChanged(); };
  cmb_pan_mode.setSize (180 + XFILLER, 48);
  cmb_pan_mode.setTopLeftPosition (sl_base_note.getX(), sl_base_note.getBottom() + YFILLER);

  cmb_pan_mode.addItem ("sin/cos panner, law: -3 dB", PANMODE01);
  cmb_pan_mode.addItem ("square root panner, law: -3 dB", PANMODE02);
  cmb_pan_mode.addItem ("linear panner, law: 0 dB", PANMODE03);
  cmb_pan_mode.addItem ("linear panner, law: -6 dB", PANMODE04);
  cmb_pan_mode.addItem ("power panner, law: -4.5 dB", PANMODE05);
  cmb_pan_mode.addItem ("power panner, law: -1.5 dB", PANMODE06);
  cmb_pan_mode.addItem ("equal power panner, law: -1.3 dB", PANMODE07);

  att_pan_mode.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (valueTreeState, "panner_mode", cmb_pan_mode));




  addAndMakeVisible (bt_ignore_midi_velocity);
  bt_ignore_midi_velocity.setButtonText ("Ignore MIDI velocity");

  bt_ignore_midi_velocity.setTooltip ("If turned on, play first layer\n of multi-layered samples,\n and with the maximun velocity");


  bt_ignore_midi_velocity.setSize (180 + XFILLER, 48);
  bt_ignore_midi_velocity.setTopLeftPosition (l_pan_mode.getX(), cmb_pan_mode.getY() + cmb_pan_mode.getHeight() + YFILLER);

  att_ignore_midi_velocity.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (valueTreeState, "ignore_midi_velocity", bt_ignore_midi_velocity));


  gr_options.setSize (gr_kitinfo.getWidth(), l_pan_mode.getHeight() + YFILLER + cmb_pan_mode.getHeight() + bt_ignore_midi_velocity.getHeight() +
  YFILLER * 2);





  addAndMakeVisible (l_plugin_name);
  l_plugin_name.setSize (180 + XFILLER, 48);
  l_plugin_name.setTopLeftPosition (gr_options.getX(), gr_options.getY() + gr_options.getHeight() + YFILLER);
  l_plugin_name.setText (juce::CharPointer_UTF8("DRUMЛАБУХ"), juce::NotificationType::dontSendNotification);
  l_plugin_name.setFont	(f_logo);


  addAndMakeVisible (hl_homepage);
  hl_homepage.setSize (280 + XFILLER, 24);
  hl_homepage.setTopLeftPosition (l_plugin_name.getX(), l_plugin_name.getY() + l_plugin_name.getHeight() + YFILLER);
  hl_homepage.setURL (juce::URL("https://psemiletov.github.io/drumlabooh"));
  hl_homepage.setButtonText ("psemiletov.github.io/drumlabooh");
  hl_homepage.setFont (f_sitelink,false,  juce::Justification::left);

//  setSize (WINDOW_WIDTH, WINDOW_HEIGHT);

    setSize (gr_options.getX() + gr_options.getWidth() + XFILLER * 2, drumlines_viewer.getBottom() + YFILLER * 2);

    tmr_leds.uplink = this;
    tmr_leds.startTimer (1000 / 15); //15 FPS


//  sl_base_note.setNumDecimalPlacesToDisplay (0);
  //sl_base_note.setValue (*audioProcessor.base_note_number);


//   sl_base_note.onChange = [this] { panner_modeMenuChanged(); };

  //std::cout << "1" << std::endl;
 // att_base_note.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (valueTreeState, "base_note_number", sl_base_note));

  //       std::cout << "2" << std::endl;


   /*
   addAndMakeVisible (bt_test);
   bt_test.setButtonText ("Test");
   bt_test.addListener (this);
*/

//bt_test.setBounds (drumlines[0].getWidth() + 50, 400, 50, 50);
}


CAudioProcessorEditor::~CAudioProcessorEditor()
{
}

//==============================================================================
void CAudioProcessorEditor::paint (juce::Graphics& g)
{

//        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
  g.fillAll (juce::Colour::fromRGB(103, 97, 114)	);
}

void CAudioProcessorEditor::resized()
{

}


void CAudioProcessorEditor::buttonClicked (juce::Button* button)
{
 /* if (button == &bt_test)
    {
     std::cout << "NewProjectAudioProcessorEditor::buttonClicked " << std::endl;
  }*/
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

      tmr_leds.stopTimer();

      audioProcessor.load_kit (full);

      //update GUI
      load_kit (full);
      tmr_leds.startTimer (1000 / 15); //15 FPS

    }


}


void CAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
    {
      std::cout << "CAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)"  << std::endl;

        if (slider == &sl_base_note)
           {
             std::cout << "sl_base_note.getValue()" << std::endl;
             audioProcessor.base_note_number = sl_base_note.getValue();


              std::cout << "audioProcessor.int_base_note_number: " << audioProcessor.base_note_number << std::endl;

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

/*
void CToggleButton.paintButton 	( 	Graphics &  	g,
		bool  	shouldDrawButtonAsHighlighted,
		bool  	shouldDrawButtonAsDown
	)
{


}
*/


void CTimer::hiResTimerCallback()
{
 // std::cout << "CTimer::timerCallback -1" << std::endl;


   if (! uplink)
       return;

   if (! uplink->isVisible())
       return;



//  std::cout << "CTimer::timerCallback - 2" << std::endl;

   if (! uplink->audioProcessor.drumkit)
      return;

 // std::cout << "CTimer::timerCallback - 3" << std::endl;


   for (int i = 0; i < uplink->audioProcessor.drumkit->v_samples.size(); i++)
        {
         bool actv = uplink->audioProcessor.drumkit->v_samples[i]->active;
         uplink->drumlines[i].led.is_on = actv;
         uplink->drumlines[i].led.velocity = uplink->audioProcessor.drumkit->v_samples[i]->velocity;
         uplink->drumlines[i].led.repaint();
        }

}
