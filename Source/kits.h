/*
written at 2023 by Peter Semiletov
this code is the public domain
 */

#ifndef KITS_H
#define KITS_H

#include <vector>
#include <map>

#include <stdint.h>
#include <string.h>

#include <JuceHeader.h>

#include "pugixml.hpp"


class CDrumSample;

class CDrumLayer
{
public:

  int session_samplerate; //uplink (session) samplerate

  CDrumSample *drum_sample;

  //for layer velo range
  float min;
  float max;

  std::string file_name; //name of the loaded file

  int channels;
  int samplerate;

 // int frames; //eq to lengthInSamples


  int lengthInSamples;
  int sample_offset; //to play inc until < lengthInSamples


  juce::AudioBuffer<float> *audio_buffer;
  const float *channel_data [2];

  //uint32_t offset;
  //int dataoffset;

  CDrumLayer (CDrumSample *s); //sample_rate is uplink (session) samplerate
  ~CDrumLayer();

  void load (const std::string &fname); //loads the sample, sets internally info, data, file_name
  void print();

  juce::AudioBuffer<float> * load_whole_sample (const std::string &fname); //called from load_whole_sample_resampled
  juce::AudioBuffer<float> * load_whole_sample_resampled (const std::string &fname, int sess_samplerate);

  //called from load
};


//class CHydrogenKit;

class CDrumSample
{
public:


  //CHydrogenKit *kit; //uplink


  std::string name;
  int id;
  int current_layer;
  int midiOutNote;
  int session_samplerate; //session srate, taken from the upper object

  bool active;
  float velocity;

  //bool hihat;
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



class CHydrogenKit
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

  CHydrogenKit();
  ~CHydrogenKit();

  void print();
  void print_stats();
};


class CHydrogenKitsScanner
{
public:

  std::vector <std::string> v_kits_dirs;
  std::vector <std::string> v_kits_names;
  std::vector <CHydrogenKit*> v_scanned_kits;
  std::map <std::string, std::string> map_kits; //name = full path

  CHydrogenKitsScanner();
  ~CHydrogenKitsScanner();

  void scan();
  void print();
};



class CHydrogenXMLWalker: public pugi::xml_tree_walker
{
public:

  CHydrogenKit *kit;

  bool is_drumkit_info;
  bool is_instrument;
  bool is_layer;
  bool drumkit_info_passed;
  bool drumkitComponent_passed;

  CHydrogenXMLWalker (CHydrogenKit *hkit);
  bool for_each (pugi::xml_node& node);
};



#endif

