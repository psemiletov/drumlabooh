/*
2023, Peter Semiletov
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "utl.h"
#include "dsp.h"




extern juce::AudioFormatManager *formatManager;

/*
static juce::NormalisableRange<float> get_cutoff_range()
{
    return {0.f, 0.999f, 0.001f, 0.999f};
}
*/



juce::AudioProcessorValueTreeState::ParameterLayout CAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  for (size_t i = 0; i < 36; i++)
      {
       pans[i] = nullptr;
       vols[i] = nullptr;
       mutes[i] = nullptr;

       lps[i] = nullptr;
       hps[i] = nullptr;

       lp_cutoff[i] = nullptr;
       lp_reso[i] = nullptr;

       hp_cutoff[i] = nullptr;
       hp_reso[i] = nullptr;

       analog[i] = nullptr;
       analog_amount[i] = nullptr;


      }


  layout.add (std::make_unique<juce::AudioParameterFloat> ("panner_mode",            // parameterID
                                                           "panner_mode",            // parameter name
                                                            1.0f,              // minimum value
                                                            4.0f,              // maximum value
                                                            1.0f)); //default

  layout.add (std::make_unique<juce::AudioParameterFloat> ("ignore_midi_velocity",      // parameterID
                                                           "ignore_midi_velocity",     // parameter name
                                                            0, 1, 0));



  for (size_t i = 0; i < 36; i++)
      {
       layout.add (std::make_unique <juce::AudioParameterFloat> ("vol" + std::to_string(i), // parameter ID
                                                                 "vol" + std::to_string(i), // parameter name
                                                                  juce::NormalisableRange<float> (-96, 6, 0.1, 5), // parameter range
                                                                  0)); //default value

       layout.add (std::make_unique<juce::AudioParameterFloat> ("pan" + std::to_string(i), "pan" + std::to_string(i), 0.0f, 1.0f, 0.5f));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("mute" + std::to_string(i),      // parameterID
                                                                "mute" + std::to_string(i),     // parameter name
                                                                0, 1, 0));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("lp" + std::to_string(i),      // parameterID
                                                                "lp" + std::to_string(i),     // parameter name
                                                                0, 1, 0));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("hp" + std::to_string(i),      // parameterID
                                                                "hp" + std::to_string(i),     // parameter name
                                                                0, 1, 0));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("analog" + std::to_string(i),      // parameterID
                                                                "analog" + std::to_string(i),     // parameter name
                                                                0, 1, 0));

/*
        layout.add (std::make_unique<juce::AudioParameterFloat> ("lp_cutoff" + std::to_string(i),
                                                                  "lp_cutoff" + std::to_string(i),
                                                                  juce::NormalisableRange<float> (1, 22100.0f, 1.0f), // parameter range
                                                                  48000.0f));


*/


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

  return layout;
}



CAudioProcessor::CAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
parameters (*this, 0, "Drumlabooh", createParameterLayout())
{

//std::cout << "CAudioProcessor::CAudioProcessor() - 1" << std::endl;


  formatManager = new juce::AudioFormatManager();
  formatManager->registerBasicFormats();


  init_db();

  fresh_start = true;
  drumkit = 0;
  drumkit_path = "";
  base_note_number = 36;
  session_samplerate = 0;

  for (size_t i = 0; i < 36; i++)
      {
       vols[i]  = parameters.getRawParameterValue ("vol" + std::to_string(i));
       pans[i]  = parameters.getRawParameterValue ("pan" + std::to_string(i));
       mutes[i]  = parameters.getRawParameterValue ("mute" + std::to_string(i));

       lps[i]  = parameters.getRawParameterValue ("lp" + std::to_string(i));
       lp_cutoff[i]  = parameters.getRawParameterValue ("lp_cutoff" + std::to_string(i));
       lp_reso[i]  = parameters.getRawParameterValue ("lp_reso" + std::to_string(i));

       hps[i]  = parameters.getRawParameterValue ("hp" + std::to_string(i));
       hp_cutoff[i]  = parameters.getRawParameterValue ("hp_cutoff" + std::to_string(i));
       hp_reso[i]  = parameters.getRawParameterValue ("hp_reso" + std::to_string(i));

       analog[i] = parameters.getRawParameterValue ("analog" + std::to_string(i));
       analog_amount[i] = parameters.getRawParameterValue ("analog_amount" + std::to_string(i));


      }

  panner_mode = parameters.getRawParameterValue ("panner_mode");
  ignore_midi_velocity = parameters.getRawParameterValue ("ignore_midi_velocity");
}


CAudioProcessor::~CAudioProcessor()
{
  if (drumkit)
      delete drumkit;

  delete formatManager;

}


const juce::String CAudioProcessor::getName() const
{
  return JucePlugin_Name;
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
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
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
//  std::cout << "CAudioProcessor::prepareToPlay - 1" << std::endl;
//  std::cout << "sampleRate: " <<  sampleRate << std::endl;


   session_samplerate = (int) sampleRate;

   //RELOAD IF SRATE CHANGED DURING SESSION, AS REAPER CAN
   if (drumkit)
      if (drumkit->samplerate != session_samplerate)
         load_kit (drumkit_path);

   //std::cout << "CAudioProcessor::prepareToPlay - 2" << std::endl;
}


void CAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

/*
#ifndef JucePlugin_PreferredChannelConfigurations
bool CAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif
*/


bool CAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
      return true;
  else
      return false;
}

// Map MIDI velocity 0-127 onto gain
// 0.0-1.0

#define VSENSE 1.0f
//#define VSENSE 0.0


float VelocityToLevel (int velocity)
{
  float min = 0.1f;
// velocity should logarithmically map onto [min..1]

  float logrange = logf( 1.0f / min );
  float vcurve = powf ((float (velocity-1) / 126.0f), 0.8f );

  return min * expf( logrange * vcurve );
}


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
          //return;
         }
     }


  if (! drumkit)
      return;


  //dst.fs = 44100;
 // shaper.fs_ = getSampleRate();

  size_t v_samples_size = drumkit->v_samples.size();

  if (v_samples_size == 0)
      return;

  for (const juce::MidiMessageMetadata metadata: midiMessages)
      {
        //  if (metadata.numBytes == 3)
            //Logger::writeToLog (metadata.getMessage().getDescription());
          // std::cout << metadata.getMessage().getDescription() << std::endl;

       juce::MidiMessage msg = metadata.getMessage();

       bool isNoteOn = msg.isNoteOn();
       bool isNoteOff = msg.isNoteOff();

       //float velocity = msg.getFloatVelocity();
       float velocity = VelocityToLevel (msg.getVelocity());

       if (*ignore_midi_velocity > 0.5)
           velocity = 1;

       int note_number = msg.getNoteNumber(); //36 starting note

       if (isNoteOn )
          {
            //std::cout << "note_number: " << note_number << std::endl;
            //std::cout << "velocity: " << velocity << std::endl;

//          int nn = note_number - (int) *base_note_number;
            int nn = note_number - base_note_number;

            if (nn < 0 || nn > v_samples_size - 1)
               {
  //              std::cout << "nn <> drumkit->v_samples.size(), nn is " << nn << std::endl;
                continue;
               }

//             std::cout << "GO ON with n: " << nn << std::endl;

            float gn = db2lin(*(vols[nn]));
            // std::cout << "gn: " << gn << std::endl;

            CDrumSample *s = drumkit->v_samples[nn];
            if (! s)
               continue;

            s->trigger_sample (velocity);

             //also untrigger open hihat if closed hihat triggering
             // so find the open hihat
            if (s->hihat_close)
               {
                for (size_t i = 0; i < v_samples_size; i++)
                    {
                     CDrumSample *s2 = drumkit->v_samples[i]; //point to the sample
                     if (s2->hihat_open)
                         s2->untrigger_sample();
                   }
               }
           }

      }


     //if (! drumkit)
        //return;

    // std::cout << "CAudioProcessor::processBlock -2 " << std::endl;


    float *channel_data [2];

    int num_channels = buffer.getNumChannels();

    if (num_channels > 0)
       channel_data [0] = buffer.getWritePointer (0);

    if (num_channels > 1)
       channel_data [1] = buffer.getWritePointer (1);


  //std::cout << "CAudioProcessor::processBlock -3 " << std::endl;

 //   juce::ScopedNoDenormals noDenormals;
//    auto totalNumInputChannels  = getTotalNumInputChannels();
    //auto totalNumOutputChannels = getTotalNumOutputChannels();

   //  std::cout << "buffer.getNumSamples():" << buffer.getNumSamples() << std::endl;


    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.


    int out_buf_length = buffer.getNumSamples();


    for (int i = 0; i < 2; ++i)
         buffer.clear (i, 0, out_buf_length);



   //for each sample out_buf_offs
    for (int out_buf_offs = 0; out_buf_offs < out_buf_length; out_buf_offs++)
        //for each drum instrument
        for (int drum_sample_index = 0; drum_sample_index < v_samples_size; drum_sample_index++)
            {
             CDrumSample *s = drumkit->v_samples[drum_sample_index];
             if (! s)
                {
                 std::cout << "!s at drum_sample_index:" << drum_sample_index << std::endl;
                 break;
                }

             bool mute = *(mutes[drum_sample_index]) > 0.5f;

             // std::cout << s->name << std::endl;

             if (! s->active)
                continue;

             CDrumLayer *l = s->v_layers[s->current_layer];

             if (! l)
                {
                 std::cout << "!l at s->current_layer:" << s->current_layer << std::endl;
                 break;
                }


             if (l->sample_offset + 1 == l->length_in_samples)
                {
                 s->untrigger_sample();
                 continue;
                }

             if (mute)
                {
                 continue;
                 l->sample_offset++;
                }

             if (l->channels == 1)
                {
                 float fl = l->channel_data[0][l->sample_offset++];
                 float fr = fl;


                 //DSP

/*
                 bool saturator_on = *(saturator[drum_sample_index]) > 0.5f;

                 if (saturator_on)
                    {
                     fl = guitarDistortion (fl, *(saturator_amount[drum_sample_index]));
                     fr = fl;

                    }


                 */


/*
                 bool saturator_on = *(saturator[drum_sample_index]) > 0.5f;

                 if (saturator_on)
                    {
                     fl = soft_saturate (fl,*(saturator_amount[drum_sample_index]));
                    fr = fl;

                    }

*/


               bool analog_on = *(analog[drum_sample_index]) > 0.5f;

                 if (analog_on)
                    {
                     //shaper.attackTime_ = *(saturator_amount[drum_sample_index]);
                     fl = warmify (fl,*(analog_amount[drum_sample_index]));

                     //fl = soft_saturate (fl,*(saturator_amount[drum_sample_index]));
                     fr = fl;

                    }


                 bool lp_on = *(lps[drum_sample_index]) > 0.5f;

                 if (lp_on)
                    {
                     lp[drum_sample_index].set_cutoff (*(lp_cutoff[drum_sample_index]));
                     lp[drum_sample_index].set_resonance (*(lp_reso[drum_sample_index]));

                     fl = lp[drum_sample_index].process (fl);
                     fr = fl;
                    }





                 bool hp_on = *(hps[drum_sample_index]) > 0.5f;

                 if (hp_on)
                    {
                     hp[drum_sample_index].set_cutoff (*(hp_cutoff[drum_sample_index]));


                     hp[drum_sample_index].set_resonance (*(hp_reso[drum_sample_index]));

                     fl = hp[drum_sample_index].process (fl);
                     fr = fl;
                    }



                 //AFTER DSP

//                 float vol = juce::Decibels::decibelsToGain ((float)*(vols[drum_sample_index]));
                 float vol = db2lin(*(vols[drum_sample_index]));

                 float pan_right = 0;
                 float pan_left = 0;

                 float pan = *(pans[drum_sample_index]);

                 if (*panner_mode == PANMODE01)
                      pan_sincos (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE02)
                     pan_sqrt (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE03)
                     pan_linear0 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE04)
                     pan_linear6 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE05)
                     pan_power45 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE06)
                     pan_power15 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE07)
                     pan_equal_power3 (pan_left, pan_right, pan);

                 float coef_right = pan_right * vol * s->velocity;
                 float coef_left = pan_left * vol * s->velocity;

                 channel_data[0][out_buf_offs] += fl * coef_left;
                 channel_data[1][out_buf_offs] += fl * coef_right;
                }
/*
             if (l->channels == 2)
                {
                 float fl = l->channel_data[0][l->sample_offset++];
                 float fr = l->channel_data[1][l->sample_offset];

//                 float gain = db2lin(*(gains[drum_sample_index]));
//                 float vol = juce::Decibels::decibelsToGain ((float)*(vols[drum_sample_index]));
                 float vol = db2lin(*(vols[drum_sample_index]));

                 float pan_right = 0;
                 float pan_left = 0;

                 float pan = *(pans[drum_sample_index]);

                 if (*panner_mode == PANMODE01)
                     pan_sincos (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE02)
                     pan_sqrt (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE03)
                     pan_linear0 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE04)
                     pan_linear6 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE05)
                     pan_power45 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE06)
                     pan_power15 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == PANMODE07)
                     pan_equal_power3 (pan_left, pan_right, pan);


                 float coef_right = pan_right * vol * s->velocity;
                 float coef_left = pan_left * vol * s->velocity;

                 channel_data[0][out_buf_offs] += fl * coef_left;
                 channel_data[1][out_buf_offs] += fr * coef_right;
                }*/
             }

 //std::cout << "CAudioProcessor::processBlock -6 " << std::endl;
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
  save_string_keyval ("drumkit_path", drumkit_path);
  save_int_keyval ("base_note_number", base_note_number);

//  std::cout << "get int_base_note_number: " << base_note_number << std::endl;

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
         drumkit_path = load_string_keyval ("drumkit_path");
        // std::cout << "drumkit_path : " << drumkit_path  << std::endl;
         //std::cout << "set base_note_number: " << _base_note_number << std::endl;
         //std::cout << "AudioProcessor::getSampleRate: << " <<  getSampleRate() << std::endl;
         session_samplerate = getSampleRate();
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
  //std::cout << "CAudioProcessor::load_kit: " << fullpath << std::endl;

  //std::cout << "CAudioProcessor::load_kit - 1" << std::endl;
  //std::cout << "fullpath: " << fullpath << std::endl;
  //std::cout << "session samplerate: " << session_samplerate << std::endl;

    if (fullpath.empty())
       return false;

//STOP PLAY

  suspendProcessing (true);

//THEN

  if (drumkit)
      delete drumkit;

  drumkit = new CDrumKit;
  drumkit->load (fullpath, session_samplerate);

  for (size_t i = 0; i < 36; i++)
      {
       lp[i].reset();
       hp[i].reset();
       hp[i].mode = FILTER_MODE_HIGHPASS;
       hp[i].reset();
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
