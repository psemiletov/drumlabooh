/*
written at 2023-24 by Peter Semiletov
this code is the public domain
 */

#ifndef KITS_H
#define KITS_H

#include <vector>
#include <map>

#include <stdint.h>
#include <string.h>

#include <JuceHeader.h>

//JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wno-switch-enum -Wzero-as-null-pointer-constant")
#include "pugixml.hpp"
//JUCE_END_IGNORE_WARNINGS_GCC_LIKE

class CDrumSample;

class CDrumLayer
{
public:

  int session_samplerate; //uplink (session) samplerate

  CDrumSample *drum_sample;

  //for layer's velo range
  float min;
  float max;

  std::string file_name; //name of the loaded file

 // int channels; //channels at this layer
 //channels veariable has been removed as we have mono only

  int samplerate; //samplerate of this layer
  int length_in_samples;// i.e frames, the amount of samples per channel
  int sample_offset; //to play inc until < length_in_samples

  juce::AudioBuffer<float> *audio_buffer; //holds audio data
  const float *channel_data [2]; // pointers to channels of audio_buffer

  CDrumLayer (CDrumSample *s);
  ~CDrumLayer();

  void load (const std::string &fname); //loads the sample, sets internally info, data, file_name
  void print();

  juce::AudioBuffer<float> * load_whole_sample (const std::string &fname); //called from load_whole_sample_resampled
  juce::AudioBuffer<float> * load_whole_sample_resampled (const std::string &fname, int sess_samplerate);

};



class CDrumSample
{
public:


  std::string name;
  int id;
  int current_layer;
  int midiOutNote;
  int session_samplerate; //session srate, taken from the upper object

  bool active;

  bool use_rnd;
  
  float velocity;

  bool hihat_open;
  bool hihat_close;

  std::vector <CDrumLayer*> v_layers;

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

  bool scan_mode; //if false, we do not load kit' samples

  std::string kit_name; //parsed from XML
  std::string kit_filename; //full path to the kit xml, txt or sfz file
  std::string kit_dir; //full path to the kit

  std::string image_fname;

  int samplerate; //session srate

  bool layers_supported;
  
  std::vector <CDrumSample*> v_samples;
  std::vector <std::string> v_hat_open_signatures;
  std::vector <std::string> v_hat_close_signatures;

  void add_sample();
  void load (const std::string &fname, int sample_rate);
  void load_txt (const std::string &fname);
  void load_sfz (const std::string & fname);

  CDrumKit();
  ~CDrumKit();

  void print();
  void print_stats();
};


class CDrumKitsScanner
{
public:

  std::vector <std::string> v_kits_dirs;
  std::vector <std::string> v_kits_names;
  std::vector <CDrumKit*> v_scanned_kits;
  std::map <std::string, std::string> map_kits; //name = full path

  CDrumKitsScanner();
  ~CDrumKitsScanner();

  void scan();
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
