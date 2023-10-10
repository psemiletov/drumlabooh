/*
written at 2023 by Peter Semiletov
this code is the public domain
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <algorithm>
#include <string>
#include <chrono>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include "kits.h"
#include "utl.h"
#include "speex_resampler_cpp.hpp"


using namespace std;

#define MAX_SAMPLES 36

juce::AudioFormatManager *formatManager;


//WE READ JUST LEFT CHANNEL IF STEREO
juce::AudioBuffer<float> *  CDrumLayer::load_whole_sample (const std::string &fname)
{
//  std::cout << "@@@@@ CDrumLayer::load_whole_sample: " << fname << std::endl;

  juce::File fl (fname);

  juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float>;

  std::unique_ptr<juce::AudioFormatReader> reader (formatManager->createReaderFor (fl));

  if (reader.get() != nullptr)
     {
      int bufsize = (int) reader->lengthInSamples;
      //buffer->setSize ((int) reader->numChannels, bufsize);
      buffer->setSize ((int) 1, bufsize);

      // if (! reader->read (buffer,  0, bufsize, 0,  true, true))
      if (! reader->read (buffer,  0, bufsize, 0,  true, false)) //read just left channel
         {
          std::cout << "! reader->read " << std::endl;
          delete buffer;
          return 0;
         }

      samplerate = reader->sampleRate;
      length_in_samples = reader->lengthInSamples;

      //hardcode 1 channel please :)
      channels = 1;//reader->numChannels;

//      std::cout << "samplerate: " << samplerate << std::endl;
//      std::cout << "length_in_samples: " << length_in_samples << std::endl;
//      std::cout << "channels: " << channels << std::endl;
     }

//  std::cout << "@@@@@ CDrumLayer::load_whole_sample END" << std::endl;

  return buffer;
}


juce::AudioBuffer<float> * CDrumLayer::load_whole_sample_resampled (const std::string &fname, int sess_samplerate)
{
  juce::AudioBuffer<float> *buffer = load_whole_sample (fname);
  if (! buffer)
     {
      cout << "load error: " << fname << endl;
      return 0;
     }

  if (samplerate == sess_samplerate)
      return buffer;

  //else we need to resample

  float ratio = (float) sess_samplerate / samplerate;
  size_t output_frames_count = ratio * length_in_samples;

  juce::AudioBuffer<float> * out_buf = new juce::AudioBuffer <float> (channels, output_frames_count);

  for (int i = 0; i < channels; i++)
      {
       float *input_buffer = buffer->getWritePointer(i);
       if (! input_buffer)
          continue;

       std::shared_ptr<speex_resampler_cpp::Resampler> rs = speex_resampler_cpp::createResampler (length_in_samples, 1, samplerate, sess_samplerate);
       rs->read (input_buffer);
       rs->write (out_buf->getWritePointer(i), output_frames_count);
      }

  samplerate = sess_samplerate;
  length_in_samples = output_frames_count;

  delete buffer;

  return out_buf;
}


void CDrumLayer::load (const std::string &fname)
{
  audio_buffer = load_whole_sample_resampled (fname, session_samplerate);

  if (! audio_buffer)
     {
      return;
      std::cout << "CDrumLayer::load ERROR: " << fname << std::endl;
     }

  file_name = fname;

  if (channels > 0)
      channel_data [0] = audio_buffer->getReadPointer (0);

  //altough we have just mono sample, this "stereo" code is a legacy

  if (channels > 1)
      channel_data [1] = audio_buffer->getReadPointer (1);
}


CDrumLayer::CDrumLayer (CDrumSample *s)
{
  drum_sample = s;
  session_samplerate = drum_sample->session_samplerate;
  sample_offset = 0;
  audio_buffer = 0;
  length_in_samples = 0;
}


void CDrumLayer::print()
{
  cout << "sample layer -- start" << endl;
  cout << "file_name: " << file_name << endl;
  cout << "min: " << min << endl;
  cout << "max: " << max << endl;
  cout << "sample layer -- end"  << endl;
}


CDrumLayer::~CDrumLayer()
{
  if (audio_buffer)
      delete audio_buffer;
}


CDrumSample::CDrumSample (int sample_rate)
{
  session_samplerate = sample_rate;
  current_layer = 0;
  velocity = 0.0f;
  hihat_open = false;
  hihat_close = false;
  active = false;
}


CDrumSample::~CDrumSample()
{
  for (size_t i = 0; i < v_layers.size(); i++)
      {
       delete v_layers[i];
      }
}


//#define GAIN_MIN -60.0f

size_t CDrumSample::map_velo_to_layer_number (float velo)
{
  if (v_layers.size() == 1)
     return 0; //return zero pos layer if we have just one layer

  size_t result = 0;

  //search for the layer within its min..max velo
  for (size_t i = 0; i < v_layers.size(); i++)
      {
       if (v_layers[i]->min <= velo &&
          (v_layers[i]->max > velo ||
          (v_layers[i]->max == 1 && velo == 1)))
          {
           result = i;
           break;
          }
       }

  return result;
}


void CDrumSample::add_layer()
{
  CDrumLayer *l = new CDrumLayer (this);
  v_layers.push_back (l);
}


void CDrumSample::print()
{
  cout << "CDrumSample -- start"  << endl;

  cout << "id: " << id << endl;
  cout << "name: " << name << endl;
  cout << "midiOutNote: " << midiOutNote << endl;

  for (auto l: v_layers)
      {
       l->print();
      }

  cout << "CDrumSample -- end"  << endl;
}


void CDrumSample::print_stats()
{
//  cout << "id: " << id << endl;
  cout << "name: " << name << endl;
 // cout << "midiOutNote: " << midiOutNote << endl;
}


CHydrogenXMLWalker::CHydrogenXMLWalker (CDrumKit *hkit)
{
  kit = hkit;
  drumkit_info_passed = false;
  drumkitComponent_passed = false;
}


bool CHydrogenXMLWalker::for_each (pugi::xml_node &node)
{
  string node_name = node.name();
  pugi::xml_text txt = node.text();

   //if (node_name == "drumkit_info")
     // drumkit_info_passed = true;

  if (node_name == "drumkitComponent")
     drumkit_info_passed = true;

  if (node_name == "instrument")
     drumkitComponent_passed = true;

  if (node_name == "name" && ! drumkit_info_passed)
     kit->kit_name = txt.as_string();

  if (node_name == "name" && drumkit_info_passed && drumkitComponent_passed)
     if (kit->v_samples.size() != 0)
         kit->v_samples.back()->name = txt.as_string();

  if (node_name == "id" && drumkit_info_passed && drumkitComponent_passed && ! kit->scan_mode)
     if (kit->v_samples.size() != 0)
         kit->v_samples.back()->id = txt.as_int();

  if (node_name == "min" && drumkit_info_passed && drumkitComponent_passed && ! kit->scan_mode)
     if (kit->v_samples.size() != 0)
         kit->v_samples.back()->v_layers.back()->min = txt.as_float();

  if (node_name == "max" && drumkit_info_passed && drumkitComponent_passed  && ! kit->scan_mode)
     if (kit->v_samples.size() != 0)
         kit->v_samples.back()->v_layers.back()->max = txt.as_float();

  if (node_name == "image")
     kit->image_fname = kit->kit_dir + "/" + txt.as_string();


  if (node_name == "instrument")
     {
      drumkit_info_passed = true;

      if (kit->v_samples.size() == MAX_SAMPLES) //WE DON'T LOAD MORE THAN 36 SAMPLES
         return false;

      kit->add_sample();

      if (! kit->layers_supported) //non-layered
         kit->v_samples.back()->add_layer(); //add default layer
     }


  if (node_name == "layer" && ! kit->scan_mode)
     {
      if (kit->v_samples.size() != 0)
          kit->v_samples.back()->add_layer();
     }


  if (node_name == "filename")
     {
      std::string fname = txt.as_string();
      std::string path = kit->kit_dir + "/" + fname;
      std::string sample_name = kit->v_samples.back()->name;

      for (auto signature: kit->v_hat_open_signatures)
          {
           if (findStringIC (sample_name, signature) || findStringIC (fname, signature))
              {
               kit->v_samples.back()->hihat_open = true;
               break;
              }
          }

      for (auto signature: kit->v_hat_close_signatures)
          {
           if (findStringIC (sample_name, signature) || findStringIC (fname, signature))
              {
               kit->v_samples.back()->hihat_close = true;
               break;
              }
          }

      if (! kit->scan_mode && kit->v_samples.size() != 0)
          if (kit->v_samples.back()->v_layers.size() != 0)
                kit->v_samples.back()->v_layers.back()->load (path.c_str());
     }


  return true;
}


void CDrumKit::load_txt (const std::string &data)
{
//  cout << "void CHydrogenKit::load_txt (const std::string data)\n";

  if (data.empty())
      return;

  size_t i = kit_dir.rfind ("/");
  kit_name = kit_dir.substr (i + 1);

  stringstream st (data);
  string line;

  while (getline (st, line))
        {
         if (line.empty())
            continue;

         if (v_samples.size() == MAX_SAMPLES) //WE DON'T LOAD MORE THAN 32 SAMPLES
            break;

         size_t pos = line.find ("=");

         if (pos == string::npos)
             continue;

         if (pos > line.size())
             continue;

         string sample_name = line.substr (0, pos);
         string fname = line.substr (pos + 1, line.size() - pos);

         if (fname.empty())
            continue;

         size_t check_for_list = fname.find (",");

         if (check_for_list != string::npos)
            {
             vector <string> v_fnames = split_string_to_vector (fname, ",", false);

             add_sample();
             v_samples.back()->name = sample_name;

             for (auto f: v_fnames)
                 {
                  string filename = kit_dir + "/" + f;
                  v_samples.back()->add_layer();

                  if (file_exists (filename) && ! scan_mode)
                      v_samples.back()->v_layers.back()->load (filename.c_str());
                 }

             float part_size = (float) 1 / v_samples.back()->v_layers.size();
             CDrumLayer *l;
              //evaluate min and max velocities by the file position in the vector
             for (size_t i = 0; i < v_samples.back()->v_layers.size(); i++)
                 {
                  l = v_samples.back()->v_layers[i];

                  float segment_start = part_size * i;
                  float segment_end = part_size * (i + 1) - 0.001;

                  //std::cout << "segment_start: " << segment_start << std::endl;
                  //std::cout << "segment_end: " << segment_end << std::endl;

                  l->min = segment_start;
                  l->max = segment_end;
                 }

             l->max = 1.0f;
            }
         else
             {
              string filename = kit_dir + "/" + fname;

              add_sample();

              v_samples.back()->name = sample_name;

              v_samples.back()->add_layer(); //add default layer

              if (file_exists (filename) && ! scan_mode)
                  v_samples.back()->v_layers.back()->load (filename.c_str());
             }


         for (auto signature: v_hat_open_signatures)
             {
              if (findStringIC (sample_name, signature) || findStringIC (fname, signature))
                 {
                  v_samples.back()->hihat_open = true;
                  break;
                 }
             }

         for (auto signature: v_hat_close_signatures)
             {
              if (findStringIC (sample_name, signature) || findStringIC (fname, signature))
                 {
                  v_samples.back()->hihat_close = true;
                  break;
                 }
             }
        }

  std::string kitimg = kit_dir + "/image.jpg";

  if (! file_exists (kitimg))
      kitimg = kit_dir + "/image.png";

  if (file_exists (kitimg))
      image_fname = kitimg;
}


std::string guess_sample_name (const std::string &raw)
{
  std::string result;

  std::string t = raw;

  //remove .ext

  t.pop_back();
  t.pop_back();
  t.pop_back();
  t.pop_back();

  //remove part before slash

  size_t pos = t.find ("/");

  if (pos != string::npos)
      t = t.substr (pos + 1);

  //remove all non-letters

  for (size_t i = 0; i < t.size(); i++)
      if (isalpha(t[i]))
         result += t[i];

  return result;
}


// trim from right
inline std::string& rtrim (std::string& s, const char* t = " \t\n\r\f\v")
{
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}


void CDrumKit::load_sfz (const std::string &data)
{
//  cout << "void CHydrogenKit::load_sfz (const std::string data)\n";

  if (data.empty())
      return;

  //change crlf in data to lf

  std::string temp_data = string_replace_all (data, "\r\n", "\n");
  temp_data = string_replace_all (data, "\\", "/");

  bool multi_layered = false;

  size_t pos = temp_data.find ("<group>");
  if (pos != string::npos)
     multi_layered = true;

  size_t i = kit_dir.rfind ("/");
  kit_name = kit_dir.substr (i + 1);

  stringstream st (temp_data);
  string line;

  while (getline (st, line))
        {
         if (v_samples.size() == MAX_SAMPLES) //WE DON'T LOAD MORE THAN 32 SAMPLES
             return;

         if (line.empty())
             continue;

         if (line.find ("//") != string::npos)
             continue;

         string fname;

//          cout << "parse line: " << line << endl;
         if (line.find ("<group>") != string::npos)
             add_sample();

         if (line.find("<region>") != string::npos  && ! multi_layered)
             add_sample();

         //parse filename for a layer

         pos = line.find ("sample=");

         if (pos != string::npos)
            {
             string just_name = line.substr (pos + 7);
             just_name = rtrim (just_name); //remove trailing spaces if any

             fname = kit_dir + "/" + just_name;

             v_samples.back()->add_layer();

             if (file_exists (fname))
                {
                 if (! scan_mode)
                     v_samples.back()->v_layers.back()->load (fname.c_str());

                 v_samples.back()->name = guess_sample_name (just_name);
                }
            }


         if (! scan_mode && multi_layered && v_samples.back()->v_layers.size() != 0)
            {
             float part_size = (float) 1 / v_samples.back()->v_layers.size();
             CDrumLayer *l;
              //evaluate min and max velocities by the file position in the vector
             for (size_t i = 0; i < v_samples.back()->v_layers.size(); i++)
                 {
                  l = v_samples.back()->v_layers[i];

                  l->min = part_size * i;;
                  l->max = part_size * (i + 1) - 0.001;;
                 }

             l->max = 1.0f;
            }

         if (! scan_mode && v_samples.size() > 0)
            {
             for (auto signature: v_hat_open_signatures)
                 {
                  if (findStringIC (v_samples.back()->name, signature))
                     {
                      v_samples.back()->hihat_open = true;
                      break;
                     }
                  }

            for (auto signature: v_hat_close_signatures)
                {
                 if (findStringIC (v_samples.back()->name, signature))
                    {
                     v_samples.back()->hihat_close = true;
                     break;
                    }
                }
           }
        }
}


void CDrumKit::load (const std::string &fname, int sample_rate)
{
//  if (! scan_mode)
  //   cout << "@@@@@@@@@@@@ void CDrumKit::load: " << fname << "samplerate: " << sample_rate << endl;

  auto start = chrono::high_resolution_clock::now();

  samplerate = sample_rate;

  string filename = resolve_symlink (fname.c_str());
  kit_filename = filename;
  kit_dir = get_file_path (kit_filename);

  std::string source = string_file_load (filename);
  if (source.empty())
      return;

  if (ends_with (kit_filename, ".txt"))
     {
      load_txt (source);
      return;
     }

  if (ends_with (kit_filename, ".sfz"))
     {
      load_sfz (source);
      return;
     }

  //else Hydrogen format

  pugi::xml_document doc;

 // cout << "loading kit: " << fname << endl;
  //cout << "source: " << source << endl;

  size_t r = source.find ("<layer>");
  if (r != std::string::npos)
      layers_supported = true;
   else
       layers_supported = false;

  //delete empty instruments
  //because we don't want parse them

  size_t idx_filename = source.rfind ("</filename>");
  size_t idx_instrument = source.find ("<instrument>", idx_filename);

  if (idx_instrument != std::string::npos)
  if (idx_instrument > idx_filename)
     //oops, we have empty instruments!
     {
      //первый пустой инструмент у нас уже есть, он находится по
      //idx_instrument

      //теперь найдем конец последнего
      size_t idx_instrument_end = source.rfind ("</instrument>");
      size_t sz_to_remove = idx_instrument_end - idx_instrument + 13;

      source = source.erase (idx_instrument, sz_to_remove);
     }


  pugi::xml_parse_result result = doc.load_buffer (source.c_str(), source.size());

  if (! result)
     return;

  CHydrogenXMLWalker walker (this);

  doc.traverse (walker);

  auto stop = chrono::high_resolution_clock::now();
  auto duration_msecs = chrono::duration_cast<chrono::milliseconds>(stop - start);

 // std::cout << "loaded at: " << duration_msecs.count() << " msecs" << std::endl;

  //seconds_counter_ev = duration_s.count();
}


CDrumKit::CDrumKit()
{
  scan_mode = false;
  layers_supported = false;

  v_hat_open_signatures.push_back ("hat_o");
  v_hat_open_signatures.push_back ("open");
  v_hat_open_signatures.push_back ("swish");

  v_hat_close_signatures.push_back ("close");
  v_hat_close_signatures.push_back ("choke");
  v_hat_close_signatures.push_back ("hat_c");
}


CDrumKit::~CDrumKit()
{
  for (size_t i = 0; i < v_samples.size(); i++)
      {
       delete v_samples[i];
      }
}


void CDrumKit::add_sample()
{
  CDrumSample *s  = new CDrumSample (samplerate);
  v_samples.push_back (s);
}


void CDrumKit::print()
{
  cout << "void CDrumKit::print() -- start" << endl;

  for (size_t i = 0; i < v_samples.size(); i++)
      {
       v_samples[i]->print();
      }

  cout << "samples count:" << v_samples.size() << endl;

  cout << "void CDrumKit::print() -- end" << endl;
}


void CDrumKit::print_stats()
{
  cout << "void CDrumKit::print-stats() -- start" << endl;

  cout << "kitname: " << kit_name << endl;

  for (size_t i = 0; i < v_samples.size(); i++)
      {
       v_samples[i]->print_stats();
      }

  cout << "void CDrumKit::print-stats() -- end" << endl;
}


CDrumKitsScanner::CDrumKitsScanner()
{
}


CDrumKitsScanner::~CDrumKitsScanner()
{
  for (size_t i = 0; i < v_scanned_kits.size(); i++)
      {
       delete v_scanned_kits[i];
      }
}


void CDrumKitsScanner::scan()
{
  std::vector <std::string> v_kits_locations;

#if !defined(_WIN32) || !defined(_WIN64)

  v_kits_locations.push_back ("/usr/share/hydrogen/data/drumkits");
  v_kits_locations.push_back ("/usr/local/share/hydrogen/data/drumkits");
  v_kits_locations.push_back ("/usr/share/drmr/drumkits");
  v_kits_locations.push_back ("/usr/share/drumrox-kits");
  v_kits_locations.push_back ("/usr/share/drumlabooh-kits");
  v_kits_locations.push_back ("/usr/local/share/drumlabooh-kits");

  v_kits_locations.push_back (get_home_dir() + "/.hydrogen/data/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.drmr/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/drumrox-kits");
  v_kits_locations.push_back (get_home_dir() + "/drumlabooh-kits");
  v_kits_locations.push_back (get_home_dir() + "/sfz-kits");
/*
  juce::File home_location = File::getSpecialLocation	(juce::File::SpecialLocationType::userHomeDirectory);
  const String & fnm = home_location.getFullPathName();
  std::string sfnm (fnm.toStdString());
  sfnm += "\\";
  */
#else

 // juce::File home_location = File::getSpecialLocation	(juce::File::SpecialLocationType::userHomeDirectory);


  v_kits_locations.push_back ("c:\\drumlabooh-kits");
  v_kits_locations.push_back ("c:\\sfz-kits");
  v_kits_locations.push_back ("d:\\drumlabooh-kits");
  v_kits_locations.push_back ("d:\\sfz-kits");

  //v_kits_locations.push_back (get_home_dir() + "/.hydrogen/data/drumkits");

  juce::File home_location = File::getSpecialLocation (juce::File::SpecialLocationType::userHomeDirectory);
  const String & fnm = home_location.getFullPathName();
  std::string sfnm (fnm.toStdString());
  sfnm += "\\";
  sfnm += ".hydrogen/data/drumkits";
  v_kits_locations.push_back (sfnm);

  /*
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  std::cout << get_home_dir() + "/.hydrogen/data/drumkits" << std::endl;

  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
*/

#endif

//  std::vector <std::string> v_kits_dirs;

  for (std::string i: v_kits_locations)
      {
       std::vector <std::string> v_kits_dirs_t = files_get_list (i);
       if (v_kits_dirs_t.size() > 0)
          v_kits_dirs.insert (v_kits_dirs.end(), v_kits_dirs_t.begin(), v_kits_dirs_t.end());
      }

  std::sort (v_kits_dirs.begin(), v_kits_dirs.end());
  v_kits_dirs.erase (std::unique (v_kits_dirs.begin(), v_kits_dirs.end() ), v_kits_dirs.end() );

  for (std::string kd: v_kits_dirs)
      {
       //cout << kd << endl;
       //cout << get_kit_name (kd + "/drumkit.xml") << endl;

       bool kit_exists = false;

       std::string fname = kd + "/drumkit.xml";

       if (file_exists (fname))
          kit_exists = true;
       else
           {
            fname = kd + "/drumkit.txt";
            if (file_exists (fname))
               kit_exists = true;
           }


       if (kd.find ("/sfz-kits") != string::npos)
          {
           //search sfz file
           std::vector <std::string> v = files_get_list (kd, ".sfz");
           if (v.size() != 0)
              fname = v[0];

           if (file_exists (fname))
               kit_exists = true;
          }


       if (kit_exists)
          {
           CDrumKit *kit = new CDrumKit;
           kit->scan_mode = true;
           kit->load (fname.c_str(), 44100);
           v_scanned_kits.push_back (kit);
//           v_kits_names.push_back (kit->kit_name);
           map_kits.insert (pair<string,string> (kit->kit_name, fname));
          }

      }

  std::sort (v_scanned_kits.begin(), v_scanned_kits.end(), [](CDrumKit* a, CDrumKit* b) {return a->kit_name < b->kit_name;});

  for (auto i : v_scanned_kits)
      {
       v_kits_names.push_back (i->kit_name);
      }

}


void CDrumKitsScanner::print()
{
  for (size_t i = 0; i < v_scanned_kits.size(); i++)
     {
      std::cout << i << ": ";
      v_scanned_kits[i]->print_stats();
      std::cout << v_scanned_kits[i]->kit_name << std::endl;
     }
}


void CDrumSample::untrigger_sample()
{
//  std::cout << "CDrumSample::UNtrigger_sample" << std::endl;

  active = false;

 // for (size_t i = 0; i < v_layers.size(); i++)
   //   v_layers[i]->sample_offset = 0;

  current_layer = 0;
}


void CDrumSample::trigger_sample (float vel)
{
//  std::cout << "CDrumSample::trigger_sample: " << name << std::endl;

  //v_layers[current_layer]->sample_offset = 0;

  active = true;

  velocity = vel;
  current_layer = map_velo_to_layer_number (velocity);

  //std::cout << "velo: " << velocity << " layer: " << current_layer << std::endl;

  v_layers[current_layer]->sample_offset = 0;
}
