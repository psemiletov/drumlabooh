/*
written at 2023-24 by Peter Semiletov
this code is the public domain
 */


#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;


class CAudioProcessorEditor;
class CDrumCell;

class CDrumkitsListBoxModel: public ListBoxModel
{
public:
  
  CAudioProcessorEditor *editor;

  juce::Font item_font {14.0f, juce::Font::bold};
  
  std::vector <std::string> items;
  std::vector <int> indexes;
  
  CDrumkitsListBoxModel();
  ~CDrumkitsListBoxModel();
  int getNumRows();
  void paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected);
  void selectedRowsChanged (int lastRowSelected) override;
};


class CTimer: public juce::HighResolutionTimer
{
public:

  CTimer() {};
  CAudioProcessorEditor *uplink = 0;
  void hiResTimerCallback() override;
};


class CLed: public juce::Component
{
public:

  juce::Colour cl_on = juce::Colours::orange;
  juce::Colour cl_off = juce::Colours::grey;
  bool is_on = false;
  float velocity = 1.0f;

  void paint (Graphics &g);
};


class CDocumentWindow: public juce::DocumentWindow
{
public:

  CDocumentWindow (const String &name, Colour backgroundColour, int requiredButtons, bool addToDesktop=true);
  void closeButtonPressed() override;
  bool keyPressed (const KeyPress &key) override;
};


class CFx: public juce::Component
{
public:

  std::unique_ptr <juce::AudioProcessorValueTreeState::ButtonAttachment> att_lp;
  std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> att_lp_cutoff;
  std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> att_lp_reso;

  std::unique_ptr <juce::AudioProcessorValueTreeState::ButtonAttachment> att_hp;
  std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> att_hp_cutoff;
  std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> att_hp_reso;

  std::unique_ptr <juce::AudioProcessorValueTreeState::ButtonAttachment> att_analog;
  std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> att_analog_amount;

  juce::GroupComponent gr_group_lp;
  juce::GroupComponent gr_group_hp;
  juce::GroupComponent gr_group_analog;

  juce::ToggleButton bt_lp;
  juce::Slider sl_lp_cutoff;
  juce::Slider sl_lp_reso;

  juce::ToggleButton bt_hp;
  juce::Slider sl_hp_cutoff;
  juce::Slider sl_hp_reso;

  juce::ToggleButton bt_analog;
  juce::Slider sl_analog_amount;


  CFx();
  ~CFx() override;
};


class CCellLabel: public juce::Label,
                  public juce::FileDragAndDropTarget
{
public:
  
  CDrumCell *cell; //uplink
  //CAudioProcessorEditor *editor = 0; //uplink
  
  bool isInterestedInFileDrag (const StringArray &files) override;
  void filesDropped (const StringArray &files, int x, int y) override;
};


#ifndef MULTICHANNEL


class CDrumCell: public juce::Component/*,
                 public juce::Slider::Listener*/
{
public:

  juce::Font f_samplename_font {16.0f, juce::Font::bold};

  size_t cell_number;
  
  CAudioProcessorEditor *editor = 0; //uplink
    
  juce::GroupComponent gr_group;
  juce::Slider sl_pan;
  juce::Slider sl_vol;

  juce::ToggleButton bt_mute;
  CLed led;

  CDocumentWindow *wnd_fx;

  CFx fx;

  juce::TextButton bt_fx;
  //juce::TextButton bt_cell_plus;
//  juce::TextButton bt_cell_minus;
  
  juce::DrawableButton bt_cell_plus { 
        "bt_cell_plus", 
        juce::DrawableButton::ImageRaw
    };

  juce::DrawableButton bt_cell_minus { 
        "bt_cell_minus", 
        juce::DrawableButton::ImageRaw
    };

  juce::Button *bt_fx_close;

  CCellLabel cell_label;

//  Label cell_label;

  
  std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> att_vol;
  std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> att_pan;
  std::unique_ptr <juce::AudioProcessorValueTreeState::ButtonAttachment> att_mute;

 // int cell_number;

  CDrumCell();
  ~CDrumCell() override;

  void attach_params (CAudioProcessorEditor *ed, int cellno);
  void set_name (const std::string &n);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CDrumCell)
};


#else


class CDrumCell: public juce::Component/*,
                 public juce::Slider::Listener*/
{
public:

  juce::Font f_samplename_font {16.0f, juce::Font::bold};

  juce::GroupComponent gr_group;

  
  
  CAudioProcessorEditor *editor = 0; //uplink
  
  CLed led;

  CDocumentWindow *wnd_fx;

  CFx fx;


  CCellLabel cell_label;
//   Label cell_label;

  size_t cell_number;

  juce::TextButton bt_fx;
  juce::Button *bt_fx_close;
  //juce::TextButton bt_cell_plus;
  //juce::TextButton bt_cell_minus;

  juce::DrawableButton bt_cell_plus { 
        "bt_cell_plus", 
        juce::DrawableButton::ImageFitted 
    };

    
  juce::DrawableButton bt_cell_minus { 
        "bt_cell_minus", 
        juce::DrawableButton::ImageRaw
    };
  
  CDrumCell();
  ~CDrumCell() override;

  void attach_params (CAudioProcessorEditor *ed, int cellno);
  void set_name (const std::string &n);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CDrumCell)
};

#endif


class CAudioProcessorEditor: public juce::AudioProcessorEditor,
                             public juce::Button::Listener,
                             public juce::ComboBox::Listener,
                             public juce::Slider::Listener
{
public:

  CAudioProcessor &audioProcessor;

  //CDrumkitsListBoxScroller drumkits_listbox_lookandfeel;

  bool need_to_update_cells;
  
 // CDrumKitsScanner kits_scanner;
  CTimer tmr_leds;
  CDrumCell drumcells [MAX_SAMPLES];

  juce::AudioProcessorValueTreeState& valueTreeState;

  std::unique_ptr <juce::AudioProcessorValueTreeState::ButtonAttachment> att_ignore_midi_velocity;
  juce::ToggleButton bt_ignore_midi_velocity;

  CDrumkitsListBoxModel drumkits_model;
  ListBox drumkits_listbox;
  
  TextEditor log_area;

//////////////////////////////////
//OPTIONS CONTROLS  
  
#ifndef MULTICHANNEL

  std::unique_ptr <juce::AudioProcessorValueTreeState::ButtonAttachment> att_global_analog_on = nullptr;
  juce::ToggleButton bt_global_analog_on;

  std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> att_global_analog_amount = nullptr;
  juce::Slider sl_global_analog_amount;
  
  std::unique_ptr <juce::AudioProcessorValueTreeState::ComboBoxAttachment> att_pan_mode;
  juce::Label l_pan_mode { {}, "Pan mode" };
  juce::ComboBox cmb_pan_mode;

#endif

  std::unique_ptr <juce::AudioProcessorValueTreeState::ComboBoxAttachment> att_midimap_mode;
  juce::Label l_midimap_mode { {}, "MIDI map mode" };
  juce::ComboBox cmb_midimap_mode;
  
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_base_note;
  juce::Label l_base_note { {}, "Base MIDI note" };
  juce::Slider sl_base_note;

  juce::GroupComponent gr_options;


//END OF OPTIONS CONTROLS  

  juce::TextButton bt_file_open;
  std::unique_ptr <juce::FileChooser> dlg_fileopen;
  
  juce::TextButton bt_kit_adapt;
  juce::TextButton bt_kit_save;
    
  juce::TooltipWindow tooltip_window;

  juce::GroupComponent gr_kitinfo;
  juce::Label l_kit_name;

//TOP BAR  
  juce::GroupComponent gr_topbar;
  
  juce::Label l_plugin_name;
  juce::Label l_plugin_author;
  juce::HyperlinkButton hl_homepage;
  juce::Font f_sitelink {20.0f, juce::Font::bold};
  juce::Font f_logo {30.0f, juce::Font::bold};
//END OF TOPBAR
  
  juce::Font f_log {14.0f, juce::Font::bold};
  juce::Font f_kits {30.0f, juce::Font::bold};
  
  juce::Font f_kitname_font {20.0f, juce::Font::bold};
  juce::ImageComponent kit_image;

  //juce::Image kit_image_default;

  
  juce::GroupComponent drumcells_group;
  juce::Component drumcells_container;
  juce::Viewport drumcells_viewer;

  //juce::Label l_drumkit_name;
  juce::TextButton bt_drumkit_info;

  juce::GroupComponent gr_drumkits;
  juce::GroupComponent gr_kitbuttons;

 
//  juce::Label l_drumkit_selector { {}, "Drumkit selector" };
//  juce::ComboBox cmb_drumkit_selector;

  CAudioProcessorEditor (CAudioProcessor &parent, juce::AudioProcessorValueTreeState &vts);
  ~CAudioProcessorEditor() override;

  //void load_kit (const std::string &kitpath);
  
  void load_kit(); //call after processor's load_kit to update GUI
  void log (const std::string &s);
 
  void update_kits_list();
  
  void adapt();
  
  void adapt_drumlabooh();
  void adapt_qdrumlabooh();
  
  
  void save_quick_kit();
    
  void paint (juce::Graphics&) override;
  void resized() override;

  void kit_changed();
  void sliderValueChanged (juce::Slider* slider) override;
  void comboBoxChanged(juce::ComboBox* comboBox) override;
  void buttonClicked (juce::Button* button) override;


  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CAudioProcessorEditor)
};
