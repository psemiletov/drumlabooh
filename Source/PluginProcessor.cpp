/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "utl.h"
#include "dsp.h"


extern juce::AudioFormatManager *formatManager;



juce::AudioProcessorValueTreeState::ParameterLayout CAudioProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  for (size_t i = 0; i < 36; i++)
      {
       pans[i] = nullptr;
       gains[i] = nullptr;
       mutes[i] = nullptr;
      }


  layout.add (std::make_unique<juce::AudioParameterFloat> ("panner_mode",            // parameterID
                                                           "panner_mode",            // parameter name
                                                            1.0f,              // minimum value
                                                            4.0f,              // maximum value
                                                            1.0f)); //default


  /*
   layout.add (std::make_unique <juce::AudioParameterFloat> ("base_note_number", // parameter ID
                                                             "base_note_number", // parameter name
                                                              juce::NormalisableRange<float> (0, 127, 1), // parameter range
                                                              36));
*/

/*
  layout.add (std::make_unique<juce::AudioParameterFloat> ("base_note_number",            // parameterID
                                                           "base_note_number",            // parameter name
                                                            0.0f,              // minimum value
                                                            127.0f,              // maximum value
                                                            36.0f)); //default

*/
  //layout.add (std::make_unique<juce::AudioParameterBool> ("invertPhase", "Invert Phase", false));

  for (size_t i = 0; i < 36; i++)
      {
       layout.add (std::make_unique <juce::AudioParameterFloat> ("gain" + std::to_string(i), // parameter ID
                                                                 "gain" + std::to_string(i), // parameter name
                                                                  juce::NormalisableRange<float> (-96, 6, 0.1, 5), // parameter range
                                                                  0));

       layout.add (std::make_unique<juce::AudioParameterFloat> ("pan" + std::to_string(i), "pan" + std::to_string(i), 0.0f, 1.0f, 0.5f));

/*
       layout.add (std::make_unique<juce::AudioParameterBool> ("mute" + std::to_string(i),      // parameterID
                                                               "mute" + std::to_string(i),     // parameter name
                                                                false));
*/
       layout.add (std::make_unique<juce::AudioParameterFloat> ("mute" + std::to_string(i),      // parameterID
                                                               "mute" + std::to_string(i),     // parameter name
                                                                0, 1, 0));

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
parameters (*this, 0, "Drumpecker", createParameterLayout())
{
   formatManager = new juce::AudioFormatManager();
   formatManager->registerBasicFormats();


  init_db();

  drumkit = 0;
  session_samplerate = 0;
  drumkit_path = "";

  int_base_note_number = 36;

  for (size_t i = 0; i < 36; i++)
      {
       gains[i]  = parameters.getRawParameterValue ("gain" + std::to_string(i));
       pans[i]  = parameters.getRawParameterValue ("pan" + std::to_string(i));
       mutes[i]  = parameters.getRawParameterValue ("mute" + std::to_string(i));

      }

  panner_mode = parameters.getRawParameterValue ("panner_mode");
 // base_note_number  = parameters.getRawParameterValue ("base_note_number");
//  std::cout << "base_note_number:" << *base_note_number << std::endl;
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
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}


bool CAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
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

   std::cout << "CAudioProcessor::prepareToPlay - 1" << std::endl;

    std::cout << "sampleRate: " <<  sampleRate << std::endl;

  //    std::cout << "*audioProcessor.panner_mode: "  << *panner_mode << std::endl;

     //std::cout << "base_note_number:" << *base_note_number << std::endl;

   session_samplerate = (int) sampleRate;

   std::cout << "CAudioProcessor::prepareToPlay - 2" << std::endl;

   //if (! drumkit)
     // load_kit (drumkit_path);
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



void CAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

//std::cout << "CAudioProcessor::processBlock -1 " << std::endl;



 //if (base_note_number == nullptr)
   //  return;

  if (fresh_start)
     {
      session_samplerate = getSampleRate();

      if (! drumkit_path.empty())
         load_kit (drumkit_path);

      fresh_start = false;
     }

  for (const juce::MidiMessageMetadata metadata : midiMessages)
      {
        //  if (metadata.numBytes == 3)
            //Logger::writeToLog (metadata.getMessage().getDescription());
          // std::cout << metadata.getMessage().getDescription() << std::endl;

        juce::MidiMessage msg = metadata.getMessage();

        bool isNoteOn = msg.isNoteOn();
        bool isNoteOff = msg.isNoteOff();
        float velocity = msg.getFloatVelocity ();

        int note_number = msg.getNoteNumber(); //36 starting note

        if (isNoteOn )
           {
            //std::cout << "note_number: " << note_number << std::endl;
            //std::cout << "velocity: " << velocity << std::endl;

            if (! drumkit)
               return;

            if (drumkit->v_samples.size() == 0)
               return;


//             int nn = note_number - (int) *base_note_number;
             int nn = note_number - int_base_note_number;

             if (nn < 0 || nn > drumkit->v_samples.size() - 1)
                {
                 std::cout << "nn <> drumkit->v_samples.size(), nn is " << nn << std::endl;
                 continue;
                }

//             std::cout << "GO ON with n: " << nn << std::endl;

             float gn = db2lin(*(gains[nn]));
            // std::cout << "gn: " << gn << std::endl;

             CDrumSample *s = drumkit->v_samples [nn];
             if (! s)
                continue;


            // std::cout << "s->v_layers[0]->lengthInSamples: " << s->v_layers[0]->length_in_samples << std::endl;
             //std::cout << "s->v_layers[0]->channels: " << s->v_layers[0]->channels << std::endl;


             s->trigger_sample (velocity);

             //also untrigger open hihat if closed hihat triggering
             // so find the open hihat
            if (s->hihat_close)
               {
                for (size_t i = 0; i < drumkit->v_samples.size(); i++)
                    {
                     CDrumSample *s2 = drumkit->v_samples[i]; //point to the sample
                     if (s2->hihat_open)
                         s2->untrigger_sample();
                   }
               }
           }

      }


     if (! drumkit)
        return;

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

    //for (int i = 0; i < num_channels; ++i)
     for (int i = 0; i < 2; ++i)
         buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
      //  auto* channelData = buffer.getWritePointer (channel);

         //std::cout << "channel: " << channel << std::endl;

//    std::cout << "CAudioProcessor::processBlock -4 " << std::endl;


        // ..do something to the data...
   int out_buf_length = buffer.getNumSamples();


  //   std::cout << "CAudioProcessor::processBlock -5 " << std::endl;

    // std::cout << "drumkit->v_samples.size(): " << drumkit->v_samples.size();

   //for each sample out_buf_offs
    for (int out_buf_offs = 0; out_buf_offs < out_buf_length; out_buf_offs++)
        //for each drum instrument
        for (int drum_sample_index = 0; drum_sample_index < drumkit->v_samples.size(); drum_sample_index++)
            {
             CDrumSample *s = drumkit->v_samples [drum_sample_index];
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

             if (l->sample_offset == l->length_in_samples)
                {
                 s->untrigger_sample();
                 continue;
                }



             l->sample_offset++;

             if (mute)
                continue;


             if (l->channels == 1)
                {
                 float fl = l->channel_data[0][l->sample_offset];
                 float fr = l->channel_data[0][l->sample_offset];

                 float gain = db2lin(*(gains[drum_sample_index]));

                 float pan_right = 0;
                 float pan_left = 0;

                 float pan = *(pans[drum_sample_index]);

                 if (*panner_mode == 1)
                      pan_sincos (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == 2)
                     pan_sqrt (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == 3)
                     pan_linear0 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == 4)
                     pan_linear6 (pan_left, pan_right, pan);


                 float coef_right = pan_right * gain * s->velocity;
                 float coef_left = pan_left * gain * s->velocity;

                 channel_data[0][out_buf_offs] += fl * coef_left;
                 channel_data[1][out_buf_offs] += fl * coef_right;

                }

             if (l->channels == 2)
                {

                 float fl = l->channel_data[0][l->sample_offset];
                 float fr = l->channel_data[1][l->sample_offset];

                 float gain = db2lin(*(gains[drum_sample_index]));

                 float pan_right = 0;
                 float pan_left = 0;

                 float pan = *(pans[drum_sample_index]);

                 if (*panner_mode == 1)
                      pan_sincos (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == 2)
                     pan_sqrt (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == 3)
                     pan_linear0 (pan_left, pan_right, pan);
                 else
                 if (*panner_mode == 4)
                     pan_linear6 (pan_left, pan_right, pan);


                 float coef_right = pan_right * gain * s->velocity;
                 float coef_left = pan_left * gain * s->velocity;

                 channel_data[0][out_buf_offs] += fl * coef_left;
                 channel_data[1][out_buf_offs] += fr * coef_right;

                }

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
  save_int_keyval ("int_base_note_number", int_base_note_number);

  std::cout << "get int_base_note_number: " << int_base_note_number << std::endl;

  auto state = parameters.copyState();
  std::unique_ptr <juce::XmlElement> xml (state.createXml());
  copyXmlToBinary (*xml, destData);
}


//load
void CAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
  std::cout << "CAudioProcessor::setStateInformation - 1" << std::endl;

  std::unique_ptr <juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

  if (xmlState.get() != nullptr)
     if (xmlState->hasTagName (parameters.state.getType()))

     {
      parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
      int_base_note_number = load_int_keyval ("int_base_note_number", 36);
      drumkit_path = load_string_keyval ("drumkit_path");
      std::cout << "drumkit_path : " << drumkit_path  << std::endl;
      std::cout << "set int_base_note_number: " << int_base_note_number << std::endl;
      std::cout << "AudioProcessor::getSampleRate: << " <<  getSampleRate() << std::endl;
      session_samplerate = getSampleRate();
     // load_kit (drumkit_path);
     }
//  std::cout << ">>>>>>>>>>>>drumkit_path: " << drumkit_path  << std::endl;

  std::cout << "CAudioProcessor::setStateInformation - 2" << std::endl;

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
  //std::cout << "int CAudioProcessor::load_int_keyval (const std::string &key, int defval)  -1" <<  std::endl;


  juce::Identifier keyid (key.c_str());
  auto addons = parameters.state.getOrCreateChildWithName ("addons", nullptr);
  auto v = addons.getProperty (keyid);

//std::cout << "int CAudioProcessor::load_int_keyval (const std::string &key, int defval)  -2" <<  std::endl;


  if (v.isVoid())
     return defval;

//  std::cout << "int CAudioProcessor::load_int_keyval (const std::string &key, int defval)  -3" <<  std::endl;


 // std::cout << "TEXT: " << v.toString() << std::endl;
  return v;
}



bool CAudioProcessor::load_kit (const std::string &fullpath)
{

  std::cout << "CAudioProcessor::load_kit - 1" << std::endl;
  std::cout << "fullpath: " << fullpath << std::endl;

    if (fullpath.empty())
       return false;

//STOP PLAY

  suspendProcessing (true);

//THEN

  if (drumkit)
      delete drumkit;

  drumkit = new CDrumKit;
  drumkit->load (fullpath, session_samplerate);

//SIGNAL TO

 suspendProcessing (false);

 std::cout << "CAudioProcessor::load_kit - 2" << std::endl;

  return true;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CAudioProcessor();
}
