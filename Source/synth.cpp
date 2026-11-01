/*
 * written at 2026 by Peter Semiletov with Gemini AI
 * this code is the public domain
 */

#include "synth.h"
#include <sstream>
#include <vector>
#include <cmath>
#include <map>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <string>
#include <map>

#include <juce_audio_basics/juce_audio_basics.h>

#include "utl.h"

constexpr int kSineTableSize = 2048;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

float g_sine_table[kSineTableSize + 1];



void init_synth_wavetables()
{
  for (int i = 0; i <= kSineTableSize; ++i)
      {
       g_sine_table[i] = std::sin(static_cast<float>(i) * (kTwoPi / static_cast<float>(kSineTableSize)));
      }
}


inline float fast_sin (float phase)
{
  phase -= static_cast<int>(phase);

  if (phase < 0.0f)
      phase += 1.0f;

  float idx_f = phase * static_cast<float>(kSineTableSize);
  int idx = static_cast<int>(idx_f);
  float frac = idx_f - static_cast<float>(idx);

  return g_sine_table[idx] + frac * (g_sine_table[idx + 1] - g_sine_table[idx]);
}


void apply_buffer_fadeout(juce::AudioBuffer<float>* buffer, int samplerate, float fade_duration_sec = 0.003f)
{
  if (! buffer)
     return;

  int num_samples = buffer->getNumSamples();
  int fade_samples = static_cast<int>(samplerate * fade_duration_sec);

  if (fade_samples > num_samples / 2)
      fade_samples = num_samples / 2;

  if (fade_samples < 2)
     fade_samples = 2;

  float* data = buffer->getWritePointer(0);
  int start_fade_idx = num_samples - fade_samples;

  for (int i = 0; i < fade_samples; ++i)
      {
       float coeff = static_cast<float>(fade_samples - i) / static_cast<float>(fade_samples);
       data[start_fade_idx + i] *= coeff;
      }
}


std::map<std::string, float> parse_synth_params (const std::string& formula_str)
{
  std::map<std::string, float> params;

  size_t start = formula_str.find('(');
  size_t end   = formula_str.rfind(')');

  if (start == std::string::npos || end == std::string::npos || end <= start)
     return params;

  std::string inner = formula_str.substr(start + 1, end - start - 1);
  std::stringstream ss(inner);
  std::string item;

   while (std::getline(ss, item, ','))
         {
          item = trim(item);

          if (item.empty())
              continue;

          size_t eq = item.find('=');
          if (eq == std::string::npos)
              continue;

          std::string key = item.substr(0, eq);
          std::string val_str = item.substr(eq + 1);

          key = trim(key);
          val_str = trim(val_str);

          std::transform (key.begin(), key.end(), key.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

          try
             {
              float val = std::stof(val_str);
              params[key] = val;
             }
          catch (...)
        {
        }
    }

  return params;
}


inline float get_deterministic_noise (uint32_t& state)
{
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return static_cast<float>(state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;
}


class Biquad
{
public:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void bandpass(float sr, float f0, float Q)
    {
        float w0 = 2.0f * kPi * f0 / sr;
        float cosw = std::cos(w0), sinw = std::sin(w0);
        float alpha = sinw / (2.0f * Q);
        float a0 = 1.0f + alpha;
        b0 =  alpha / a0;
        b1 =  0.0f;
        b2 = -alpha / a0;
        a1 = -2.0f * cosw / a0;
        a2 = (1.0f - alpha) / a0;
    }

    float process(float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};


juce::AudioBuffer<float>* render_boom (const std::string& formula_str, int samplerate)
{
  float duration   = 0.35f;
  float base_freq  = 70.0f;
  float freq_sweep = -40.0f;
  float noise_amt  = 0.3f;
  float roughness  = 0.6f;
  float wobble_rate = 24.0f;
  float attack = 0.001f;
  float release = 0.12f;
  float gain = 0.95f;
  float bubble_amt = 0.5f;

  float lpf_cutoff = 1400.0f;
  float hpf_cutoff = 45.0f;
  bool granular = true;
  float grain_mix = 0.4f;

  std::map<std::string, float> p = parse_synth_params(formula_str);

  if (p.count("duration"))  duration   = p["duration"];
  if (p.count("freq"))      base_freq  = p["freq"];
  if (p.count("sweep"))     freq_sweep = p["sweep"];
  if (p.count("noise"))     noise_amt  = p["noise"];
  if (p.count("roughness")) roughness  = p["roughness"];
  if (p.count("wobble"))    wobble_rate = p["wobble"];
  if (p.count("attack"))    attack     = p["attack"];
  if (p.count("release"))   release    = p["release"];
  if (p.count("gain"))      gain       = p["gain"];
  if (p.count("bubble"))    bubble_amt = p["bubble"];
  if (p.count("tone"))      lpf_cutoff = p["tone"];
  if (p.count("granular"))  granular   = (p["granular"] > 0.5f);
  if (p.count("grain_mix")) grain_mix  = p["grain_mix"];

  int num_samples = static_cast<int>(samplerate * (duration * 1.1f));
  if (num_samples < 100)
      num_samples = 100;

  juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
  float* data = buffer->getWritePointer(0);

  uint32_t noise_state = 13579;
  double phase1 = 0.0, phase2 = 0.0;
  float wobblePhase = 0.0f;
  float bubblePhase = 0.0f;

  float lpf_state = 0.0f;
  float hpf_prev_in = 0.0f;
  float hpf_prev_out = 0.0f;

  const float dt = 1.0f / static_cast<float>(samplerate);

  float rc_lpf = 1.0f / (kTwoPi * lpf_cutoff);
  float alpha_lpf = dt / (rc_lpf + dt);

  float rc_hpf = 1.0f / (kTwoPi * hpf_cutoff);
  float alpha_hpf = rc_hpf / (rc_hpf + dt);

  float micro_burst_env = 0.0f;
  float micro_burst_phase = 0.0f;
  int micro_burst_timer = 0;
  int next_burst_samples = static_cast<int>(samplerate * 0.02f);

  const float body_env_factor = std::exp(-dt / release);
  const float noise_env_factor = std::exp(-dt / (release * 0.5f));
  float body_env = 1.0f;
  float noise_env = 1.0f;

  const float wobble_phase_inc = kTwoPi * wobble_rate * dt;
  const float bubble_phase_inc = kTwoPi * 14.0f * dt;
  const float micro_burst_freq_inc = kTwoPi * (base_freq * 1.6f) * dt;

  for (int i = 0; i < num_samples; ++i)
      {
       float t = static_cast<float>(i) * dt;
       float u = t / duration;

       if (t >= attack)
          {
           body_env *= body_env_factor;
           noise_env *= noise_env_factor;
          }
       else
           {
            body_env = t / attack;
            noise_env = t / attack;
          }

       float kick_drop = std::exp(-t * 70.0f);
       float freq1 = base_freq + (base_freq * 3.0f * kick_drop) + freq_sweep * u;
       float freq2 = base_freq * 0.5f + freq_sweep * 0.8f * u;

       float wobble = std::sin(wobblePhase) * roughness * 0.7f;
       wobblePhase += wobble_phase_inc;
       freq1 *= (1.0f + wobble);

       phase1 += static_cast<double>(kTwoPi) * freq1 * dt;
       phase2 += static_cast<double>(kTwoPi) * freq2 * dt;

       if (phase1 > kTwoPi)
           phase1 -= kTwoPi;

       if (phase2 > kTwoPi)
           phase2 -= kTwoPi;

       float tone_sig = std::sin(static_cast<float>(phase1)) * 0.75f + (std::sin(static_cast<float>(phase2 * 2.0f)) * 0.25f);

       float bubble = 1.0f + bubble_amt * std::sin(bubblePhase) * std::cos(bubblePhase * 0.4f);
       bubblePhase += bubble_phase_inc;

       if (bubblePhase > kTwoPi)
           bubblePhase -= kTwoPi;

       float raw_noise = get_deterministic_noise(noise_state);

       float granular_sig = 0.0f;
       if (granular)
          {
           micro_burst_timer++;

           if (micro_burst_timer >= next_burst_samples && u < 0.75f)
              {
               micro_burst_env = 1.0f;
               micro_burst_timer = 0;
               float jitter = get_deterministic_noise(noise_state);
               next_burst_samples = static_cast<int>(samplerate * (0.012f + std::abs(jitter) * 0.015f));
              }

           if (micro_burst_env > 0.001f)
              {
               micro_burst_phase += micro_burst_freq_inc;

               if (micro_burst_phase > kTwoPi)
                   micro_burst_phase -= kTwoPi;

               granular_sig = std::sin(micro_burst_phase) * micro_burst_env;
               micro_burst_env *= 0.993f;
              }
          }

       float click = 0.0f;

       if (t < 0.0025f)
            click = raw_noise * (1.0f - (t / 0.0025f)) * 0.85f;

       float body = (tone_sig * (1.0f - noise_amt) + (raw_noise * noise_env * noise_amt)) + click;

       if (granular)
          body = body * (1.0f - grain_mix) + (granular_sig * noise_env * grain_mix);

       body *= body_env * bubble * gain;

       lpf_state = lpf_state + alpha_lpf * (body - lpf_state);

       float hpf_out = alpha_hpf * (hpf_prev_out + lpf_state - hpf_prev_in);
       hpf_prev_in = lpf_state;
       hpf_prev_out = hpf_out;

       float s = std::tanh(hpf_out * 2.2f);
       data[i] = s;
      }

  apply_buffer_fadeout(buffer, samplerate, 0.003f);
  return buffer;
}


juce::AudioBuffer<float>* render_gabber_kick(const std::string& formula_str, int samplerate)
{
    float f0 = 220.0f;
    float f1 = 42.0f;
    float decay = 0.38f;
    float drive = 0.8f;
    float tone = 500.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("f0"))    f0 = p["f0"];
    if (p.count("f1"))    f1 = p["f1"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("drive")) drive = p["drive"];
    if (p.count("tone"))  tone = p["tone"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;

    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (kTwoPi * tone);
    float alpha = dt / (rc + dt);

    const float env_factor = std::exp(-dt * (4.0f / decay));
    float env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = f1 + (f0 - f1) * std::exp(-t * 32.0f);
        phase += kTwoPi * freq * dt;

        float raw_sine = std::sin(phase);

        float gain = 1.0f + (drive * 12.0f) * std::exp(-t * 15.0f);
        float distorted = std::tanh(raw_sine * gain);

        lpf_state = lpf_state + alpha * (distorted - lpf_state);

        float mixed = (raw_sine * 0.3f) + (lpf_state * 0.7f);

        env *= env_factor;
        float sample = mixed * env;

        sample = std::tanh(sample * 1.1f);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_computer_snare(const std::string& formula_str, int samplerate)
{
    float freq = 210.0f;
    float decay = 0.2f;
    float snap = 0.5f;
    float tone = 1200.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("freq"))  freq = p["freq"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("snap"))  snap = p["snap"];
    if (p.count("tone"))  tone = p["tone"];

    if (tone < 200.0f) tone = 200.0f;
    if (tone > samplerate * 0.45f) tone = samplerate * 0.45f;

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    if (!buffer) return nullptr;
    float* data = buffer->getWritePointer(0);

    double phase1 = 0.0;
    double phase2 = 0.0;
    uint32_t noise_state = 0x12345678u;

    const float dt = 1.0f / static_cast<float>(samplerate);

    float f1 = freq;
    float f2 = freq * 1.48f;

    float noise_lpf_state = 0.0f;
    float noise_rc = 1.0f / (kTwoPi * 1200.0f);
    float noise_alpha = dt / (noise_rc + dt);

    float master_lpf_state = 0.0f;
    float master_rc = 1.0f / (kTwoPi * tone);
    float master_alpha = dt / (master_rc + dt);

    const float body_env_factor = std::exp(-dt * (4.5f / decay));
    float body_env = 1.0f;

    const float noise_env_factor = std::exp(-dt * (6.0f / decay));
    float noise_env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float pitch_env = std::exp(-t * 35.0f);
        float current_f1 = f1 + (f1 * 0.5f) * pitch_env;
        float current_f2 = f2 + (f2 * 0.3f) * pitch_env;

        phase1 += kTwoPi * current_f1 * dt;
        if (phase1 > kTwoPi) phase1 -= kTwoPi;

        phase2 += kTwoPi * current_f2 * dt;
        if (phase2 > kTwoPi) phase2 -= kTwoPi;

        float body = (std::sin(phase1) + std::sin(phase2) * 0.6f) * 0.5f;
        body_env *= body_env_factor;
        body *= body_env;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        noise_lpf_state = noise_lpf_state + noise_alpha * (raw_noise - noise_lpf_state);
        float snare_wires = raw_noise - noise_lpf_state;

        noise_env *= noise_env_factor;
        float wire_envelope = noise_env * (1.0f - std::exp(-t * 800.0f));
        snare_wires *= wire_envelope * snap * 1.8f;

        float raw_sample = body * 0.7f + snare_wires;

        master_lpf_state = master_lpf_state + master_alpha * (raw_sample - master_lpf_state);
        float sample = master_lpf_state;

        sample = std::tanh(sample * 1.5f);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_computer_tom(const std::string& formula_str, int samplerate)
{
    float base_freq = 150.0f;
    float decay = 0.35f;
    float click_mix = 0.2f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("freq"))  base_freq = p["freq"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("click")) click_mix = p["click"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    double click_phase = 0.0;

    float dt = 1.0f / static_cast<float>(samplerate);
    float env_factor = std::exp(-dt * (4.0f / decay));
    float env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = base_freq + base_freq * 1.5f * std::exp(-t * 30.0f);
        phase += 2.0 * kPi * freq * dt;

        env *= env_factor;
        float sample = std::sin(phase) * env;

        if (t < 0.008f)
        {
            click_phase += 2.0 * kPi * (base_freq * 3.0f) * dt;
            float square_click = (std::sin(click_phase) >= 0.0f) ? 1.0f : -1.0f;
            float click_env = (1.0f - t / 0.008f);
            sample += square_click * click_mix * click_env;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_coldwave_snare(const std::string& formula_str, int samplerate)
{
    float f0 = 210.0f;
    float f1 = 115.0f;
    float decay = 0.17f;
    float crunch = 0.5f;
    float tone = 900.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("f0"))     f0 = p["f0"];
    if (p.count("f1"))     f1 = p["f1"];
    if (p.count("decay"))  decay = p["decay"];
    if (p.count("crunch")) crunch = p["crunch"];
    if (p.count("tone"))   tone = p["tone"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    uint32_t cw_noise_state = 43210;

    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (kTwoPi * tone);
    float alpha = dt / (rc + dt);

    float body_env_factor = std::exp(-dt * (6.0f / decay));
    float body_env = 1.0f;

    float noise_env_factor = std::exp(-dt * (8.0f / decay));
    float noise_env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;
        float sample = 0.0f;

        if (t < decay)
        {
            float freq = f1 + (f0 - f1) * std::exp(-t * 40.0f);
            phase += 2.0 * kPi * freq * dt;
            body_env *= body_env_factor;
            sample += std::sin(phase) * body_env * 0.5f;
        }

        cw_noise_state ^= cw_noise_state << 13;
        cw_noise_state ^= cw_noise_state >> 17;
        cw_noise_state ^= cw_noise_state << 5;
        float raw_noise = static_cast<float>(cw_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        noise_env *= noise_env_factor;
        float current_n_env = noise_env;
        if (t < 0.002f) current_n_env *= (t / 0.002f);

        sample += lpf_state * current_n_env * crunch;

        sample = std::tanh(sample * 1.2f);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_psytrance_kick(const std::string& formula_str, int samplerate)
{
    float f0 = 240.0f;
    float f1 = 38.0f;
    float decay = 0.22f;
    float click_level = 0.35f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("f0"))     f0 = p["f0"];
    if (p.count("f1"))     f1 = p["f1"];
    if (p.count("decay"))  decay = p["decay"];
    if (p.count("click"))  click_level = p["click"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    uint32_t click_noise_state = 9999;

    float dt = 1.0f / static_cast<float>(samplerate);
    float env_factor = std::exp(-dt * (4.0f / decay));
    float env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = f1 + (f0 - f1) * std::exp(-t * 35.0f);
        phase += 2.0 * kPi * freq * dt;

        env *= env_factor;
        float sample = std::sin(phase) * env;

        if (t < 0.004f)
        {
            click_noise_state ^= click_noise_state << 13;
            click_noise_state ^= click_noise_state >> 17;
            click_noise_state ^= click_noise_state << 5;
            float noise_click = static_cast<float>(click_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

            float click_env = (1.0f - t / 0.004f);
            sample += noise_click * click_level * click_env;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_hihat(const std::string& formula_str, int samplerate)
{
    float decay = 0.08f;
    float tone = 7000.0f;
    float hpf_freq = 4500.0f;
    float q = 0.707f;
    float noise_mix = 0.5f;
    float metal_mix = 0.5f;
    float pitch = 1.0f;
    float detune = 0.0f;
    float drive = 1.0f;

    bool is_open = (formula_str.rfind("hihat_open(", 0) == 0);
    if (is_open)
    {
        decay = 0.35f;
    }

    auto params = parse_synth_params(formula_str);
    if (params.count("decay"))  decay = params["decay"];
    if (params.count("tone"))   tone = params["tone"];
    if (params.count("hpf"))    hpf_freq = params["hpf"];
    if (params.count("q"))      q = params["q"];
    if (params.count("res"))    q = params["res"];
    if (params.count("noise"))  noise_mix = params["noise"];
    if (params.count("metal"))  metal_mix = params["metal"];
    if (params.count("pitch"))  pitch = params["pitch"];
    if (params.count("detune")) detune = params["detune"];
    if (params.count("drive"))  drive = params["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 54321;

    float base_metal_freqs[6] = { 405.3f, 535.1f, 723.4f, 845.2f, 1150.0f, 1420.0f };
    double metal_phases[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    float phase_incs[6];
    for (int m = 0; m < 6; ++m)
    {
        float freq_mod = base_metal_freqs[m] * (1.0f + static_cast<float>(m) * detune * 0.15f);
        phase_incs[m] = (freq_mod * pitch) * (1.0f / static_cast<float>(samplerate));
    }

    float dt = 1.0f / static_cast<float>(samplerate);

    if (hpf_freq < 100.0f) hpf_freq = 100.0f;
    if (hpf_freq > samplerate * 0.45f) hpf_freq = samplerate * 0.45f;
    if (q < 0.5f) q = 0.5f;
    if (q > 10.0f) q = 10.0f;

    float f = 2.0f * std::sin(static_cast<float>(kPi) * hpf_freq * dt);
    float damp = 1.0f / q;
    float hp_s1 = 0.0f;
    float hp_s2 = 0.0f;

    float rc_lpf = 1.0f / (2.0f * kPi * tone);
    float alpha_lpf = rc_lpf / (rc_lpf + dt);
    float lpf_prev = 0.0f;

    float env_factor = std::exp(-dt * (3.0f / decay));
    float env = 1.0f;

    const float inv_attack_time = 1.0f / 0.002f;
    const bool apply_drive = (drive > 1.0f);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float metal_sample = 0.0f;
        for (int m = 0; m < 6; ++m)
        {
            metal_phases[m] += phase_incs[m];
            if (metal_phases[m] >= 1.0) metal_phases[m] -= 1.0;

            float sig = fast_sin(static_cast<float>(metal_phases[m]));
            metal_sample += (sig >= 0.0f ? 1.0f : -1.0f);
        }
        metal_sample /= 6.0f;

        float noise_sample = get_deterministic_noise(noise_state);
        float raw_mix = (metal_sample * metal_mix) + (noise_sample * noise_mix);

        float hp_out = raw_mix - hp_s2 - damp * hp_s1;
        float bp_out = f * hp_out + hp_s1;
        float lp_out = f * bp_out + hp_s2;

        hp_s1 = bp_out;
        hp_s2 = lp_out;

        if (std::isnan(hp_out) || std::isinf(hp_out))
            hp_out = 0.0f;

        lpf_prev = alpha_lpf * lpf_prev + (1.0f - alpha_lpf) * hp_out;

        env *= env_factor;
        float current_env = env;
        if (t < 0.002f)
        {
            current_env *= (t * inv_attack_time);
        }

        float sample = lpf_prev * current_env;

        if (apply_drive)
        {
            sample = std::tanh(sample * drive);
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_clap(const std::string& formula_str, int session_samplerate)
{
    float decay = 0.28f;
    float tone = 1400.0f;
    float spread = 0.011f;
    int burst_count = 4;
    float drive = 1.5f;

    auto params = parse_synth_params(formula_str);
    if (params.count("decay"))  decay = params["decay"];
    if (params.count("tone"))   tone = params["tone"];
    if (params.count("spread")) spread = params["spread"];
    if (params.count("count"))  burst_count = static_cast<int>(params["count"]);
    if (params.count("drive"))  drive = params["drive"];

    if (decay < 0.05f) decay = 0.05f;
    if (decay > 2.0f) decay = 2.0f;
    if (burst_count < 1) burst_count = 1;
    if (burst_count > 12) burst_count = 12;

    int num_samples = static_cast<int>(session_samplerate * (decay * 1.2f));
    if (num_samples < 200) num_samples = 200;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 77777;
    const float dt = 1.0f / static_cast<float>(session_samplerate);

    float lpf_state1 = 0.0f;
    float lpf_state2 = 0.0f;

    const float rc = 1.0f / (2.0f * 3.14159265f * tone);
    const float alpha = dt / (rc + dt);

    const float tail_env_factor = std::exp(-dt * (3.0f / decay));
    float tail_env = 1.0f;

    const float burst_decay_factor = std::exp(-dt * 450.0f);

    float burst_times[12];
    float burst_accents[12];
    float b_env[12] = { 0.0f };
    bool b_active[12] = { false };

    for (int b = 0; b < burst_count; ++b)
    {
        burst_times[b] = static_cast<float>(b) * spread;
        burst_accents[b] = (b == 0) ? 1.6f : (1.0f - 0.05f * static_cast<float>(b));
    }

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float burst_env = 0.0f;
        for (int b = 0; b < burst_count; ++b)
        {
            if (!b_active[b] && t >= burst_times[b])
            {
                b_active[b] = true;
                float dt_burst = t - burst_times[b];
                b_env[b] = std::exp(-dt_burst * 450.0f) * burst_accents[b];
            }
            else if (b_active[b])
            {
                b_env[b] *= burst_decay_factor;
            }
            burst_env += b_env[b];
        }

        float current_tail_env = tail_env;
        tail_env *= tail_env_factor;

        float total_env = (burst_env * 0.85f) + (current_tail_env * 0.15f);

        lpf_state1 += alpha * (noise - lpf_state1);
        lpf_state2 += alpha * (lpf_state1 - lpf_state2);

        float clap_body = (lpf_state1 * 0.5f + lpf_state2 * 1.5f);
        float sample = clap_body * total_env;

        sample = std::tanh(sample * drive);

        if (!std::isfinite(sample)) sample = 0.0f;
        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, session_samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_fart(const std::string& formula_str, int session_samplerate)
{
    float decay = 0.45f;
    float tone = 650.0f;
    float spread = 0.022f;
    int burst_count = 8;
    float pitch = 45.0f;
    float wet = 1.2f;

    auto params = parse_synth_params(formula_str);
    if (params.count("decay"))  decay = params["decay"];
    if (params.count("tone"))   tone = params["tone"];
    if (params.count("spread")) spread = params["spread"];
    if (params.count("count"))  burst_count = static_cast<int>(params["count"]);
    if (params.count("pitch"))  pitch = params["pitch"];
    if (params.count("freq"))   pitch = params["freq"];
    if (params.count("wet"))    wet = params["wet"];

    if (burst_count < 1) burst_count = 1;
    if (tone < 80.0f) tone = 80.0f;

    int num_samples = static_cast<int>(session_samplerate * (decay * 1.2f));
    if (num_samples < 200) num_samples = 200;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 77777;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(session_samplerate);

    float rc = 1.0f / (2.0f * 3.14159265f * tone);
    float alpha = dt / (rc + dt);

    double phase = 0.0;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float burst_env = 0.0f;
        for (int b = 0; b < burst_count; ++b)
        {
            float burst_time = static_cast<float>(b) * spread;
            if (t >= burst_time)
            {
                float dt_burst = t - burst_time;
                burst_env += std::exp(-dt_burst * 120.0f);
            }
        }

        float main_env = std::exp(-t * (3.0f / decay));

        float noise = get_deterministic_noise(noise_state);
        lpf_state = lpf_state + alpha * (noise - lpf_state);

        float current_pitch = pitch + pitch * 0.5f * std::exp(-t * 25.0f);
        phase += 2.0 * 3.14159265 * current_pitch * dt;
        float sub_osc = std::sin(phase) * main_env;

        float combined_signal = (lpf_state * burst_env * 0.7f) + (sub_osc * 0.4f);

        float sample = combined_signal * main_env;
        if (wet > 0.1f)
        {
            sample = std::tanh(sample * (1.0f + wet));
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, session_samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_snare(const std::string& formula_str, int samplerate)
{
    float f0 = 200.0f;
    float f1 = 110.0f;
    float decay = 0.18f;
    float noise_level = 0.6f;
    float noise_decay = 0.12f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("f0"))      f0 = p["f0"];
    if (p.count("f1"))      f1 = p["f1"];
    if (p.count("decay"))   decay = p["decay"];
    if (p.count("noise"))   noise_level = p["noise"];
    if (p.count("n_decay")) noise_decay = p["n_decay"];

    int num_samples = static_cast<int>(samplerate * std::max(decay, noise_decay) * 1.1f);
    if (num_samples <= 0)
        num_samples = static_cast<int>(samplerate * 0.25f);

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    uint32_t snare_noise_state = 12345;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);

    float rc = 1.0f / (2.0f * 3.14159265f * 2500.0f);
    float alpha = dt / (rc + dt);

    float body_env_factor = std::exp(-dt * (6.0f / decay));
    float body_env = 1.0f;

    float noise_env_factor = std::exp(-dt * (8.0f / noise_decay));
    float noise_env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;
        float sample = 0.0f;

        if (t < decay)
        {
            float freq = f1 + (f0 - f1) * std::exp(-t * 45.0f);
            phase += 2.0 * kPi * freq * dt;

            body_env *= body_env_factor;
            sample += std::sin(phase) * body_env * 0.45f;
        }

        if (t < noise_decay)
        {
            snare_noise_state ^= snare_noise_state << 13;
            snare_noise_state ^= snare_noise_state >> 17;
            snare_noise_state ^= snare_noise_state << 5;
            float raw_noise = static_cast<float>(snare_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

            lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

            noise_env *= noise_env_factor;
            float current_n_env = noise_env;
            if (t < 0.0015f) current_n_env *= (t / 0.0015f);

            sample += lpf_state * current_n_env * noise_level;
        }

        data[i] = std::tanh(sample * 1.3f);
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_kick(const std::string& formula_str, int samplerate)
{
    float f0 = 150.0f;
    float f1 = 45.0f;
    float decay = 0.35f;
    float click_level = 0.2f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("f0"))    f0 = p["f0"];
    if (p.count("f1"))    f1 = p["f1"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("click")) click_level = p["click"];

    int num_samples = static_cast<int>(samplerate * decay);
    if (num_samples <= 0)
        num_samples = static_cast<int>(samplerate * 0.5f);

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    uint32_t noise_state = 0x13579BDFu;

    const float dt = 1.0f / static_cast<float>(samplerate);
    const double TWO_PI_DT = kTwoPi * dt;
    const float env_decay_coeff = 3.0f / decay;
    const float inv_click_duration = 1.0f / 0.005f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = f1 + (f0 - f1) * std::exp(-t * 20.0f);
        phase += TWO_PI_DT * freq;
        if (phase > kTwoPi) phase -= kTwoPi;

        float env = std::exp(-t * env_decay_coeff);
        float sample = std::sin(phase) * env;

        if (t < 0.005f)
        {
            noise_state ^= noise_state << 13;
            noise_state ^= noise_state >> 17;
            noise_state ^= noise_state << 5;
            float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;
            float click = raw_noise * (1.0f - t * inv_click_duration);
            sample += click * click_level;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_kick2(const std::string& formula_str, int samplerate)
{
    float f0 = 150.0f;
    float f1 = 45.0f;
    float decay = 0.35f;
    float click_level = 0.2f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("f0"))    f0 = p["f0"];
    if (p.count("f1"))    f1 = p["f1"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("click")) click_level = p["click"];

    int num_samples = static_cast<int>(samplerate * decay);
    if (num_samples <= 0)
        num_samples = static_cast<int>(samplerate * 0.5f);

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) / samplerate;

        float freq = f1 + (f0 - f1) * std::exp(-t * 20.0f);
        phase += 2.0 * kPi * freq / samplerate;

        float env = std::exp(-t * (3.0f / decay));
        float sample = std::sin(phase) * env;

        if (t < 0.005f)
        {
            float click = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * (1.0f - t / 0.005f);
            sample += click * click_level;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_box_sound(const std::string& formula_str, int samplerate)
{
    float duration   = 0.32f;
    float base_freq  = 55.0f;
    float freq_sweep = -25.0f;
    float noise_mix  = 0.7f;
    float bass_mix   = 0.4f;
    float vibrato_r  = 22.0f;
    float vibrato_d  = 0.5f;
    float squeak_mix = 0.25f;
    float attack     = 0.002f;
    float release    = 0.14f;
    float gain       = 0.95f;

    float cutoff_min = 120.0f;
    float cutoff_max = 900.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("duration")) duration   = p["duration"];
    if (p.count("freq"))     base_freq  = p["freq"];
    if (p.count("sweep"))    freq_sweep = p["sweep"];
    if (p.count("noise"))    noise_mix  = p["noise"];
    if (p.count("bass"))     bass_mix   = p["bass"];
    if (p.count("vibrato")) vibrato_r  = p["vibrato"];
    if (p.count("squeak"))   squeak_mix = p["squeak"];
    if (p.count("attack"))   attack     = p["attack"];
    if (p.count("release"))  release    = p["release"];
    if (p.count("gain"))     gain       = p["gain"];

    int num_samples = static_cast<int>(samplerate * (duration * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 98765;
    double bass_phase = 0.0;
    double squeak_phase = 0.0;
    float vibrato_phase = 0.0f;

    float brown_noise = 0.0f;

    float lpf_state = 0.0f;
    float hpf_prev_in = 0.0f;
    float hpf_prev_out = 0.0f;

    float dt = 1.0f / static_cast<float>(samplerate);
    float hpf_cutoff = 35.0f;
    float rc_hpf = 1.0f / (2.0f * kPi * hpf_cutoff);
    float alpha_hpf = rc_hpf / (rc_hpf + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;
        float u = t / duration;

        float env = (t < attack)
        ? (t / attack)
        : std::exp(-(t - attack) / release);

        float pressure_env = env * (1.0f + 1.5f * std::exp(-t * 25.0f));

        float vibrato = std::sin(vibrato_phase) * vibrato_d;
        vibrato_phase += kTwoPi * vibrato_r * dt;

        float current_freq = base_freq + freq_sweep * u;
        current_freq *= (1.0f + vibrato);

        bass_phase += kTwoPi * current_freq * dt;
        float bass_wave = std::sin(bass_phase);
        bass_wave += std::sin(bass_phase * 2.0f) * 0.4f;

        float white = get_deterministic_noise(noise_state);
        brown_noise = brown_noise * 0.92f + white * 0.28f;

        float squeak = 0.0f;
        if (u < 0.4f)
        {
            float squeak_freq = 1800.0f + 600.0f * std::sin(u * 15.0f);
            squeak_phase += kTwoPi * squeak_freq * dt;
            squeak = std::sin(squeak_phase) * (white * 0.5f + 0.5f) * (1.0f - u / 0.4f) * squeak_mix;
        }

        float dynamic_cutoff = cutoff_min + (cutoff_max - cutoff_min) * pressure_env;
        float rc_lpf = 1.0f / (2.0f * kPi * dynamic_cutoff);
        float alpha_lpf = dt / (rc_lpf + dt);

        float mixed = (brown_noise * noise_mix) + (bass_wave * bass_mix * 0.5f) + squeak;
        mixed *= pressure_env;

        lpf_state = lpf_state + alpha_lpf * (mixed - lpf_state);

        float hpf_out = alpha_hpf * (hpf_prev_out + lpf_state - hpf_prev_in);
        hpf_prev_in = lpf_state;
        hpf_prev_out = hpf_out;

        float s = std::tanh(hpf_out * 2.5f) * gain;
        data[i] = s;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_cowbell_sound2(const std::string& formula_str, int samplerate)
{
    float base_freq = 560.0f;
    float ratio     = 1.45f;
    float decay     = 0.18f;
    float tone      = 1200.0f;
    float drive     = 1.2f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("freq"))  base_freq = p["freq"];
    if (p.count("ratio")) ratio     = p["ratio"];
    if (p.count("decay")) decay     = p["decay"];
    if (p.count("tone"))  tone      = p["tone"];
    if (p.count("drive")) drive     = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);

    if (!buffer)
    {
        std::cerr << "[ERROR] Failed to allocate audio buffer for cowbell!\n";
        return nullptr;
    }

    float* data = buffer->getWritePointer(0);

    double phase1 = 0.0;
    double phase2 = 0.0;

    const float dt = 1.0f / static_cast<float>(samplerate);

    const double inc1 = kTwoPi * base_freq * dt;
    const double inc2 = kTwoPi * (base_freq * ratio) * dt;

    float lpf_state = 0.0f;
    float rc = 1.0f / (2.0f * static_cast<float>(kPi) * tone);
    float alpha = dt / (rc + dt);

    const float env_factor = std::exp(-dt * (4.0f / decay));
    float env = 1.0f;

    const float inv_click_time = 1.0f / 0.0015f;
    const bool apply_drive = (drive > 1.0f);

    float t = 0.0f;

    for (int i = 0; i < num_samples; ++i, t += dt)
    {
        phase1 += inc1;
        if (phase1 > kTwoPi) phase1 -= kTwoPi;

        phase2 += inc2;
        if (phase2 > kTwoPi) phase2 -= kTwoPi;

        float sq1 = (std::sin(static_cast<float>(phase1)) >= 0.0f) ? 1.0f : -1.0f;
        float sq2 = (std::sin(static_cast<float>(phase2)) >= 0.0f) ? 1.0f : -1.0f;

        float raw_signal = (sq1 * 0.5f) + (sq2 * 0.5f);

        lpf_state += alpha * (raw_signal - lpf_state);

        float current_env = env;
        env *= env_factor;

        float click = 0.0f;
        if (t < 0.0015f)
            click = (1.0f - t * inv_click_time) * 0.4f;

        float sample = (lpf_state + click) * current_env;

        if (apply_drive)
            sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_cowbell_sound(const std::string& formula_str, int samplerate)
{
    float base_freq = 560.0f;
    float ratio     = 1.45f;
    float decay     = 0.18f;
    float tone      = 1200.0f;
    float drive     = 1.2f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("freq"))  base_freq = p["freq"];
    if (p.count("ratio")) ratio     = p["ratio"];
    if (p.count("decay")) decay     = p["decay"];
    if (p.count("tone"))  tone      = p["tone"];
    if (p.count("drive")) drive     = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);

    if (!buffer)
    {
        std::cerr << "[ERROR] Failed to allocate audio buffer for cowbell!\n";
        return nullptr;
    }

    float* data = buffer->getWritePointer(0);

    double phase1 = 0.0;
    double phase2 = 0.0;

    float dt = 1.0f / static_cast<float>(samplerate);

    float lpf_state = 0.0f;
    float rc = 1.0f / (kTwoPi * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq1 = base_freq;
        float freq2 = base_freq * ratio;

        phase1 += kTwoPi * freq1 * dt;
        phase2 += kTwoPi * freq2 * dt;

        float sq1 = (std::sin(phase1) >= 0.0f) ? 1.0f : -1.0f;
        float sq2 = (std::sin(phase2) >= 0.0f) ? 1.0f : -1.0f;

        float raw_signal = (sq1 * 0.5f) + (sq2 * 0.5f);

        lpf_state = lpf_state + alpha * (raw_signal - lpf_state);

        float env = std::exp(-t * (4.0f / decay));

        float click = 0.0f;
        if (t < 0.0015f)
            click = (1.0f - t / 0.0015f) * 0.4f;

        float sample = (lpf_state + click) * env;

        if (drive > 1.0f)
            sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_rimshot(const std::string& formula_str, int samplerate)
{
    float freq  = 440.0f;
    float decay = 0.12f;
    float click = 0.7f;
    float tone  = 2500.0f;
    float drive = 1.4f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("freq"))  freq = p["freq"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("click")) click = p["click"];
    if (p.count("tone"))  tone = p["tone"];
    if (p.count("drive")) drive = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    uint32_t rim_noise_state = 54321;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);

    float rc = 1.0f / (2.0f * kPi * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float current_freq = freq + freq * 0.8f * std::exp(-t * 80.0f);
        phase += 2.0 * kPi * current_freq * dt;
        float body_env = std::exp(-t * (5.0f / decay));
        float body = std::sin(phase) * body_env * 0.4f;

        rim_noise_state ^= rim_noise_state << 13;
        rim_noise_state ^= rim_noise_state >> 17;
        rim_noise_state ^= rim_noise_state << 5;
        float raw_noise = static_cast<float>(rim_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        float click_env = 0.0f;
        if (t < 0.003f)
            click_env = (1.0f - t / 0.003f);
        else
            click_env = std::exp(-t * 30.0f) * 0.15f;

        float rim_click = lpf_state * click_env * click;

        float sample = body + rim_click;

        if (drive > 1.0f)
            sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_dnb_kick(const std::string& formula_str, int samplerate)
{
    float start_freq = 150.0f;
    float end_freq   = 45.0f;
    float pitch_time = 0.035f;
    float decay      = 0.35f;
    float click      = 1.5f;
    float drive      = 2.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("start_freq")) start_freq = p["start_freq"];
    if (p.count("end_freq"))   end_freq   = p["end_freq"];
    if (p.count("pitch_time")) pitch_time = p["pitch_time"];
    if (p.count("decay"))      decay      = p["decay"];
    if (p.count("click"))      click      = p["click"];
    if (p.count("drive"))      drive      = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    float dt = 1.0f / static_cast<float>(samplerate);
    uint32_t noise_state = 13579;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = end_freq + (start_freq - end_freq) * std::exp(-t / pitch_time);
        phase += 2.0 * kPi * freq * dt;

        float env = std::exp(-t * (3.0f / decay));
        float sub = std::sin(phase) * env;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float click_env = 0.0f;
        if (t < 0.004f)
            click_env = (1.0f - t / 0.004f) * click;

        float trans = noise * click_env;

        float sample = sub + trans;

        sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.005f);
    return buffer;
}

juce::AudioBuffer<float>* render_dnb_snare(const std::string& formula_str, int samplerate)
{
    float body_freq = 210.0f;
    float decay     = 0.25f;
    float snap      = 2.0f;
    float tone      = 3500.0f;
    float drive     = 1.8f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("body_freq")) body_freq = p["body_freq"];
    if (p.count("decay"))     decay     = p["decay"];
    if (p.count("snap"))      snap      = p["snap"];
    if (p.count("tone"))      tone      = p["tone"];
    if (p.count("drive"))     drive     = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    uint32_t noise_state = 98765;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);

    float rc = 1.0f / (2.0f * kPi * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float cur_freq = body_freq + body_freq * 0.5f * std::exp(-t * 50.0f);
        phase += 2.0 * kPi * cur_freq * dt;
        float body_env = std::exp(-t * (6.0f / decay));
        float body = std::sin(phase) * body_env * 0.6f;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        float wire_env = std::exp(-t * (4.0f / decay)) * snap;
        if (t < 0.002f)
            wire_env *= 2.5f;

        float wires = lpf_state * wire_env * 0.5f;

        float sample = body + wires;

        sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.004f);
    return buffer;
}

juce::AudioBuffer<float>* render_snare_fluffy(const std::string& formula_str, int samplerate)
{
    float body_freq  = 190.0f;
    float decay      = 0.22f;
    float snap       = 2.2f;
    float fm_amount  = 1.5f;
    float tone       = 4200.0f;
    float drive      = 2.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("freq"))      body_freq = p["freq"];
    if (p.count("body_freq")) body_freq = p["body_freq"];
    if (p.count("decay"))     decay     = p["decay"];
    if (p.count("snap"))      snap      = p["snap"];
    if (p.count("fm"))        fm_amount = p["fm"];
    if (p.count("tone"))      tone      = p["tone"];
    if (p.count("drive"))     drive     = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase1 = 0.0, phase2 = 0.0, fm_phase = 0.0;
    uint32_t noise_state = 0xACE1u;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);

    float rc = 1.0f / (2.0f * kPi * tone);
    float alpha = dt / (rc + dt);

    const int delay_len = static_cast<int>(samplerate * 0.002f);
    std::vector<float> noise_delay(delay_len > 0 ? delay_len : 1, 0.0f);
    int delay_idx = 0;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float pitch_env = std::exp(-t * 60.0f);
        float cur_freq = body_freq + (body_freq * 0.7f * pitch_env);

        float fm_env = std::exp(-t * 250.0f) * fm_amount;
        fm_phase += 2.0 * kPi * (cur_freq * 2.25f) * dt;
        float fm_mod = std::sin(fm_phase) * fm_env;

        phase1 += 2.0 * kPi * cur_freq * dt + fm_mod;
        phase2 += 2.0 * kPi * (cur_freq * 1.6f) * dt;

        float body_env = std::exp(-t * (5.5f / decay));
        float body = (std::sin(phase1) * 0.6f + std::sin(phase2) * 0.3f) * body_env;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        float delayed_noise = noise_delay[delay_idx];
        noise_delay[delay_idx] = lpf_state;
        delay_idx = (delay_idx + 1) % noise_delay.size();

        float dense_noise = lpf_state + (delayed_noise * 0.4f);

        float snap_env = std::exp(-t * (4.2f / decay)) * snap;
        if (t < 0.003f)
            snap_env *= 2.2f;

        float wires = dense_noise * snap_env * 0.45f;

        float sample = body + wires;
        if (drive > 1.0f)
            sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.004f);
    return buffer;
}

juce::AudioBuffer<float>* render_metal(const std::string& formula_str, int samplerate)
{
    float decay = 1.2f;
    float metal = 750.0f;
    float tone = 5500.0f;
    float drive = 1.3f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("decay")) decay = p["decay"];
    if (p.count("metal")) metal = p["metal"];
    if (p.count("tone"))  tone  = p["tone"];
    if (p.count("drive")) drive = p["drive"];

    int num_samples = static_cast<int>(samplerate * decay);
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double p1 = 0.0, p2 = 0.0, p3 = 0.0, p4 = 0.0, p5 = 0.0, p6 = 0.0;
    uint32_t noise_state = 0x98765432u;

    float dt = 1.0f / static_cast<float>(samplerate);

    float bpf_s1 = 0.0f, bpf_s2 = 0.0f;
    float w0 = 2.0f * kPi * tone * dt;
    if (w0 > 0.8f) w0 = 0.8f;
    float q = 2.0f;
    float alpha = w0 / q;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        p1 += 2.0 * kPi * (metal * 1.000f) * dt;
        p2 += 2.0 * kPi * (metal * 1.473f) * dt;
        p3 += 2.0 * kPi * (metal * 2.154f) * dt;
        p4 += 2.0 * kPi * (metal * 3.112f) * dt;
        p5 += 2.0 * kPi * (metal * 4.391f) * dt;
        p6 += 2.0 * kPi * (metal * 5.817f) * dt;

        float osc1 = (std::sin(p1) > 0.0f ? 1.0f : -1.0f);
        float osc2 = (std::sin(p2) > 0.0f ? 1.0f : -1.0f);
        float osc3 = std::sin(p3);
        float osc4 = std::sin(p4);
        float osc5 = std::sin(p5);
        float osc6 = std::sin(p6);

        float metallic_ring = (osc1 * osc3 + osc2 * osc4 + osc5 * osc6) * 0.33f;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float combined = (raw_noise * 0.35f) + (metallic_ring * 0.65f);

        bpf_s1 = bpf_s1 + w0 * (combined - bpf_s1 - alpha * bpf_s2);
        bpf_s2 = bpf_s2 + w0 * bpf_s1;
        float filtered = bpf_s1;

        float body_env = std::exp(-t * (4.5f / decay));
        float noise_env = std::exp(-t * (6.0f / decay));

        float attack = 0.0f;
        if (t < 0.002f)
            attack = (1.0f - t / 0.002f) * 1.5f;

        float sample = (filtered * noise_env + metallic_ring * body_env * 0.5f + attack) * body_env;

        float release_fade = 1.0f;
        float tail_time = decay * 0.2f;
        if (t > (decay - tail_time))
        {
            release_fade = (decay - t) / tail_time;
            if (release_fade < 0.0f) release_fade = 0.0f;
        }

        sample *= release_fade;

        if (drive > 1.0f)
            sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    return buffer;
}

juce::AudioBuffer<float>* render_crash(const std::string& formula_str, int samplerate)
{
    float size = 18.0f;
    float decay = 0.0f;
    float tone = 5500.0f;
    float noise_mix = 0.6f;
    float metal_mix = 0.4f;
    float pitch = 1.0f;
    float detune = 0.35f;
    float drive = 1.2f;

    auto params = parse_synth_params(formula_str);
    if (params.count("size"))   size = params["size"];
    if (params.count("decay"))  decay = params["decay"];
    if (params.count("tone"))   tone = params["tone"];
    if (params.count("noise"))  noise_mix = params["noise"];
    if (params.count("metal"))  metal_mix = params["metal"];
    if (params.count("pitch"))  pitch = params["pitch"];
    if (params.count("detune")) detune = params["detune"];
    if (params.count("drive"))  drive = params["drive"];

    if (size < 12.0f) size = 12.0f;
    if (size > 24.0f) size = 24.0f;

    float size_scale = 16.0f / size;
    pitch *= size_scale;

    tone *= (1.0f - (size - 14.0f) * 0.03f);

    if (decay <= 0.0f)
        decay = 1.2f + (size - 14.0f) * 0.18f;

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 0xABCD1234u;

    float base_metal_freqs[6] = { 280.5f, 410.2f, 595.8f, 780.4f, 1050.0f, 1380.0f };
    float metal_phases[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    float hpf_prev_input = 0.0f;
    float hpf_prev_output = 0.0f;

    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (2.0f * kPi * tone);
    float alpha = rc / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float metal_sample = 0.0f;
        for (int m = 0; m < 6; ++m)
        {
            float freq_mod = base_metal_freqs[m] * (1.0f + static_cast<float>(m) * detune * 0.2f);
            metal_phases[m] += (freq_mod * pitch) * dt;
            if (metal_phases[m] >= 1.0f) metal_phases[m] -= 1.0f;
            float sq = (metal_phases[m] < 0.5f) ? 1.0f : -1.0f;
            metal_sample += sq;
        }
        metal_sample /= 6.0f;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float noise_sample = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float raw_mix = (metal_sample * metal_mix) + (noise_sample * noise_mix);

        float hpf_output = alpha * (hpf_prev_output + raw_mix - hpf_prev_input);
        hpf_prev_input = raw_mix;
        hpf_prev_output = hpf_output;

        float env = std::exp(-2.5f * (t / decay));
        if (t < 0.003f)
            env *= (t / 0.003f) * 2.5f;

        float sample = hpf_output * env;

        if (drive > 1.0f)
            sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.015f);
    return buffer;
}

struct Mode
{
    float coef1 = 0.0f, coef2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
    float amp = 0.0f;
};

juce::AudioBuffer<float>* render_crash_hybrid(const std::string& formula_str, int samplerate)
{
    if (samplerate <= 0) samplerate = 44100;

    float decay = 1.5f;
    float tone = 5500.0f;
    float pitch = 1.0f;
    float drive = 1.2f;
    float noise_mix = 0.5f;

    auto params = parse_synth_params(formula_str);

    if (params.count("decay"))  decay = params["decay"];
    if (params.count("tone"))   tone = params["tone"];
    if (params.count("pitch"))  pitch = params["pitch"];
    if (params.count("drive"))  drive = params["drive"];
    if (params.count("noise"))  noise_mix = params["noise"];

    if (decay < 0.1f) decay = 0.1f;
    if (decay > 6.0f) decay = 6.0f;

    if (pitch < 0.3f) pitch = 0.3f;
    if (pitch > 3.0f) pitch = 3.0f;

    if (noise_mix < 0.0f) noise_mix = 0.0f;
    if (noise_mix > 1.0f) noise_mix = 1.0f;

    float cutoff = tone;

    if (cutoff <= 1.0f)
        cutoff = 2000.0f + cutoff * 8000.0f;

    if (cutoff < 500.0f) cutoff = 500.0f;
    if (cutoff > samplerate * 0.45f) cutoff = samplerate * 0.45f;

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 512) num_samples = 512;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    if (!buffer) return nullptr;
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 0xABCD1234u;
    uint32_t strike_noise_state = 0x55AA3311u;

    float base_freqs[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
    float phases[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    float strike_freqs[4] = { 3800.0f, 5100.0f, 6600.0f, 8400.0f };
    float strike_phases[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    float dt = 1.0f / static_cast<float>(samplerate);

    float hpf_prev_input = 0.0f;
    float hpf_prev_output = 0.0f;

    float rc = 1.0f / (2.0f * kPi * cutoff);
    float alpha = rc / (rc + dt);

    float noise_lpf_state = 0.0f;
    float noise_alpha = dt / (0.00003f + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float metal_sample = 0.0f;
        for (int m = 0; m < 6; ++m)
        {
            phases[m] += (base_freqs[m] * pitch) * dt;
            if (phases[m] >= 1.0f) phases[m] -= 1.0f;

            metal_sample += (phases[m] < 0.5f) ? 1.0f : -1.0f;
        }
        metal_sample /= 6.0f;

        float strike_sample = 0.0f;
        float pitch_drop = 1.0f + 3.0f * std::exp(-t * 3000.0f);

        for (int s = 0; s < 4; ++s)
        {
            strike_phases[s] += (strike_freqs[s] * pitch * pitch_drop) * dt;
            if (strike_phases[s] >= 1.0f) strike_phases[s] -= 1.0f;

            strike_sample += std::sin(strike_phases[s] * 2.0f * kPi);
        }
        strike_sample *= 0.25f;

        float strike_env = std::exp(-t * 900.0f);

        strike_noise_state ^= strike_noise_state << 13;
        strike_noise_state ^= strike_noise_state >> 17;
        strike_noise_state ^= strike_noise_state << 5;
        float stick_click = static_cast<float>(strike_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;
        stick_click *= std::exp(-t * 1800.0f);

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        noise_lpf_state += noise_alpha * (raw_noise - noise_lpf_state);
        float noise_diff = raw_noise - noise_lpf_state;

        float noise_sample = (noise_lpf_state * 0.4f + noise_diff * 1.2f);

        float raw_mix = (metal_sample * (1.0f - noise_mix))
        + (noise_sample * noise_mix)
        + (strike_sample * strike_env * 1.2f)
        + (stick_click * 0.6f);

        float hpf_output = alpha * (hpf_prev_output + raw_mix - hpf_prev_input);
        hpf_prev_input = raw_mix;
        hpf_prev_output = hpf_output;

        float env = std::exp(-3.5f * std::pow(t / decay, 0.85f));
        if (t < 0.0008f)
            env *= (t / 0.0008f);

        float sample = hpf_output * env;

        if (drive > 1.0f)
            sample = std::tanh(sample * drive);

        if (!std::isfinite(sample)) sample = 0.0f;

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.02f);
    return buffer;
}

juce::AudioBuffer<float>* render_ride(const std::string& formula_str, int samplerate)
{
    float decay = 1.8f;
    float pitch = 1.1f;
    float tone = 0.6f;
    float drive = 1.1f;
    float noise = 0.25f;
    float ping = 0.6f;

    std::map<std::string, float> p = parse_synth_params(formula_str);

    if (p.count("decay")) decay = p["decay"];
    if (p.count("pitch")) pitch = p["pitch"];
    if (p.count("tone"))  tone  = p["tone"];
    if (p.count("drive")) drive = p["drive"];
    if (p.count("noise")) noise = p["noise"];
    if (p.count("ping"))  ping  = p["ping"];

    if (decay < 0.1f) decay = 0.1f;
    if (decay > 8.0f) decay = 8.0f;

    float filter_freq = tone;
    if (filter_freq <= 1.0f)
        filter_freq = 4000.0f + filter_freq * 10000.0f;

    if (filter_freq < 1000.0f) filter_freq = 1000.0f;
    if (filter_freq > samplerate * 0.45f) filter_freq = samplerate * 0.45f;

    int num_samples = static_cast<int>(samplerate * decay);

    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double p1 = 0.0, p2 = 0.0, p3 = 0.0, p4 = 0.0, p5 = 0.0, p6 = 0.0;
    uint32_t noise_state = 0x87654321u;
    uint32_t ping_noise_state = 0x12345678u;

    float dt = 1.0f / static_cast<float>(samplerate);

    float bpf_s1 = 0.0f, bpf_s2 = 0.0f;
    float w0 = kTwoPi * filter_freq * dt;
    if (w0 > 0.92f) w0 = 0.92f;

    float q = 1.4f;
    float alpha = w0 / q;

    float base_metal = 1250.0f * pitch;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        p1 += kTwoPi * (base_metal * 1.000f) * dt;
        p2 += kTwoPi * (base_metal * 1.618f) * dt;
        p3 += kTwoPi * (base_metal * 2.542f) * dt;
        p4 += kTwoPi * (base_metal * 3.833f) * dt;
        p5 += kTwoPi * (base_metal * 5.211f) * dt;
        p6 += kTwoPi * (base_metal * 7.145f) * dt;

        float osc1 = std::sin(p1);
        float osc2 = std::sin(p2);
        float osc3 = std::sin(p3);
        float osc4 = std::sin(p4);
        float osc5 = std::sin(p5);
        float osc6 = std::sin(p6);

        float metallic_ring = (osc1 * osc3 + osc2 * osc4 + osc5 * osc6) * 0.22f;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        ping_noise_state ^= ping_noise_state << 13;
        ping_noise_state ^= ping_noise_state >> 17;
        ping_noise_state ^= ping_noise_state << 5;
        float raw_ping_noise = static_cast<float>(ping_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float stick_ping_env = std::exp(-t * 400.0f) * ping;
        float stick_ping = raw_ping_noise * stick_ping_env;

        float combined = (raw_noise * noise) + (metallic_ring * (1.0f - noise)) + (stick_ping * 1.5f);

        bpf_s1 = bpf_s1 + w0 * (combined - bpf_s1 - alpha * bpf_s2);
        bpf_s2 = bpf_s2 + w0 * bpf_s1;
        float filtered = bpf_s1;

        float body_env = std::exp(-t * (4.5f / decay));

        float sample = (filtered * body_env * 0.55f + metallic_ring * body_env * 0.25f);

        float release_fade = 1.0f;
        float tail_time = decay * 0.2f;

        if (t > (decay - tail_time))
        {
            release_fade = (decay - t) / tail_time;
            if (release_fade < 0.0f) release_fade = 0.0f;
        }

        sample *= release_fade;

        if (drive > 1.0f)
            sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    return buffer;
}

juce::AudioBuffer<float>* render_glass(const std::string& formula_str, int samplerate)
{
    float decay = 1.8f;
    float pitch = 1.1f;
    float tone = 0.6f;
    float drive = 1.1f;
    float noise = 0.25f;
    float ping = 0.6f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("decay")) decay = p["decay"];
    if (p.count("pitch")) pitch = p["pitch"];
    if (p.count("tone"))  tone  = p["tone"];
    if (p.count("drive")) drive = p["drive"];
    if (p.count("noise")) noise = p["noise"];
    if (p.count("ping"))  ping  = p["ping"];

    if (decay < 0.1f) decay = 0.1f;
    if (decay > 8.0f) decay = 8.0f;

    float filter_freq = tone;
    if (filter_freq <= 1.0f)
        filter_freq = 3000.0f + filter_freq * 9000.0f;

    if (filter_freq < 500.0f) filter_freq = 500.0f;
    if (filter_freq > samplerate * 0.45f) filter_freq = samplerate * 0.45f;

    int num_samples = static_cast<int>(samplerate * decay);
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double p1 = 0.0, p2 = 0.0, p3 = 0.0, p4 = 0.0, p5 = 0.0, p6 = 0.0;
    uint32_t noise_state = 0x87654321u;

    float dt = 1.0f / static_cast<float>(samplerate);

    float bpf_s1 = 0.0f, bpf_s2 = 0.0f;
    float w0 = 2.0f * kPi * filter_freq * dt;
    if (w0 > 0.9f) w0 = 0.9f;
    float q = 1.8f;
    float alpha = w0 / q;

    float base_glass = 1200.0f * pitch;

    float body_env_factor = std::exp(-dt * (5.0f / decay));
    float body_env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        p1 += (base_glass * 1.000f) * dt;
        p2 += (base_glass * 1.618f) * dt;
        p3 += (base_glass * 2.542f) * dt;
        p4 += (base_glass * 3.833f) * dt;
        p5 += (base_glass * 5.211f) * dt;
        p6 += (base_glass * 7.145f) * dt;

        float osc1 = fast_sin(static_cast<float>(p1));
        float osc2 = fast_sin(static_cast<float>(p2));
        float osc3 = fast_sin(static_cast<float>(p3));
        float osc4 = fast_sin(static_cast<float>(p4));
        float osc5 = fast_sin(static_cast<float>(p5));
        float osc6 = fast_sin(static_cast<float>(p6));

        float glass_ring = (osc1 * osc3 + osc2 * osc4 + osc5 * osc6) * 0.25f;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float combined = (raw_noise * noise) + (glass_ring * (1.0f - noise));

        bpf_s1 = bpf_s1 + w0 * (combined - bpf_s1 - alpha * bpf_s2);
        bpf_s2 = bpf_s2 + w0 * bpf_s1;
        float filtered = bpf_s1;

        body_env *= body_env_factor;

        float attack = 0.0f;
        if (t < 0.0012f)
            attack = (1.0f - t / 0.0012f) * 3.5f * ping;

        float sample = (filtered * body_env * 0.5f + glass_ring * body_env * 0.3f + attack);

        float release_fade = 1.0f;
        float tail_time = decay * 0.25f;
        if (t > (decay - tail_time))
        {
            release_fade = (decay - t) / tail_time;
            if (release_fade < 0.0f) release_fade = 0.0f;
        }

        sample *= release_fade;

        if (drive > 1.0f)
            sample = std::tanh(sample * drive);

        data[i] = sample;
    }

    return buffer;
}

juce::AudioBuffer<float>* render_psykick(const std::string& formula_str, int samplerate)
{
    float f0 = 240.0f;
    float f1 = 38.0f;
    float decay = 0.22f;
    float click_level = 0.25f;
    float drive = 0.5f;
    float curve = 35.0f;
    float fm = 0.3f;
    float click_hp = 3000.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("f0"))       f0 = p["f0"];
    if (p.count("f1"))       f1 = p["f1"];
    if (p.count("decay"))    decay = p["decay"];
    if (p.count("click"))    click_level = p["click"];
    if (p.count("drive"))    drive = p["drive"];
    if (p.count("curve"))    curve = p["curve"];
    if (p.count("fm"))       fm = p["fm"];
    if (p.count("click_hp")) click_hp = p["click_hp"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase = 0.0;
    uint32_t click_noise_state = 9999;

    float dt = 1.0f / static_cast<float>(samplerate);
    float env_factor = std::exp(-dt * (4.0f / decay));
    float env = 1.0f;

    float hp_alpha = 1.0f / (1.0f + kTwoPi * click_hp * dt);
    float prev_noise_in = 0.0f;
    float prev_noise_out = 0.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq_env = std::exp(-t * curve);
        float freq = f1 + (f0 - f1) * freq_env;

        float fm_mod = std::sin(2.0 * kPi * freq * 2.0 * t) * fm * freq_env * 50.0f;
        phase += kTwoPi * (freq + fm_mod) * dt;

        env *= env_factor;
        float body_signal = std::sin(phase) * env;

        if (drive > 0.001f)
        {
            float pre_gain = 1.0f + drive * 6.0f;
            float raw_driven = body_signal * pre_gain;
            float clipped = std::max(-1.0f, std::min(1.0f, raw_driven));
            body_signal = juce::jmap(drive, 0.0f, 1.0f, body_signal, clipped * 0.9f);
        }

        float sample = body_signal;

        if (t < 0.004f)
        {
            click_noise_state ^= click_noise_state << 13;
            click_noise_state ^= click_noise_state >> 17;
            click_noise_state ^= click_noise_state << 5;
            float raw_noise = static_cast<float>(click_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

            float filtered_noise = hp_alpha * (prev_noise_out + raw_noise - prev_noise_in);
            prev_noise_in = raw_noise;
            prev_noise_out = filtered_noise;

            float click_env = (1.0f - t / 0.004f);
            sample += filtered_noise * click_level * click_env;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_goakick(const std::string& formula_str, int samplerate)
{
    float f0 = 220.0f;
    float f1 = 48.0f;
    float decay = 0.28f;
    float punch = 0.40f;
    float warmth = 0.35f;
    float bit_depth = 16.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("f0"))        f0 = p["f0"];
    if (p.count("f1"))        f1 = p["f1"];
    if (p.count("decay"))     decay = p["decay"];
    if (p.count("punch"))     punch = p["punch"];
    if (p.count("warmth"))    warmth = p["warmth"];
    if (p.count("bit_depth")) bit_depth = p["bit_depth"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase_body = 0.0;
    double phase_punch = 0.0;

    const float dt = 1.0f / static_cast<float>(samplerate);

    float env_factor = std::exp(-dt * (3.5f / decay));
    float env = 1.0f;

    float pitch_env_factor = std::exp(-dt * 28.0f);
    float pitch_env = 1.0f;

    const float steps = std::pow(2.0f, bit_depth - 1.0f);
    const bool apply_bitcrush = (bit_depth < 15.9f);
    const bool apply_warmth = (warmth > 0.05f);
    const float warmth_drive = 1.0f + warmth * 1.8f;
    const float warmth_offset = 0.05f * warmth;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq_body = f1 + (f0 - f1) * pitch_env;
        pitch_env *= pitch_env_factor;

        phase_body += kTwoPi * freq_body * dt;
        if (phase_body > kTwoPi) phase_body -= kTwoPi;

        env *= env_factor;
        float sin_wave = std::sin(static_cast<float>(phase_body));

        float phase_norm = static_cast<float>(phase_body / kPi);
        float tri_wave = 0.0f;
        if (phase_norm < 1.0f)
            tri_wave = -1.0f + 2.0f * phase_norm;
        else
            tri_wave = 3.0f - 2.0f * phase_norm;

        float body_signal = juce::jmap(warmth, 0.0f, 1.0f, sin_wave, tri_wave) * env;

        float punch_signal = 0.0f;
        if (t < 0.015f)
        {
            float p_env = (1.0f - t / 0.015f);
            float freq_punch = 160.0f * p_env;
            phase_punch += kTwoPi * freq_punch * dt;
            if (phase_punch > kTwoPi) phase_punch -= kTwoPi;
            punch_signal = std::sin(static_cast<float>(phase_punch)) * p_env * punch;
        }

        float sample = body_signal + punch_signal;

        if (apply_warmth)
        {
            sample += warmth_offset;
            sample = std::tanh(sample * warmth_drive);
        }

        if (apply_bitcrush)
            sample = std::round(sample * steps) / steps;

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_goasnare(const std::string& formula_str, int samplerate)
{
    float tone_freq = 180.0f;
    float tone_decay = 0.08f;
    float noise_freq = 3200.0f;
    float noise_decay = 0.16f;
    float snappy = 0.50f;
    float drive = 0.35f;
    float bit_depth = 16.0f;

    std::map<std::string, float> p = parse_synth_params(formula_str);
    if (p.count("tone_freq"))   tone_freq = p["tone_freq"];
    if (p.count("tone_decay"))  tone_decay = p["tone_decay"];
    if (p.count("noise_freq"))  noise_freq = p["noise_freq"];
    if (p.count("noise_decay")) noise_decay = p["noise_decay"];
    if (p.count("snappy"))      snappy = p["snappy"];
    if (p.count("drive"))       drive = p["drive"];
    if (p.count("bit_depth"))   bit_depth = p["bit_depth"];

    float max_decay = std::max(tone_decay, noise_decay);
    int num_samples = static_cast<int>(samplerate * (max_decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float>* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    double phase1 = 0.0;
    double phase2 = 0.0;
    uint32_t noise_state = 54321;

    const float dt = 1.0f / static_cast<float>(samplerate);

    float tone_env_factor = std::exp(-dt * (4.5f / tone_decay));
    float noise_env_factor = std::exp(-dt * (4.0f / noise_decay));
    float pitch_env_factor = std::exp(-dt * 40.0f);

    float env_tone = 1.0f;
    float env_noise = 1.0f;
    float pitch_env = 1.0f;

    float w0 = static_cast<float>(kTwoPi) * noise_freq * dt;
    float q = 1.8f;
    float alpha = std::sin(w0) / (2.0f * q);
    float b0 = alpha;
    float b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * std::cos(w0);
    float a2 = 1.0f - alpha;

    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

    const float steps = std::pow(2.0f, bit_depth - 1.0f);
    const bool apply_bitcrush = (bit_depth < 15.9f);
    const bool apply_drive = (drive > 0.01f);
    const float drive_mult = 1.0f + drive * 2.5f;

    const float tone_mix = 1.0f - snappy * 0.7f;
    const float noise_mix = snappy * 1.3f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float f1 = tone_freq + 60.0f * pitch_env;
        float f2 = tone_freq * 1.55f + 80.0f * pitch_env;
        pitch_env *= pitch_env_factor;

        phase1 += kTwoPi * f1 * dt;
        phase2 += kTwoPi * f2 * dt;
        if (phase1 > kTwoPi) phase1 -= kTwoPi;
        if (phase2 > kTwoPi) phase2 -= kTwoPi;

        env_tone *= tone_env_factor;
        float tone_signal = (std::sin(static_cast<float>(phase1)) * 0.6f +
        std::sin(static_cast<float>(phase2)) * 0.4f) * env_tone;

        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float filtered_noise = (b0 / a0) * raw_noise + (b2 / a0) * x2 - (a1 / a0) * y1 - (a2 / a0) * y2;
        x2 = x1; x1 = raw_noise;
        y2 = y1; y1 = filtered_noise;

        env_noise *= noise_env_factor;
        float noise_signal = filtered_noise * env_noise;

        float sample = tone_signal * tone_mix + noise_signal * noise_mix;

        if (apply_drive)
            sample = std::tanh(sample * drive_mult);

        if (apply_bitcrush)
            sample = std::round(sample * steps) / steps;

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_synth_sample(const std::string& filename, int session_samplerate, bool is_filename)
{
    std::string formula_str;

    if (is_filename)
        formula_str = string_file_load(filename);
    else
        formula_str = filename;

    if (formula_str.rfind("boom(", 0) == 0)
        return render_boom(formula_str, session_samplerate);

    if (formula_str.rfind("kick(", 0) == 0)
        return render_kick(formula_str, session_samplerate);

    if (formula_str.rfind("snare(", 0) == 0)
        return render_snare(formula_str, session_samplerate);

    if (formula_str.rfind("goasnare(", 0) == 0)
        return render_goasnare(formula_str, session_samplerate);

    if (formula_str.rfind("hihat_close(", 0) == 0 || formula_str.rfind("hihat_open(", 0) == 0)
        return render_hihat(formula_str, session_samplerate);

    if (formula_str.rfind("clap(", 0) == 0)
        return render_clap(formula_str, session_samplerate);

    if (formula_str.rfind("psykick(", 0) == 0)
        return render_psytrance_kick(formula_str, session_samplerate);

    if (formula_str.rfind("psytrance_kick(", 0) == 0)
        return render_psykick(formula_str, session_samplerate);

    if (formula_str.rfind("goakick(", 0) == 0)
        return render_goakick(formula_str, session_samplerate);

    if (formula_str.rfind("coldwave_snare(", 0) == 0)
        return render_coldwave_snare(formula_str, session_samplerate);

    if (formula_str.rfind("computer_tom(", 0) == 0)
        return render_computer_tom(formula_str, session_samplerate);

    if (formula_str.rfind("computer_snare(", 0) == 0)
        return render_computer_snare(formula_str, session_samplerate);

    if (formula_str.rfind("gabber_kick(", 0) == 0)
        return render_gabber_kick(formula_str, session_samplerate);

    if (formula_str.rfind("box(", 0) == 0)
        return render_box_sound(formula_str, session_samplerate);

    if (formula_str.rfind("cowbell(", 0) == 0)
        return render_cowbell_sound(formula_str, session_samplerate);

    if (formula_str.rfind("rimshot(", 0) == 0)
        return render_rimshot(formula_str, session_samplerate);

    if (formula_str.rfind("dnb_kick(", 0) == 0)
        return render_dnb_kick(formula_str, session_samplerate);

    if (formula_str.rfind("snare_fluffy(", 0) == 0)
        return render_snare_fluffy(formula_str, session_samplerate);

    if (formula_str.rfind("ride(", 0) == 0)
        return render_ride(formula_str, session_samplerate);

    if (formula_str.rfind("metal(", 0) == 0)
        return render_crash(formula_str, session_samplerate);

    if (formula_str.rfind("glass(", 0) == 0)
        return render_glass(formula_str, session_samplerate);

    if (formula_str.rfind("crash_hybrid(", 0) == 0)
        return render_crash_hybrid(formula_str, session_samplerate);

    if (formula_str.rfind("fart(", 0) == 0)
        return render_fart(formula_str, session_samplerate);

    return nullptr;
}
