/*
2023-25, Peter Semiletov
*/

#include <random>

#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "utl.h"
#include "dsp.h"



//extern juce::AudioFormatManager *formatManager;

extern std::mt19937 rnd_mt19937;


juce::AudioProcessorValueTreeState::ParameterLayout CAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  for (size_t i = 0; i < 36; i++)
      {

    #ifndef MULTICHANNEL

       pans[i] = nullptr;
       vols[i] = nullptr;
       mutes[i] = nullptr;

    #endif

       lps[i] = nullptr;
       hps[i] = nullptr;

       lp_cutoff[i] = nullptr;
       lp_reso[i] = nullptr;

       hp_cutoff[i] = nullptr;
       hp_reso[i] = nullptr;

       analog[i] = nullptr;
       analog_amount[i] = nullptr;
      }


  layout.add (std::make_unique<juce::AudioParameterFloat> ("midimap_mode",            // parameterID
                                                           "midimap_mode",            // parameter name
                                                           1.0f,              // minimum value
                                                           2.0f,              // maximum value
                                                           1.0f)); //default
    
      
#ifndef MULTICHANNEL

  layout.add (std::make_unique<juce::AudioParameterFloat> ("panner_mode",            // parameterID
                                                           "panner_mode",            // parameter name
                                                           1.0f,              // minimum value
                                                           7.0f,              // maximum value
                                                           1.0f)); //default

#endif

  layout.add (std::make_unique<juce::AudioParameterFloat> ("ignore_midi_velocity",      // parameterID
                                                           "ignore_midi_velocity",     // parameter name
                                                           0, 1, 0));



  for (size_t i = 0; i < 36; i++)
      {
        
#ifndef MULTICHANNEL

       layout.add (std::make_unique <juce::AudioParameterFloat> ("vol" + std::to_string(i), // parameter ID
                                                                 "vol" + std::to_string(i), // parameter name
                                                                  juce::NormalisableRange<float> (-96, 6, 0.1, 5), // parameter range
                                                                  0)); //default value

       layout.add (std::make_unique<juce::AudioParameterFloat> ("pan" + std::to_string(i), "pan" + std::to_string(i), 0.0f, 1.0f, 0.5f));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("mute" + std::to_string(i),      // parameterID
                                                                "mute" + std::to_string(i),     // parameter name
                                                                0, 1, 0));

#endif

       layout.add (std::make_unique<juce::AudioParameterFloat> ("lp" + std::to_string(i),      // parameterID
                                                                "lp" + std::to_string(i),     // parameter name
                                                                0, 1, 0));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("hp" + std::to_string(i),      // parameterID
                                                                "hp" + std::to_string(i),     // parameter name
                                                                0, 1, 0));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("analog" + std::to_string(i),      // parameterID
                                                                "analog" + std::to_string(i),     // parameter name
                                                                0, 1, 0));


       layout.add (std::make_unique<juce::AudioParameterFloat> ("lp_cutoff" + std::to_string(i),
                                                                "lp_cutoff" + std::to_string(i),
                                                                 juce::NormalisableRange<float> (0, 0.999f, 0.001f), // parameter range
                                                                 0.999f));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("lp_reso" + std::to_string(i),
                                                                "lp_reso" + std::to_string(i),
                                                                 juce::NormalisableRange<float> (0, 1.0f, 0.001f), // parameter range
                                                                 0.001f));


       layout.add (std::make_unique<juce::AudioParameterFloat> ("hp_cutoff" + std::to_string(i),
                                                                "hp_cutoff" + std::to_string(i),
                                                                 juce::NormalisableRange<float> (0, 0.999f, 0.001f), // parameter range
                                                                 0.999f));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("hp_reso" + std::to_string(i),
                                                                "hp_reso" + std::to_string(i),
                                                                 juce::NormalisableRange<float> (0, 1.0f, 0.001f), // parameter range
                                                                 0.001f));


       layout.add (std::make_unique<juce::AudioParameterFloat> ("analog_amount" + std::to_string(i),
                                                                "analog_amount" + std::to_string(i),
                                                                 juce::NormalisableRange<float> (0.001f, 1.0f, 0.001f), // parameter range
                                                                 0.001f));
      }


      
     layout.add (std::make_unique<juce::AudioParameterFloat> ("global_analog_on",      // parameterID
                                                              "global_analog_on",     // parameter name
                                                               0, 1, 0));
     
     layout.add (std::make_unique<juce::AudioParameterFloat> ("global_analog_amount",
                                                              "global_analog_amount",
                                                               juce::NormalisableRange<float> (0.001f, 1.0f, 0.001f), // parameter range
                                                               0.001f));
      
   /* layout.add (std::make_unique<juce::AudioParameterFloat> ("randomizer_seed",
                                                              "randomizer_seed",
                                                               juce::NormalisableRange<float> (1, 65536, 1), // parameter range
                                                               1));
     */ 
  return layout;
}


void CAudioProcessor::reset_layer_index()
{
  for (size_t i = 0; i < 36; i++)
     layer_index[i] = 0;
}


#if defined(MULTICHANNEL)

CAudioProcessor::CAudioProcessor()
                                 :AudioProcessor (BusesProperties()
                                 .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                 .withOutput("Drumlabooh out 01", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 02", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 03", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 04", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 05", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 06", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 07", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 08", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 09", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 10", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 11", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 12", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 13", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 14", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 15", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 16", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 17", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 18", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 19", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 20", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 21", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 22", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 23", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 24", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 25", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 26", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 27", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 28", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 29", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 30", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 31", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 32", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 33", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 34", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 35", AudioChannelSet::mono(), true)
                                 .withOutput("Drumlabooh out 36", AudioChannelSet::mono(), true)),
                                 
                                  parameters (*this, 0, "Drumlabooh-multi", createParameterLayout())


{
 //std::cout << "CAudioProcessor::CAudioProcessor()36 - 1" << std::endl;
  
  init_db();
  rnd_init();

  scanner.scan();

  fresh_start = true;
  drumkit = 0;
  drumkit_path = "";
  base_note_number = 36;
  randomizer_seed = get_rnd (1, RND_MAX_TWEAK);
  session_samplerate = 0;

  for (size_t i = 0; i < 36; i++)
      {
       layer_index[i] = 0;  
        
       lps[i] = parameters.getRawParameterValue ("lp" + std::to_string(i));
       lp_cutoff[i] = parameters.getRawParameterValue ("lp_cutoff" + std::to_string(i));
       lp_reso[i] = parameters.getRawParameterValue ("lp_reso" + std::to_string(i));

       hps[i] = parameters.getRawParameterValue ("hp" + std::to_string(i));
       hp_cutoff[i] = parameters.getRawParameterValue ("hp_cutoff" + std::to_string(i));
       hp_reso[i] = parameters.getRawParameterValue ("hp_reso" + std::to_string(i));

       analog[i] = parameters.getRawParameterValue ("analog" + std::to_string(i));
       analog_amount[i] = parameters.getRawParameterValue ("analog_amount" + std::to_string(i));
      }

  //panner_mode = parameters.getRawParameterValue ("panner_mode");
  ignore_midi_velocity = parameters.getRawParameterValue ("ignore_midi_velocity");
  global_analog_on = parameters.getRawParameterValue ("global_analog_on");
  global_analog_amount = parameters.getRawParameterValue ("global_analog_amount");
  midimap_mode = parameters.getRawParameterValue ("midimap_mode");
}


#else
//STEREO

CAudioProcessor::CAudioProcessor()
                                  :AudioProcessor (BusesProperties()
                                      .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                 ),
                                  parameters (*this, 0, "Drumlabooh", createParameterLayout())
{
//std::cout << "CAudioProcessor::CAudioProcessor() - 1" << std::endl;
  
  init_db();
  rnd_init();
  scanner.scan();
  
  fresh_start = true;
  drumkit = 0;
  drumkit_path = "";
  base_note_number = 36;
  randomizer_seed = get_rnd (1, RND_MAX_TWEAK);

  
  session_samplerate = 0;

  for (size_t i = 0; i < 36; i++)
      {
       layer_index[i] = 0; 
       
       vols[i] = parameters.getRawParameterValue ("vol" + std::to_string(i));
       pans[i] = parameters.getRawParameterValue ("pan" + std::to_string(i));
       mutes[i] = parameters.getRawParameterValue ("mute" + std::to_string(i));

       lps[i] = parameters.getRawParameterValue ("lp" + std::to_string(i));
       lp_cutoff[i] = parameters.getRawParameterValue ("lp_cutoff" + std::to_string(i));
       lp_reso[i] = parameters.getRawParameterValue ("lp_reso" + std::to_string(i));

       hps[i] = parameters.getRawParameterValue ("hp" + std::to_string(i));
       hp_cutoff[i] = parameters.getRawParameterValue ("hp_cutoff" + std::to_string(i));
       hp_reso[i] = parameters.getRawParameterValue ("hp_reso" + std::to_string(i));

       analog[i] = parameters.getRawParameterValue ("analog" + std::to_string(i));
       analog_amount[i] = parameters.getRawParameterValue ("analog_amount" + std::to_string(i));
      }
     
      
  panner_mode = parameters.getRawParameterValue ("panner_mode");
  ignore_midi_velocity = parameters.getRawParameterValue ("ignore_midi_velocity");
  
  global_analog_on = parameters.getRawParameterValue ("global_analog_on");
  global_analog_amount = parameters.getRawParameterValue ("global_analog_amount");
  midimap_mode = parameters.getRawParameterValue ("midimap_mode");
  
}

#endif


CAudioProcessor::~CAudioProcessor()
{
  if (drumkit)
      delete drumkit;
}


const juce::String CAudioProcessor::getName() const
{
 // return JucePlugin_Name;

#ifndef MULTICHANNEL

 juce::String s = "drumlabooh";

#else

 juce::String s = "drumlabooh-multi";

#endif

 return s;
}


bool CAudioProcessor::acceptsMidi() const
{
  return true;
}


bool CAudioProcessor::producesMidi() const
{
  return false;
}


bool CAudioProcessor::isMidiEffect() const
{
  return false;
}


double CAudioProcessor::getTailLengthSeconds() const
{
  return 0.0;
}


int CAudioProcessor::getNumPrograms()
{
  return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}


int CAudioProcessor::getCurrentProgram()
{
  return 0;
}


void CAudioProcessor::setCurrentProgram (int index)
{
}


const juce::String CAudioProcessor::getProgramName (int index)
{
  return {};
}


void CAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}


//==============================================================================
void CAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
  session_samplerate = (int) sampleRate;

  //RELOAD IF sampleRate CHANGED DURING SESSION
  if (drumkit)
     if (drumkit->samplerate != session_samplerate)
        load_kit (drumkit_path);
   
}


void CAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}



#if defined(MULTICHANNEL)

//ok Reaper Linux
bool CAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const  
{
   if (layouts.getMainOutputChannels() == 36)
     return true;

  
  return false;
}

#else
//STEREO

bool CAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  if (layouts.getMainOutputChannels() == 2)
     return true;

  return false;
}

#endif


// Map MIDI velocity 0-127 to gain 0.0-1.0

#define VSENSE 1.0f
//#define VSENSE 0.0


float VelocityToLevel (int velocity)
{
  float min = 0.1f;
// velocity should logarithmically map onto [min..1]

  float logrange = logf (1.0f / min);
  float vcurve = powf ((float (velocity - 1) / 126.0f), 0.8f);

  return min * expf (logrange * vcurve);
}




//==============================================================================
bool CAudioProcessor::hasEditor() const
{
  return true; // (change this to false if you choose to not supply an editor)
}


juce::AudioProcessorEditor* CAudioProcessor::createEditor()
{
  return new CAudioProcessorEditor (*this, parameters);
}


//save
void CAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
  //std::string  get_home_dir
 
//  std::cout << "void CAudioProcessor::getStateInformation (juce::MemoryBlock& destData)\n";

  
  //for ALT kits 
  if (drumkit)
     {
      for (size_t i = 0; i < 36; i++)
          {
           CDrumSample *s = drumkit->a_samples[i];
           if (s)
               save_int_keyval ("layer_index" + std::to_string(i), s->current_layer);
           else
               save_int_keyval ("layer_index" + std::to_string(i), 0);
          }
     }
  
  std::string drumkitpath_to_save = transform_kit_path_from_local (drumkit_path); 
  
  std::cout << "SAVE PATH drumkitpath_to_save: " << drumkitpath_to_save << std::endl;
  std::cout << "SAVE PATH drumkit_path: " << drumkit_path << std::endl;
  
  //save_string_keyval ("drumkit_path", drumkit_path);
  save_string_keyval ("drumkit_path", drumkitpath_to_save);
  
  save_int_keyval ("base_note_number", base_note_number);
  save_int_keyval ("randomizer_seed", randomizer_seed);
    
  auto state = parameters.copyState();
  std::unique_ptr <juce::XmlElement> xml (state.createXml());
  copyXmlToBinary (*xml, destData);
}


//load
void CAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
  //std::cout << "CAudioProcessor::setStateInformation - 1" << std::endl;
  std::unique_ptr <juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

  if (xmlState.get() != nullptr)
     if (xmlState->hasTagName (parameters.state.getType()))
        {
         parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
         base_note_number = load_int_keyval ("base_note_number", 36);
         randomizer_seed = load_int_keyval ("randomizer_seed", get_rnd (1, 65536));
         
         drumkit_path = load_string_keyval ("drumkit_path");
         session_samplerate = getSampleRate();
         
         for (size_t i = 0; i < 36; i++)
            {
             std::string key = "layer_index" + std::to_string (i);
             layer_index[i] = load_int_keyval (key, 0);
//             std::cout << "read to layer_index[i] : " << i << ": " << layer_index[i]  << std::endl;
            }   
         
        }

  //std::cout << "CAudioProcessor::setStateInformation - 2" << std::endl;
}


void CAudioProcessor::save_string_keyval (const std::string &key, const std::string &val)
{
  auto addons = parameters.state.getOrCreateChildWithName ("addons", nullptr);
  addons.setProperty (key.c_str(), val.c_str(), nullptr);
}


std::string CAudioProcessor::load_string_keyval (const std::string &key)
{
  juce::Identifier keyid (key.c_str());
  auto addons = parameters.state.getOrCreateChildWithName ("addons", nullptr);
  auto text = addons.getProperty (keyid, juce::String()).toString();
//  std::cout << "TEXT: " << text << std::endl;
  return text.toStdString();
}


void CAudioProcessor::save_int_keyval (const std::string &key, int val)
{
  auto addons = parameters.state.getOrCreateChildWithName ("addons", nullptr);
  addons.setProperty (key.c_str(), val, nullptr);
}


int CAudioProcessor::load_int_keyval (const std::string &key, int defval)
{
  juce::Identifier keyid (key.c_str());
  auto addons = parameters.state.getOrCreateChildWithName ("addons", nullptr);
  auto v = addons.getProperty (keyid);

  if (v.isVoid())
     return defval;

  return v;
}


bool CAudioProcessor::load_kit (const std::string &fullpath)
{
  if (fullpath.empty())
      return false;

  std::cout << "CAudioProcessor::load_kit: " << fullpath << std::endl;
  std::cout << session_samplerate << std::endl;

//STOP PLAY

  suspendProcessing (true);

//THEN


  if (drumkit)
      delete drumkit;


  drumkit = new CDrumKit;

  drumkit->load (fullpath, session_samplerate);

  for (size_t i = 0; i < 36; i++)
      {
        //загрузи в КИТС из layer_index
       //////////НОВОЕ!!! 
       CDrumSample* s = drumkit->a_samples[i];
       if (s)
          s->current_layer = layer_index[i]; //в layer_index загружено из параметров плагина
        
  //     std::cout << "layer_index[i]: " << i << " :" << layer_index[i] << std::endl; 
        
       //if (drumkit->a_samples[i])
         // drumkit->a_samples[i]->current_layer = (*(layer_index[i]))
        
       lp[i].reset();
       hp[i].reset();
       hp[i].mode = FILTER_MODE_HIGHPASS;
    //   hp[i].reset(); //FIXME зачем в третий раз??
      }

//resume

  suspendProcessing (false);

 //std::cout << "CAudioProcessor::load_kit - 2" << std::endl;

  return true;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CAudioProcessor();
}


#if defined(MULTICHANNEL)


void CAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
  //the code block below allows Ardour load kit properly when session rate is ready
  if (fresh_start)
     {
      session_samplerate = getSampleRate();

      if (! drumkit_path.empty())
         {
          load_kit (drumkit_path);
          fresh_start = false;
         }
     }

  int num_channels = buffer.getNumChannels();

  if (num_channels < 1)
     return;

  int out_buf_length = buffer.getNumSamples();
   
  for (size_t i = 0; i < num_channels; i++)
      buffer.clear (i, 0, out_buf_length);
   //  buffer.clear(); 

  if (! drumkit)
      return;
   
  if (drumkit->sample_counter == 0)
      return;

  
  AudioPlayHead *play_head = getPlayHead();
  int64_t currentSamplePosition = 0;
  
  if (play_head)
     {
      auto positionInfo = play_head->getPosition();
      if (positionInfo)
         {
          auto timeInSamples = positionInfo->getTimeInSamples();
          // Безопасно получить значение, если оно есть, иначе оставить currentSamplePosition как 0:
          if (timeInSamples.hasValue())
             currentSamplePosition = *timeInSamples; // Используем оператор * для получения значения
          else
              currentSamplePosition = get_rnd (1, RND_MAX_TWEAK);
        }
     }

  if (currentSamplePosition == 0)
     currentSamplePosition = get_rnd (1, RND_MAX_TWEAK); 
   
   
  drumkit->rnd_set_rnd_seed (currentSamplePosition + randomizer_seed);
  
  
     //////////////////////
  // CACHE ALL ATOMIC PARAMETERS
 
 
  for (size_t i = 0; i < 35; i++)
     {
      //a_mutes [i] = *(mutes[i]) > 0.5f;
      a_lps [i] = *(lps[i]) > 0.5f;
      a_hps [i] = *(hps[i]) > 0.5f;
      
      a_analog_amount [i] = *(analog_amount[i]);
      a_analog_on [i] = *(analog[i]);
      
      a_lp_cutoff [i] = *(lp_cutoff[i]); 
      a_lp_reso [i] = *(lp_reso[i]);
      
      a_hp_cutoff [i] = *(hp_cutoff[i]); 
      a_hp_reso [i] = *(hp_reso[i]);
     } 
  
  
  int int_midimap_mode = (int) *midimap_mode;

  //END OF CACHING
  
  
  for (const juce::MidiMessageMetadata metadata: midiMessages)
      {
       juce::MidiMessage msg = metadata.getMessage();

       bool isNoteOn = msg.isNoteOn();
       bool isNoteOff = msg.isNoteOff();

       //float velocity = msg.getFloatVelocity();
       int uvelocity = msg.getVelocity();
       float velocity = VelocityToLevel (uvelocity);
       
       if (*ignore_midi_velocity > 0.5)
           velocity = 1;

       int note_number = msg.getNoteNumber();
       
       if (isNoteOn)
          {
           int nn = note_number - base_note_number; //base_note_number = 36 by default
                                                    // nn is index for drumkit->a_samples[nn]
           
           if (int_midimap_mode == MIDIMAPMODE_LABOOH)
               if (nn < 0 || nn > 35) // MAX_SAMPLES = 36
                   continue;

           CDrumSample *s = 0;

           if (int_midimap_mode == MIDIMAPMODE_LABOOH)
               s = drumkit->a_samples[nn];
            else
                if (drumkit->map_samples.count (note_number) > 0) 
                    s = drumkit->map_samples[note_number];
//                    std::cout << "play mapped note: " << note_number << std::endl;
  
            if (! s)
               continue;
  
            if (drumkit->kit_type == KIT_TYPE_HYDROGEN)
                s->trigger_sample (velocity);
            else //new
                if (drumkit->kit_type == KIT_TYPE_ALTDRUMLABOOH)
                   s->trigger_sample_uint_by_index (uvelocity, velocity);  
                else            
                   s->trigger_sample_uint (uvelocity, velocity);
            
//            std::cout << "s->current_layer:" << s->current_layer << std::endl;
  //          std::cout << "s->>v_layers.size():" << s->v_layers.size() << std::endl;


           //also untrigger open hihat if closed hihat triggering
           // so find the open hihat
             
          //AUTOMUTE START
             
           if (s->mute_group != -1)
              {
               for (size_t i = 0; i < MAX_SAMPLES; i++)
                   {
                    CDrumSample *s2 = drumkit->a_samples[i]; //point to the sample
                    if (! s2 || s == s2)
                       continue;
                      
                    if (s2->mute_group == s->mute_group)
                       {
                        if (drumkit->kit_type == KIT_TYPE_ALTDRUMLABOOH) 
                           s2->untrigger_sample (true);
                        else
                           s2->untrigger_sample (false);
                          
                       }
                   }
              }
           
           //AUTOMUTE END   
              
          }

     } //end of cycle for juce::MidiMessageMetadata metadata: midiMessages


     if (drumkit->sample_counter > num_channels) //YES????????????????????????????????????
        return;

       
     float *channel_data [MAX_SAMPLES]; //output channels
    

     for (size_t i = 0; i < num_channels; i++)
         channel_data[i] = buffer.getWritePointer(i);


    //for each drum instrument
     for (int drum_sample_index = 0; drum_sample_index < MAX_SAMPLES; drum_sample_index++)
        {
         //for each sample out_buf_offs
         for (int out_buf_offs = 0; out_buf_offs < out_buf_length; out_buf_offs++)
             {
              CDrumSample *s = drumkit->a_samples[drum_sample_index];
              if (! s)
                 // std::cout << "!s at drum_sample_index:" << drum_sample_index << std::endl;
                  continue;

              if (! s->active)
                 continue;


              //std::cout << "s->current_layer:" << s->current_layer << std::endl;

              CDrumLayer *l = s->v_layers.at (s->current_layer);

              if (! l)
                 {
                  std::cout << "!l at s->current_layer:" << s->current_layer << std::endl;
                  break;
                 }


              if (l->sample_offset + 1 == l->length_in_samples)
                 {
                  if (drumkit->kit_type == KIT_TYPE_ALTDRUMLABOOH)  
                     s->untrigger_sample (true);
                  else 
                      s->untrigger_sample (false);
                  
                  continue;
                 }

               if (l->channel_data)
                   {
                    float fl = l->channel_data[l->sample_offset++];

                 //DSP
                    bool analog_on = a_analog_on [drum_sample_index];
 
                    if (analog_on)
                       fl = warmify (fl, a_analog_amount[drum_sample_index]);


                    bool lp_on = a_lps [drum_sample_index];

                    if (lp_on)
                       {
                        lp[drum_sample_index].set_cutoff (a_lp_cutoff[drum_sample_index]);
                        lp[drum_sample_index].set_resonance (a_lp_reso[drum_sample_index]);
                        fl = softLimit (lp[drum_sample_index].process (fl));
                       }


                   //bool hp_on = *(hps[drum_sample_index]) > 0.5f;
                    bool hp_on = a_hps [drum_sample_index];

                                      
                   if (hp_on)
                      {
                       hp[drum_sample_index].set_cutoff (a_hp_cutoff[drum_sample_index]);
                       hp[drum_sample_index].set_resonance (a_hp_reso[drum_sample_index]);

                       fl = softLimit (hp[drum_sample_index].process (fl));
                      }
 
                   if (s->layer_index_mode != LAYER_INDEX_MODE_NOVELOCITY)
                       fl *= s->velocity;
                  
                   channel_data[drum_sample_index][out_buf_offs] = fl;
                  }

             }
        } 
 //std::cout << "CAudioProcessor::processBlock -6 " << std::endl;
  //}
}


#else
//STEREO


void CAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
  //this code allow Ardour load kit properly when session rate is ready
  if (fresh_start)
     {
      session_samplerate = getSampleRate();

      if (! drumkit_path.empty())
         {
          load_kit (drumkit_path);
          fresh_start = false;
         }
     }


  int num_channels = buffer.getNumChannels();
  int out_buf_length = buffer.getNumSamples();
  
  
  //clearing input buffer, good for Reaper
  for (int i = 0; i < num_channels; ++i)
      buffer.clear (i, 0, out_buf_length);

  if (! drumkit)
      return;

//  size_t v_samples_size = drumkit->v_samples.size();

  if (drumkit->sample_counter == 0)
      return;

  
  AudioPlayHead* play_head = getPlayHead();
  int64_t currentSamplePosition = 0;
  
  if (play_head)
     {
      auto positionInfo = play_head->getPosition();
      if (positionInfo)
         {
        auto timeInSamples = positionInfo->getTimeInSamples();
        // Безопасно получить значение, если оно есть, иначе оставить currentSamplePosition как 0:
        
        if (timeInSamples.hasValue())
            currentSamplePosition = *timeInSamples; // Используем оператор * для получения значения
        else
             currentSamplePosition = get_rnd (1, RND_MAX_TWEAK);
            
       }
    }
    
    
    
  if (currentSamplePosition == 0)
     currentSamplePosition = get_rnd (1, RND_MAX_TWEAK); 
  
  drumkit->rnd_set_rnd_seed (currentSamplePosition + randomizer_seed);

   //////////////////////
  // CACHE ALL ATOMIC PARAMETERS

  int int_midimap_mode = (int) *midimap_mode;
  
  mix_analog_amount = *(global_analog_amount);
  mix_analog_amount_on = *global_analog_on > 0.5f;
   
  float pan_sum = 0.0f;
  float vol_sum = 0.0f;
   
 
  for (size_t i = 0; i < 35; i++)
      {
       a_vols [i] = db2lin(*(vols[i]));
       a_mutes [i] = *(mutes[i]) > 0.5f;

       a_lps [i] = *(lps[i]) > 0.5f;
       a_lp_cutoff [i] = *(lp_cutoff[i]); 
       a_lp_reso [i] = *(lp_reso[i]);

       a_hps [i] = *(hps[i]) > 0.5f;
       a_hp_cutoff [i] = *(hp_cutoff[i]); 
       a_hp_reso [i] = *(hp_reso[i]);

       a_analog_on [i] = *(analog[i]);
       a_analog_amount [i] = *(analog_amount[i]);
      
 
       float pan = *(pans[i]); //float pan = *(pans[drum_sample_index]);
      
       vol_sum += a_vols[i];
       pan_sum += pan;
      
       int int_panner_mode = *panner_mode;
      
       if (int_panner_mode == PANMODE01)
          pan_sincos (a_pan_left[i], a_pan_right[i], pan);
       else
       if (int_panner_mode == PANMODE02)
           pan_sqrt (a_pan_left[i], a_pan_right[i], pan);
       else
       if (int_panner_mode == PANMODE03)
          pan_linear0 (a_pan_left[i], a_pan_right[i], pan);
       else
       if (int_panner_mode == PANMODE04)
          pan_linear6 (a_pan_left[i], a_pan_right[i], pan);
       else
       if (int_panner_mode == PANMODE05)
          pan_powsin_45 (a_pan_left[i], a_pan_right[i], pan); //ok
       else
       if (int_panner_mode == PANMODE06) // на деле - -4.5
          pan_powsin_6 (a_pan_left[i], a_pan_right[i], pan);
       else
       if (int_panner_mode == PANMODE07) //на деле -4.3
          pan_sin_1_3 (a_pan_left[i], a_pan_right[i], pan);
             
     } 
   
  ///////////////////
      
      
  for (const juce::MidiMessageMetadata metadata: midiMessages)
      {
        //  if (metadata.numBytes == 3)
            //Logger::writeToLog (metadata.getMessage().getDescription());
          // std::cout << metadata.getMessage().getDescription() << std::endl;

       juce::MidiMessage msg = metadata.getMessage();

       bool isNoteOn = msg.isNoteOn();
       bool isNoteOff = msg.isNoteOff();

       //float velocity = msg.getFloatVelocity();
       int uvelocity = msg.getVelocity();
       float velocity = VelocityToLevel (uvelocity);
       
       if (*ignore_midi_velocity > 0.5)
           velocity = 1;

       int note_number = msg.getNoteNumber(); //36 starting note

       if (isNoteOn)
          {
            //std::cout << "note_number: " << note_number << std::endl;
            //std::cout << "velocity: " << velocity << std::endl;

//          int nn = note_number - (int) *base_note_number;
            int nn = note_number - base_note_number;


            if (int_midimap_mode == MIDIMAPMODE_LABOOH)
                if (nn < 0 || nn > 35)
                    continue;

                    //std::cout << "nn < 0 || nn > 35: " << nn << std::endl; 
//             std::cout << "GO ON with n: " << nn << std::endl;

            float gn = db2lin (a_vols[nn]);
            // std::cout << "gn: " << gn << std::endl;

            CDrumSample *s = 0;

            if (int_midimap_mode == MIDIMAPMODE_LABOOH)
                s = drumkit->a_samples[nn];
            else
                if (drumkit->map_samples.count (note_number) > 0) 
                   s = drumkit->map_samples[note_number];
//                   std::cout << "play mapped note: " << note_number << std::endl;

            if (! s)
               continue;
  
            
           if (drumkit->kit_type == KIT_TYPE_HYDROGEN)
               s->trigger_sample (velocity);
           else //new
               if (drumkit->kit_type == KIT_TYPE_ALTDRUMLABOOH)
                  s->trigger_sample_uint_by_index (uvelocity, velocity); 
               else            
                   s->trigger_sample_uint (uvelocity, velocity);
                     

             //also untrigger open hihat if closed hihat triggering
             // so find the open hihat
             
            //AUTOMUTE START
  
           if (s->mute_group != -1)
              {
               for (size_t i = 0; i < MAX_SAMPLES; i++)
                   {
                    CDrumSample *s2 = drumkit->a_samples[i]; //point to the sample
                    if (! s2 || s == s2)
                       continue;
                      
                    if (s2->mute_group == s->mute_group)
                       {
                        if (drumkit->kit_type == KIT_TYPE_ALTDRUMLABOOH) 
                           s2->untrigger_sample (true);
                        else
                           s2->untrigger_sample (false);
                          
                       }
                   }
              }
          
             //automute end

           }

      }


    float *channel_data[2]; //output channels

    
    if (num_channels > 0)
       channel_data [0] = buffer.getWritePointer (0);

    if (num_channels > 1)
       channel_data [1] = buffer.getWritePointer (1);

 
   //for each sample out_buf_offs
    for (int out_buf_offs = 0; out_buf_offs < out_buf_length; out_buf_offs++)
        //for each drum instrument
        {
         for (int drum_sample_index = 0; drum_sample_index < 36; drum_sample_index++)
             {
              CDrumSample *s = drumkit->a_samples[drum_sample_index];

              if (! s)
                 // std::cout << "!s at drum_sample_index:" << drum_sample_index << std::endl;
                  continue;

              if (! s->active)
                 continue;

               
//             CDrumLayer *l = s->v_layers.at(s->current_layer);
              CDrumLayer *l = s->v_layers[s->current_layer];
  
              if (! l)
                 {
                  std::cout << "!l at s->current_layer:" << s->current_layer << std::endl;
                  break;
                 }


              if (l->sample_offset + 1 == l->length_in_samples)
                 {
                  if (drumkit->kit_type == KIT_TYPE_ALTDRUMLABOOH)  
                     s->untrigger_sample(true);
                  else
                      s->untrigger_sample(false);
                  
                  continue;
                 }

             //bool mute = *(mutes[drum_sample_index]) > 0.5f;

              if (a_mutes[drum_sample_index]) //просто не идем дальше, не играем
                 {
                  l->sample_offset++;
                  continue;
                 }

              if (l->channel_data)
                 {
                 //take mono audio data from the current layer with incremented offset
              //   float fl = l->channel_data[0][l->sample_offset++];
                  float fl = l->channel_data[l->sample_offset++];
              
                 //DSP


                  if (a_analog_on [drum_sample_index]) 
                     fl = warmify (fl, a_analog_amount[drum_sample_index]);
           
                // bool lp_on = *(lps[drum_sample_index]) > 0.5f;
                  bool lp_on = a_lps[drum_sample_index];

                   
                  if (lp_on)
                     {
                      lp[drum_sample_index].set_cutoff (a_lp_cutoff[drum_sample_index]);
                      lp[drum_sample_index].set_resonance (a_lp_reso[drum_sample_index]);

                      fl = softLimit (lp[drum_sample_index].process (fl));
                     }


//                 bool hp_on = *(hps[drum_sample_index]) > 0.5f;
                  bool hp_on = a_hps[drum_sample_index];

                  if (hp_on)
                     {
                      hp[drum_sample_index].set_cutoff (a_hp_cutoff[drum_sample_index]);
                      hp[drum_sample_index].set_resonance (a_hp_reso[drum_sample_index]);
 
                      fl = softLimit (hp[drum_sample_index].process (fl));
                     //fr = fl;
                     }

                 //AFTER DSP

//                 float vol = juce::Decibels::decibelsToGain ((float)*(vols[drum_sample_index]));
                  float vol = a_vols [drum_sample_index];

                  float coef_right = 0.000f;                    
                  float coef_left = 0.000f;  

                  if (s->layer_index_mode != LAYER_INDEX_MODE_NOVELOCITY)
                     {
                      coef_right = a_pan_right[drum_sample_index] * vol * s->velocity;
                      coef_left = a_pan_left[drum_sample_index] * vol * s->velocity;
                     }
                  else
                      {
                       coef_right = a_pan_right[drum_sample_index] * vol;
                       coef_left = a_pan_left[drum_sample_index] * vol;
                      }
                 
                  channel_data[0][out_buf_offs] += fl * coef_left;
                 //channel_data[1][out_buf_offs] += fr * coef_right;
                  channel_data[1][out_buf_offs] += fl * coef_right;
                 }
              }
              
              if (mix_analog_amount_on)
                 {
                  channel_data[0][out_buf_offs] = warmify (channel_data[0][out_buf_offs], mix_analog_amount);
                  channel_data[1][out_buf_offs] = warmify (channel_data[1][out_buf_offs], mix_analog_amount);
                 }
      
    }
 //std::cout << "CAudioProcessor::processBlock -6 " << std::endl;
}


#endif
