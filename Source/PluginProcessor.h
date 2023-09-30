#pragma once

#include <JuceHeader.h>

//nclude "melatonin_perfetto/melatonin_perfetto.h"

#include "kits.h"
#include "dsp.h"
#include "fx-resofilter.h"



class CTransientShaper
{
public:

    float attackTime_;     // Время нарастания атаки в секундах
    float releaseTime_;    // Время спада в секундах
    float envelope_;       // Значение огибающей амплитуды
    float fs_ = 44100.0;   // Частота дискретизации, можно изменить на нужную


    CTransientShaper (float attackTime = 0.001f, float releaseTime = 0.1f)
    {
      attackTime_ = attackTime;
      releaseTime_ = releaseTime;
      envelope_ = 0.0;
    }

    // Функция обработки атаки входного сэмпла
    float processAttack(float input) {
        // Вычисляем изменение амплитуды атаки
        float attackDelta = (1.0 - envelope_) / (attackTime_ * fs_);

        // Акцентируем атаку, увеличивая амплитуду
        envelope_ += attackDelta * input;

        // Ограничиваем амплитуду атаки в пределах [0, 1]
        envelope_ = myclamp(envelope_, 0.0f, 1.0f);

        // Применяем изменение амплитуды к входному сэмплу
        return envelope_ * input;
    }

    // Функция обработки спада входного сэмпла
    float processRelease(float input) {
        // Вычисляем изменение амплитуды спада
        float releaseDelta = envelope_ / (releaseTime_ * fs_);

        // Уменьшаем амплитуду спада
        envelope_ -= releaseDelta * envelope_;

        // Ограничиваем амплитуду спада в пределах [0, 1]
        envelope_ = myclamp(envelope_, 0.0f, 1.0f);

        // Применяем изменение амплитуды к входному сэмплу
        return envelope_ * input;
    }


    template <typename T>
    T myclamp(const T& value, const T& min, const T& max) {
        if (value < min) {
            return min;
        } else if (value > max) {
            return max;
        } else {
            return value;
        }
    }
};


class CAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif

{
public:

    juce::AudioProcessorValueTreeState parameters;

    std::atomic<float>* pans[36];
    std::atomic<float>* vols[36];
    std::atomic<float>* mutes[36]; //bool

    std::atomic<float>* lps[36]; //bool
    std::atomic<float>* hps[36]; //bool
    std::atomic<float>* analog[36]; //bool


    std::atomic<float>* lp_cutoff[36]; //
    std::atomic<float>* lp_reso[36]; //

    std::atomic<float>* hp_cutoff[36]; //
    std::atomic<float>* hp_reso[36]; //

    std::atomic<float>* analog_amount[36]; //



    CResoFilter lp[36];
    CResoFilter hp[36];

//    CTransientShaper shaper;
  //  DistortionProcessor dst;

    std::atomic<float>* panner_mode = nullptr;
    std::atomic<float>* ignore_midi_velocity = nullptr;

    int base_note_number;

    bool fresh_start;

    std::string drumkit_path; //full path!

    CDrumKitsScanner scanner;
    CDrumKit *drumkit;

    int session_samplerate; //sess rate

    CAudioProcessor();
    ~CAudioProcessor() override;


    bool load_kit (const std::string &fullpath);
    void save_string_keyval (const std::string &key, const std::string &val);
    std::string load_string_keyval (const std::string &key);

    void save_int_keyval (const std::string &key, int val);
    int load_int_keyval (const std::string &key, int defval);


    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;


    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CAudioProcessor)
};
