/*
 w ritten at 2026 by Peter Semiletov
 this code is the public domain
 */

#pragma once

#include <string>
#include <juce_audio_basics/juce_audio_basics.h>

bool is_synth_formula (const std::string &str);
juce::AudioBuffer<float>* render_synth_sample (const std::string &filename, int session_samplerate, bool is_filename = true);
void init_synth_wavetables();
