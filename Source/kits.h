/*
written at 2023-25 by Peter Semiletov
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

#include "utl.h"


#define LAYER_INDEX_MODE_VEL 0
#define LAYER_INDEX_MODE_RND 1
#define LAYER_INDEX_MODE_ROBIN 2
#define LAYER_INDEX_MODE_NOVELOCITY 3

//same as ALTDRUMLABOOH 
#define LAYER_INDEX_MODE_ALT 4



#define KIT_TYPE_DRUMLABOOH 0
#define KIT_TYPE_HYDROGEN 1
#define KIT_TYPE_SFZ 2
#define KIT_TYPE_QDRUMLABOOH 3
//#define KIT_TYPE_ALTDRUMLABOOH 4 //каждый слой - отдельный one shot; может быть как кит, а так и директория, которая действует как драмкит



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

  int umin; //uint
  int umax; //uint
  
  
  std::string file_name; //name of the loaded file, full path
  
 // int channels; //channels at this layer
 //channels variable has been removed as we have mono only

  int samplerate; //samplerate of this layer
  int length_in_samples;// i.e frames, the amount of samples per channel
  int sample_offset; //to play inc until < length_in_samples

  juce::AudioBuffer <float> *audio_buffer; //holds audio data
  //const float *channel_data [2]; // pointers to channels of audio_buffer, 2 is legacy from stereo support
  //now we use just first channel
  const float *channel_data; // pointer to the first (element 0) channel of audio_buffer
  
  CDrumLayer (CDrumSample *s);
  ~CDrumLayer();

  void load (const std::string &fname, int offset = 0); //loads the sample, sets internal info, data, file_name
  void print();
  
  juce::AudioBuffer<float> * load_whole_sample (const std::string &fname, int offset = 0); //called from load_whole_sample_resampled
  juce::AudioBuffer<float> * load_whole_sample_resampled (const std::string &fname, int sess_samplerate, int offset = 0); //main function to load the sample layer

};


class CDrumSample
{
public:

  std::string name; //sample name, for example "Bass Drum"
  int id; //Hydrogen's. For what? 
  
  int mute_group; //-1 no; 7777 hihat auto
  
  int current_layer; //текущий слой

  int session_samplerate; //session srate, taken from the upper level object

  bool active; //is sample triggered to play? 
  
  bool has_mapping; //is the MIDI note defined for that instrument at drumkit file?

  int mapped_note; //the assigned MIDI note, if defined
  
 
  int layer_index_mode; //0 - velocity, 1 - rnd, 2 - round robin

  FastDeterministicRNG rnd_generator;
 
  size_t robin_counter; 
  
  float velocity;

  std::vector <CDrumLayer*> v_layers; //container for the actual sounds 

  CDrumSample (int sample_rate);
  ~CDrumSample();

  size_t map_velo_to_layer_number (float velo);
  size_t map_uint_velo_to_layer_number (int velo);

  std::string get_name (bool by_current_layer = false);  
  
  void add_layer();

  void print();
  void print_stats();

  void sample_next();
  void sample_prev();
  
  void untrigger_sample (bool alt = false);
  void trigger_sample (float vel); //not vel actually! измени формулировки!
  void trigger_sample_uint (int vel, float velo);
  void trigger_sample_uint_by_index (int vel, float velo);

};


class CDrumKit
{
public:
   
  std::string kit_name; //parsed from XML or evaluated in other way
  std::string kit_filename; //full path to the kit xml, txt or sfz file
  std::string kit_dir; //full path to the kit directory
  
  int kit_type;  //SFZ, Hydrogen of Drumlabooh?
  
  bool is_dir_kit; //true if kit is just a directory
  bool mute_groups_auto; //true if not defined at the kit file
  bool loaded; //is kit fully loaded?
  
  std::string image_fname; //full path to the kit's picture

  int samplerate; //session srate

  bool layers_supported;
  bool has_mapping;

  size_t load_duration_msecs;
  std::string str_load_duration_msecs;
  
  size_t total_samples_size();
  
  //std::vector <CDrumSample*> v_samples;
  
  CDrumSample* a_samples [36];
  
  std::map <int, CDrumSample*> map_samples; //just a map, actual samples are at v_samples
  
  std::vector <std::string> v_auto_mute_signatures;

  CDrumSample* add_sample (size_t index);
  CDrumSample* temp_sample;
  
  int sample_counter;
  
  void load (const std::string &fname, int sample_rate);
  
  void load_hydrogen (const std::string &data);
  void load_txt (const std::string &tdata);
  void load_labooh_xml (const std::string &tdata);
  void load_directory (const std::string &path);

  void load_qtxt (const std::string &data);
  void load_sfz (const std::string &data);
  
  void setup_auto_mute(); //if not defined at the kit
  
  CDrumSample* load_sample_to_index (size_t index, const std::string &fname, int sample_rate);
  void remove_sample_at_index (size_t index);

  std::string get_description();

  CDrumKit();
  ~CDrumKit();

  void rnd_set_rnd_seed (uint64_t seed);

  
  void print();
  void print_stats();
  
  void adapt(); //for Adapt function
  
};


class CDrumKitsScanner
{
public:

  std::vector <std::string> v_kits_dirs;
  std::vector <std::string> v_kits_names;
  std::map <std::string, std::string> map_kits; //name = full path

  CDrumKitsScanner();
  ~CDrumKitsScanner();

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
int get_rnd (int ta, int tb);


#endif
