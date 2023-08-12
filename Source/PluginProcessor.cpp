/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "utl.h"


extern juce::AudioFormatManager *formatManager;

#define minusInfdB -60

juce::AudioProcessorValueTreeState::ParameterLayout CAudioProcessor::createParameterLayout()
{

  for (size_t i = 0; i < 36; i++)
     {
      pans[i] = nullptr;
      gains[i] = nullptr;
     }

  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add (std::make_unique<juce::AudioParameterInt> ("panmode",            // parameterID
                                                         "Pan mode",            // parameter name
                                                         0,              // minimum value
                                                         3,              // maximum value
                                                         0)); //default


  layout.add (std::make_unique<juce::AudioParameterInt> ("first_note_number",            // parameterID
                                                         "first_note_number",            // parameter name
                                                         0,              // minimum value
                                                         127,              // maximum value
                                                         36)); //default




  layout.add (std::make_unique<juce::AudioParameterFloat> ("gain", "Gain", 0.0f, 1.0f, 0.5f));
// layout.add (std::make_unique<juce::AudioParameterInt> ("gain", "Gain", 0.0f, 1.0f, 0.5f));


  //first_note_number

  layout.add (std::make_unique<juce::AudioParameterBool> ("invertPhase", "Invert Phase", false));

  //layout.add (std::make_unique<juce::AudioParameterFloat> ("gain0", "Gain0", minusInfdB, 6, 0));
  //layout.add (std::make_unique<juce::AudioParameterFloat> ("pan0", "Pan0", 0.0f, 1.0f, 0.5f));

  for (size_t i = 0; i < 36; i++)
      {
       layout.add (std::make_unique<juce::AudioParameterFloat> ("gain" + std::to_string(i), "gain" + std::to_string(i), minusInfdB, 6, 0));
       layout.add (std::make_unique<juce::AudioParameterFloat> ("pan" + std::to_string(i), "pan" + std::to_string(i), 0.0f, 1.0f, 0.5f));
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

  drumkit = 0;
  session_samplerate = 0;

  drumkit_path = "";




  //auto addons = treeState.state.getOrCreateChildWithName ("addons", nullptr);
  //auto text = addons.getProperty ("drumkit_name", juce::String()).toString();

  //std::cout << text << std::endl;

  //phaseParameter = parameters.getRawParameterValue ("invertPhase");
  //gainParameter  = parameters.getRawParameterValue ("gain");
//  gain0  = parameters.getRawParameterValue ("gain0");
//  pan0  = parameters.getRawParameterValue ("pan0");

 // gains[0]  = parameters.getRawParameterValue ("gain0");
 // pans[0]  = parameters.getRawParameterValue ("pan0");

  for (size_t i = 0; i < 36; i++)
      {
       gains[i]  = parameters.getRawParameterValue ("gain" + std::to_string(i));
       pans[i]  = parameters.getRawParameterValue ("pan" + std::to_string(i));

      }

  first_note_number  = parameters.getRawParameterValue ("first_note_number");
  std::cout << "first_note_number:" << *first_note_number << std::endl;


}

CAudioProcessor::~CAudioProcessor()
{
 // std::cout << "save drumkit_name: " << drumkit_name << std::endl;

   //save_string_keyval ("drumkit_name", drumkit_name);

   if (drumkit)
       delete drumkit;

   delete formatManager;
}

//==============================================================================
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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
     formatManager = new juce::AudioFormatManager();
     formatManager->registerBasicFormats();

      std::cout << "AudioProcessor::getSampleRate: << " <<  getSampleRate() << std::endl;

     session_samplerate = (int) sampleRate;

     load_kit (drumkit_path);

}


void CAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}


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


void CAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

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
            std::cout << "note_number: " << note_number << std::endl;
            std::cout << "velocity: " << velocity << std::endl;


            if (! drumkit)
               return;

            if (drumkit->v_samples.size() == 0)
               return;



          }

      }

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
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


//==============================================================================
void CAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    save_string_keyval ("drumkit_path", drumkit_path);


        auto state = parameters.copyState();
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);



}


void CAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
        std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName (parameters.state.getType()))
                parameters.replaceState (juce::ValueTree::fromXml (*xmlState));


             drumkit_path = load_string_keyval ("drumkit_path");

     std::cout << ">>>>>>>>>>>>drumkit_path: " << drumkit_path  << std::endl;

}



void CAudioProcessor::save_string_keyval (const std::string &key, const std::string &val)
{
  auto addons = parameters.state.getOrCreateChildWithName ("addons", nullptr);
  addons.setProperty (key.c_str(), val.c_str(), nullptr);

//  addons.setProperty ("drumkit_name", "val.c_str()", nullptr);


}


std::string CAudioProcessor::load_string_keyval (const std::string &key)
{
  //std::cout << "CAudioProcessor::load_string_keyval : " << key << std::endl;

  juce::Identifier keyid (key.c_str());

  auto addons = parameters.state.getOrCreateChildWithName ("addons", nullptr);
  //ret ValueTree

  //auto addons = parameters.state.getOrCreateChildWithName ("addons", nullptr);

  //auto text = addons.getProperty (key.c_str(), juce::String()).toString();

  //auto text = addons.getProperty ("drumkit_name", juce::String()).toString();

  auto text = addons.getProperty (keyid, juce::String()).toString();

  std::cout << "TEXT: " << text << std::endl;

  return text.toStdString();
}



bool CAudioProcessor::load_kit (const std::string &fullpath)
{

  std::cout << "CAudioProcessor::load_kit - 1" << std::endl;
//STOP PLAY

  suspendProcessing (true);

//THEN

   if (drumkit)
      delete drumkit;

    drumkit = new CHydrogenKit;
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


