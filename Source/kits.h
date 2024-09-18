/*
written at 2023-24 by Peter Semiletov
this code is the public domain
 */

#ifndef KITS_H
#define KITS_H

#include <vector>
#include <map>
#include <random>

#include <stdint.h>
#include <string.h>

#include <JuceHeader.h>

//JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wno-switch-enum -Wzero-as-null-pointer-constant")
#include "pugixml.hpp"
//JUCE_END_IGNORE_WARNINGS_GCC_LIKE

#define LAYER_INDEX_MODE_VEL 0
#define LAYER_INDEX_MODE_RND 1
#define LAYER_INDEX_MODE_ROBIN 2
#define LAYER_INDEX_MODE_NOVELOCITY 3

#define KIT_TYPE_DRUMLABOOH 0
#define KIT_TYPE_HYDROGEN 1
#define KIT_TYPE_SFZ 2
#define KIT_TYPE_QDRUMLABOOH 3

#define MAX_SAMPLES 36


class CDrumSample;


class CDrumLayer
{
public:

  int session_samplerate; //uplink's (session) samplerate

  CDrumSample *drum_sample; //uplink sample class that holds this sample layer

  //for layer's velocity range
  float min;
  float max;

  std::string file_name; //name of the loaded file, full path
  
 // int channels; //channels at this layer
 //channels variable has been removed as we have mono only

  int samplerate; //samplerate of this layer
  int length_in_samples;// i.e frames, the amount of samples per channel
  int sample_offset; //to play inc until < length_in_samples

  juce::AudioBuffer <float> *audio_buffer; //holds audio data
  //const float *channel_data [2]; // pointers to channels of audio_buffer, 2 is legacy from stereo support
  //now we use just first channel
  const float *channel_data; // pointer to the first channel of audio_buffer
  
  CDrumLayer (CDrumSample *s);
  ~CDrumLayer();

  void load (const std::string &fname); //loads the sample, sets internal info, data, file_name
  void print();

  juce::AudioBuffer<float> * load_whole_sample (const std::string &fname); //called from load_whole_sample_resampled
  juce::AudioBuffer<float> * load_whole_sample_resampled (const std::string &fname, int sess_samplerate); //main function to load the sample layer

};


class CDrumSample
{
public:

  std::string name; //sample name, for example "Bass Drum"
  int id; //Hydrogen's. For what? 
  int current_layer;
//  int midiOutNote;
  int session_samplerate; //session srate, taken from the upper level object

  bool active; //is sample triggered to play? 
  
  bool has_mapping; //is the MIDI note defined for that instrument at drumkit file?

  int mapped_note; //the assigned MIDI note, if defined
  
  //float random_number; //changes as sample triggers
  //float noise_level;
  
  int layer_index_mode; //0 - velocity, 1 - rnd, 2 - round robin
  
  size_t robin_counter; 
  
  float velocity;

  bool hihat_open;
  bool hihat_close;

  std::vector <CDrumLayer*> v_layers; //container for the actual sounds 

  CDrumSample (int sample_rate);
  ~CDrumSample();

  size_t map_velo_to_layer_number (float velo);

  void add_layer();

  void print();
  void print_stats();

  void untrigger_sample();
  void trigger_sample (float vel);

};


class CDrumKit
{
public:

  bool scan_mode; //if false, we do not load kit's samples

  std::string kit_name; //parsed from XML or evaluated in other way
  std::string kit_filename; //full path to the kit xml, txt or sfz file
  std::string kit_dir; //full path to the kit directory
  
  int kit_type;  //SFZ, Hydrogen of Drumlabooh?
  
  bool loaded; //is kit fully loaded?
  
  std::string image_fname; //full path to the kit's picture

  int samplerate; //session srate

  bool layers_supported;
  bool has_mapping;

  size_t total_samples_size();
  
  //std::vector <CDrumSample*> v_samples;
  
  CDrumSample* a_samples [36];
  
  std::map <int, CDrumSample*> map_samples; //just a map, actual samples are at v_samples
  
  std::vector <std::string> v_hat_open_signatures;
  std::vector <std::string> v_hat_close_signatures;

  CDrumSample* add_sample (size_t index);
  CDrumSample* temp_sample;
  
  int sample_counter;
  
  void load (const std::string &fname, int sample_rate);
  void load_txt (const std::string &fname);
  void load_qtxt (const std::string &fname);
  void load_sfz (const std::string & fname);
  
  CDrumSample* load_sample_to_index (size_t index, const std::string &fname, int sample_rate);
  void remove_sample_at_index (size_t index);
  

  CDrumKit();
  ~CDrumKit();

  void print();
  void print_stats();
  
  void adapt(); //for Adapt function
  
  void save_qkit();
  
};


class CDrumKitsScanner
{
public:

  std::vector <std::string> v_kits_dirs;
  std::vector <std::string> v_kits_names;
  //std::vector <CDrumKit*> v_scanned_kits;
  std::map <std::string, std::string> map_kits; //name = full path

  CDrumKitsScanner();
  ~CDrumKitsScanner();

  void scan_full(); //old scan, uses semi-loading for each drumkit
  void scan(); //new, does not load kits, just extract names in some ways 

  void print();
};


class CHydrogenXMLWalker: public pugi::xml_tree_walker
{
public:

  CDrumKit *kit;

  bool is_drumkit_info;
  bool is_instrument;
  bool is_layer;
  bool drumkit_info_passed;
  bool drumkitComponent_passed;

  CHydrogenXMLWalker (CDrumKit *hkit);
  bool for_each (pugi::xml_node& node);
};


void rnd_init();


#endif
