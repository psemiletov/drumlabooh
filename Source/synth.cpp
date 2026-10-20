/*
 written at 2026 by Peter Semiletov with Gemini AI
 this code is the public domain
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
#include <sstream>
#include <map>
#include <cmath>


#include <juce_audio_basics/juce_audio_basics.h>



#include "utl.h"



// Константа размера таблицы (можно использовать для любых волновых таблиц)
constexpr int kSineTableSize = 2048;
//extern float g_sine_table[kSineTableSize + 1];

float g_sine_table[kSineTableSize + 1];


// Функция инициализации таблиц (вызывается один раз при старте приложения/плагина)
void init_synth_wavetables()
{
  for (int i = 0; i <= kSineTableSize; ++i)
      {
       g_sine_table[i] = std::sin (static_cast<float>(i) * (2.0f * 3.14159265f / static_cast<float> (kSineTableSize)));
      }
}

// Быстрая inline-функция для чтения из таблицы с линейной интерполяцией
inline float fast_sin (float phase)
{
  phase -= static_cast<int>(phase);

  if (phase < 0.0f)
     phase += 1.0f;

  float idx_f = phase * static_cast<float> (kSineTableSize);
  int idx = static_cast<int>(idx_f);
  float frac = idx_f - static_cast<float> (idx);

  return g_sine_table[idx] + frac * (g_sine_table[idx + 1] - g_sine_table[idx]);
}


void apply_buffer_fadeout (juce::AudioBuffer<float>* buffer, int samplerate, float fade_duration_sec = 0.003f)
{
  if (! buffer)
     return;

  int num_samples = buffer->getNumSamples();
  int fade_samples = static_cast<int> (samplerate * fade_duration_sec);

   // Если буфер слишком короткий, сглаживаем последнюю четверть
  if (fade_samples > num_samples / 2)
      fade_samples = num_samples / 2;

  if (fade_samples < 2)
      fade_samples = 2; // Минимум пара семплов

  float* data = buffer->getWritePointer (0);
  int start_fade_idx = num_samples - fade_samples;

  for (int i = 0; i < fade_samples; ++i)
      {
       // Линейный коэффициент от 1.0 до 0.0 на отрезке фейда
       float coeff = static_cast<float>(fade_samples - i) / static_cast<float>(fade_samples);
       data[start_fade_idx + i] *= coeff;
     }
}

/*
std::map<std::string, float> parse_synth_params (const std::string &formula_str)
{
    std::map<std::string, float> params;

    size_t start = formula_str.find ('(');
    size_t end = formula_str.rfind (')');
    if (start == std::string::npos || end == std::string::npos || end <= start)
    {
        return params; // Обязательные фигурные скобки исправляют warning о misleading-indentation
    }

    std::string inner = formula_str.substr (start + 1, end - start - 1);

    std::stringstream ss(inner);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        size_t eq = item.find ('=');
        if (eq != std::string::npos)
        {
            std::string key = item.substr(0, eq);
            std::string val_str = item.substr(eq + 1);

            try {
                float val = std::stof(val_str);
                key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
                params[key] = val;
            } catch (...) {}
        }
    }

    return params;
}
*/


std::map <std::string, float> parse_synth_params (const std::string &formula_str)
{
  std::map<std::string, float> params;

  size_t start = formula_str.find('(');
  size_t end   = formula_str.rfind(')');

  if (start == std::string::npos || end == std::string::npos || end <= start)
      return params;

  std::string inner = formula_str.substr (start + 1, end - start - 1);
  std::stringstream ss(inner);
  std::string item;

  while (std::getline(ss, item, ','))
        {
         // skip empty items (e.g. trailing commas)
         item = trim (item);

         if (item.empty())
             continue;

         size_t eq = item.find('=');
         if (eq == std::string::npos)
            continue;

        std::string key = item.substr (0, eq);
        std::string val_str = item.substr (eq + 1);

        key = trim (key);
        val_str = trim (val_str);

        // normalize key to lowercase
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        try {
             float val = std::stof(val_str); // supports "1.23", "2e3", leading/trailing spaces
             params[key] = val;
            } catch (...)
                    {
                     // ignore parse errors
                    }
    }

  return params;
}

// Вспомогательный генератор детерминированного шума (xorshift32)
inline float get_deterministic_noise (uint32_t &state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    // Сначала маска для uint32_t, потом каст в float
    return static_cast<float>(state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;
}


// --- Вспомогательный класс фильтра (оставляем без изменений, он отличный) ---
class Biquad
{
public:

  float b0=1, b1=0, b2=0, a1=0, a2=0;
  float z1=0, z2=0;

  void bandpass (float sr, float f0, float Q)
                {
                 float w0 = 2.0f * 3.14159265f * f0 / sr;
                 float cosw = std::cos(w0), sinw = std::sin(w0);
                 float alpha = sinw / (2.0f * Q);
                 float a0 = 1.0f + alpha;
                 b0 =  alpha / a0;
                 b1 =  0.0f;
                 b2 = -alpha / a0;
                 a1 = -2.0f * cosw / a0;
                 a2 = (1.0f - alpha) / a0;
                }


 float process (float x)
               {
                float y = b0*x + z1;
                z1 = b1*x - a1*y + z2;
                z2 = b2*x - a2*y;
                return y;
               }
};


juce::AudioBuffer<float>* render_boom_sound (const std::string &formula_str, int samplerate)
{
    float duration   = 0.35f;
    float base_freq  = 70.0f;
    float freq_sweep = -40.0f;
    float noise_amt  = 0.3f;
    float roughness  = 0.6f;
    float wobble_rate= 24.0f;
    float attack     = 0.001f;
    float release    = 0.12f;
    float gain       = 0.95f;
    float bubble_amt = 0.5f;

    float lpf_cutoff = 1400.0f;
    float hpf_cutoff = 45.0f;
    bool  granular   = true;
    float grain_mix  = 0.4f;

    std::map<std::string, float> p = parse_synth_params (formula_str);

    if (p.count("duration"))   duration   = p["duration"];
    if (p.count("freq"))       base_freq  = p["freq"];
    if (p.count("sweep"))      freq_sweep = p["sweep"];
    if (p.count("noise"))      noise_amt  = p["noise"];
    if (p.count("roughness"))  roughness  = p["roughness"];
    if (p.count("wobble"))     wobble_rate= p["wobble"];
    if (p.count("attack"))     attack     = p["attack"];
    if (p.count("release"))    release    = p["release"];
    if (p.count("gain"))       gain       = p["gain"];
    if (p.count("bubble"))     bubble_amt = p["bubble"];
    if (p.count("tone"))       lpf_cutoff = p["tone"];
    if (p.count("granular"))   granular   = (p["granular"] > 0.5f);
    if (p.count("grain_mix"))  grain_mix  = p["grain_mix"];

    int num_samples = static_cast<int>(samplerate * (duration * 1.1f));
    if (num_samples < 100)
        num_samples = 100;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    uint32_t noise_state = 13579;
    double phase1 = 0.0, phase2 = 0.0;
    float wobblePhase = 0.0f;
    float bubblePhase = 0.0f;

    float lpf_state = 0.0f;
    float hpf_prev_in = 0.0f;
    float hpf_prev_out = 0.0f;

    float dt = 1.0f / static_cast<float>(samplerate);

    float rc_lpf = 1.0f / (2.0f * M_PI * lpf_cutoff);
    float alpha_lpf = dt / (rc_lpf + dt);

    float rc_hpf = 1.0f / (2.0f * M_PI * hpf_cutoff);
    float alpha_hpf = rc_hpf / (rc_hpf + dt);

    float micro_burst_env = 0.0f;
    float micro_burst_phase = 0.0f;
    int   micro_burst_timer = 0;
    int   next_burst_samples = static_cast<int>(samplerate * 0.02f);

    float body_env_factor = std::exp(-dt / release);
    float noise_env_factor = std::exp(-dt / (release * 0.5f));
    float body_env = 1.0f;
    float noise_env = 1.0f;

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

        float kick_drop = std::exp (-t * 70.0f);
        float freq1 = base_freq + (base_freq * 3.0f * kick_drop) + freq_sweep * u;
        float freq2 = base_freq * 0.5f + freq_sweep * 0.8f * u;

        float wobble = std::sin (wobblePhase) * roughness * 0.7f;
        wobblePhase += 2.0f * M_PI * wobble_rate * dt;
        freq1 *= (1.0f + wobble);

        phase1 += 2.0 * M_PI * freq1 * dt;
        phase2 += 2.0 * M_PI * freq2 * dt;
        float tone_sig = std::sin (phase1) * 0.75f + (std::sin (phase2 * 2.0f) * 0.25f);

        float bubble = 1.0f + bubble_amt * std::sin (bubblePhase) * std::cos (bubblePhase * 0.4f);
        bubblePhase += 2.0f * M_PI * 14.0f * dt;

        float raw_noise = get_deterministic_noise (noise_state);

        float granular_sig = 0.0f;
        if (granular)
           {
            micro_burst_timer++;
            if (micro_burst_timer >= next_burst_samples && u < 0.75f)
               {
                micro_burst_env = 1.0f;
                micro_burst_timer = 0;
                float jitter = get_deterministic_noise (noise_state);
                next_burst_samples = static_cast<int>(samplerate * (0.012f + std::abs(jitter) * 0.015f));
               }

            if (micro_burst_env > 0.001f)
               {
                micro_burst_phase += 2.0 * M_PI * (base_freq * 1.6f) * dt;
                granular_sig = std::sin (micro_burst_phase) * micro_burst_env;
                micro_burst_env *= std::exp (-dt * 70.0f);
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

        float s = std::tanh (hpf_out * 2.2f);
        data[i] = s;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_gabber_kick (const std::string &formula_str, int samplerate)
{
    float f0 = 220.0f;
    float f1 = 42.0f;
    float decay = 0.38f;
    float drive = 0.8f;
    float tone = 500.0f;

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("f0"))    f0 = p["f0"];
    if (p.count("f1"))    f1 = p["f1"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("drive")) drive = p["drive"];
    if (p.count("tone"))  tone = p["tone"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    double phase = 0.0;

    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (2.0f * 3.14159265f * tone);
    float alpha = dt / (rc + dt);

    // Оптимизация огибающей затухания
    float env_factor = std::exp (-dt * (4.0f / decay));
    float env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = f1 + (f0 - f1) * std::exp (-t * 32.0f);
        phase += 2.0 * M_PI * freq * dt;

        float raw_sine = std::sin (phase);

        float gain = 1.0f + (drive * 12.0f) * std::exp (-t * 15.0f);
        float distorted = std::tanh (raw_sine * gain);

        lpf_state = lpf_state + alpha * (distorted - lpf_state);

        float mixed = (raw_sine * 0.3f) + (lpf_state * 0.7f);

        env *= env_factor;
        float sample = mixed * env;

        sample = std::tanh (sample * 1.1f);

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_computer_snare (const std::string &formula_str, int samplerate)
{
    float freq = 380.0f;
    float decay = 0.14f;
    float crunch = 0.55f;
    float tone = 1100.0f;

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("freq"))   freq = p["freq"];
    if (p.count("decay"))  decay = p["decay"];
    if (p.count("crunch")) crunch = p["crunch"];
    if (p.count("tone"))   tone = p["tone"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    double phase = 0.0;
    uint32_t comp_snare_state = 7777;

    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (2.0f * 3.14159265f * tone);
    float alpha = dt / (rc + dt);

    float body_env_factor = std::exp(-dt * (6.0f / decay));
    float body_env = 1.0f;

    float noise_env_factor = std::exp(-dt * (9.0f / decay));
    float noise_env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;
        float sample = 0.0f;

        if (t < decay)
        {
            float current_freq = freq + freq * 0.5f * std::exp(-t * 60.0f);
            phase += 2.0 * M_PI * current_freq * dt;
            float square = (std::sin(phase) >= 0.0f) ? 1.0f : -1.0f;

            body_env *= body_env_factor;
            sample += square * body_env * 0.45f;
        }

        comp_snare_state ^= comp_snare_state << 13;
        comp_snare_state ^= comp_snare_state >> 17;
        comp_snare_state ^= comp_snare_state << 5;
        float raw_noise = static_cast<float>(comp_snare_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        noise_env *= noise_env_factor;
        float current_n_env = noise_env;
        if (t < 0.0015f) current_n_env *= (t / 0.0015f);

        sample += lpf_state * current_n_env * crunch;

        sample = std::tanh (sample * 1.4f);

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_computer_tom (const std::string &formula_str, int samplerate)
{
    float base_freq = 150.0f;
    float decay = 0.35f;
    float click_mix = 0.2f;

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("freq"))   base_freq = p["freq"];
    if (p.count("decay"))  decay = p["decay"];
    if (p.count("click"))  click_mix = p["click"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    double phase = 0.0;
    double click_phase = 0.0;

    float dt = 1.0f / static_cast<float>(samplerate);
    float env_factor = std::exp(-dt * (4.0f / decay));
    float env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = base_freq + base_freq * 1.5f * std::exp (-t * 30.0f);
        phase += 2.0 * M_PI * freq * dt;

        env *= env_factor;
        float sample = std::sin (phase) * env;

        if (t < 0.008f)
        {
            click_phase += 2.0 * M_PI * (base_freq * 3.0f) * dt;
            float square_click = (std::sin(click_phase) >= 0.0f) ? 1.0f : -1.0f;
            float click_env = (1.0f - t / 0.008f);
            sample += square_click * click_mix * click_env;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_coldwave_snare (const std::string &formula_str, int samplerate)
{
    float f0 = 210.0f;
    float f1 = 115.0f;
    float decay = 0.17f;
    float crunch = 0.5f;
    float tone = 900.0f;

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("f0"))      f0 = p["f0"];
    if (p.count("f1"))      f1 = p["f1"];
    if (p.count("decay"))   decay = p["decay"];
    if (p.count("crunch"))  crunch = p["crunch"];
    if (p.count("tone"))    tone = p["tone"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer(0);

    double phase = 0.0;
    uint32_t cw_noise_state = 43210;

    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (2.0f * 3.14159265f * tone);
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
            float freq = f1 + (f0 - f1) * std::exp (-t * 40.0f);
            phase += 2.0 * M_PI * freq * dt;
            body_env *= body_env_factor;
            sample += std::sin (phase) * body_env * 0.5f;
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

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_psytrance_kick (const std::string &formula_str, int samplerate)
{
    float f0 = 240.0f;
    float f1 = 38.0f;
    float decay = 0.22f;
    float click_level = 0.35f;

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("f0"))      f0 = p["f0"];
    if (p.count("f1"))      f1 = p["f1"];
    if (p.count("decay"))   decay = p["decay"];
    if (p.count("click"))   click_level = p["click"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    double phase = 0.0;
    uint32_t click_noise_state = 9999;

    float dt = 1.0f / static_cast<float>(samplerate);
    float env_factor = std::exp(-dt * (4.0f / decay));
    float env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = f1 + (f0 - f1) * std::exp (-t * 35.0f);
        phase += 2.0 * M_PI * freq * dt;

        env *= env_factor;
        float sample = std::sin (phase) * env;

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

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_hihat (const std::string &formula_str, int samplerate)
{
    float decay = 0.08f;
    float tone = 7000.0f;
    float noise_mix = 0.5f;
    float metal_mix = 0.5f;
    float pitch = 1.0f;
    float detune = 0.0f;
    float drive = 1.0f;

    bool is_open = (formula_str.rfind ("hihat_open(", 0) == 0);
    if (is_open)
    {
        decay = 0.35f;
    }

    auto params = parse_synth_params (formula_str);
    if (params.count("decay"))   decay = params["decay"];
    if (params.count("tone"))    tone = params["tone"];
    if (params.count("noise"))   noise_mix = params["noise"];
    if (params.count("metal"))   metal_mix = params["metal"];
    if (params.count("pitch"))   pitch = params["pitch"];
    if (params.count("detune"))  detune = params["detune"];
    if (params.count("drive"))   drive = params["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 54321;

    float base_metal_freqs[6] = { 405.3f, 535.1f, 723.4f, 845.2f, 1150.0f, 1420.0f };
    double metal_phases[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    float hpf_prev_input = 0.0f;
    float hpf_prev_output = 0.0f;

    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (2.0f * M_PI * tone);
    float alpha = rc / (rc + dt);

    float env_factor = std::exp(-dt * (3.0f / decay));
    float env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float metal_sample = 0.0f;
        for (int m = 0; m < 6; ++m)
        {
            float freq_mod = base_metal_freqs[m] * (1.0f + static_cast<float>(m) * detune * 0.15f);
            metal_phases[m] += (freq_mod * pitch) * dt;
            if (metal_phases[m] >= 1.0) metal_phases[m] -= 1.0;

            float sig = fast_sin(static_cast<float>(metal_phases[m]));
            metal_sample += (sig >= 0.0f ? 1.0f : -1.0f);
        }
        metal_sample /= 6.0f;

        float noise_sample = get_deterministic_noise(noise_state);
        float raw_mix = (metal_sample * metal_mix) + (noise_sample * noise_mix);

        float hpf_output = alpha * (hpf_prev_output + raw_mix - hpf_prev_input);
        hpf_prev_input = raw_mix;
        hpf_prev_output = hpf_output;

        env *= env_factor;
        float current_env = env;
        if (t < 0.002f) {
            current_env *= (t / 0.002f);
        }

        float sample = hpf_output * current_env;

        if (drive > 1.0f) {
            sample = std::tanh(sample * drive);
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_clap (const std::string &formula_str, int session_samplerate)
{
    float decay = 0.28f;
    float tone = 1200.0f;
    float spread = 0.011f;
    int burst_count = 4;

    auto params = parse_synth_params (formula_str);
    if (params.count("decay"))  decay = params["decay"];
    if (params.count("tone"))   tone = params["tone"];
    if (params.count("spread")) spread = params["spread"];
    if (params.count("count"))  burst_count = static_cast<int>(params["count"]);

    int num_samples = static_cast<int>(session_samplerate * (decay * 1.2f));
    if (num_samples < 200) num_samples = 200;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 77777;

    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(session_samplerate);

    float rc = 1.0f / (2.0f * 3.14159265f * tone);
    float alpha = dt / (rc + dt);

    float tail_env_factor = std::exp(-dt * (3.5f / decay));
    float tail_env = 1.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float noise = get_deterministic_noise(noise_state);

        float burst_env = 0.0f;
        for (int b = 0; b < burst_count; ++b)
        {
            float burst_time = static_cast<float>(b) * spread;
            if (t >= burst_time)
            {
                float dt_burst = t - burst_time;
                burst_env += std::exp(-dt_burst * 400.0f);
            }
        }

        tail_env *= tail_env_factor;

        float total_env = (burst_env * 0.8f) + (tail_env * 0.2f);

        lpf_state = lpf_state + alpha * (noise - lpf_state);

        data[i] = lpf_state * total_env * 1.2f;
    }

    apply_buffer_fadeout (buffer, session_samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_fart (const std::string &formula_str, int session_samplerate)
{
    // Параметры по умолчанию
    float decay = 0.45f;       // Длительность (в секундах)
    float tone = 650.0f;       // Частота фильтра (чем ниже, тем глуше/басовее)
    float spread = 0.022f;     // Интервал между импульсами срыва (период трепыхания)
    int burst_count = 8;       // Количество импульсов
    float pitch = 45.0f;       // Низкочастотный резонанс ("сочность"/бубнение, Гц)
    float wet = 1.2f;          // Сатурация / нелинейная плотность

    auto params = parse_synth_params (formula_str);
    if (params.count("decay"))       decay = params["decay"];
    if (params.count("tone"))        tone = params["tone"];
    if (params.count("spread"))      spread = params["spread"];
    if (params.count("count"))       burst_count = static_cast<int>(params["count"]);
    if (params.count("pitch"))       pitch = params["pitch"];
    if (params.count("freq"))        pitch = params["freq"]; // Альтернативный алиас
    if (params.count("wet"))         wet = params["wet"];

    // Защита диапазона параметров
    if (burst_count < 1) burst_count = 1;
    if (tone < 80.0f) tone = 80.0f;

    int num_samples = static_cast<int>(session_samplerate * (decay * 1.2f));
    if (num_samples < 200) num_samples = 200;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 77777;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(session_samplerate);

    // Расчет коэффициента LPF
    float rc = 1.0f / (2.0f * 3.14159265f * tone);
    float alpha = dt / (rc + dt);

    double phase = 0.0;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Серия импульсов давления (burst envelope)
        float burst_env = 0.0f;
        for (int b = 0; b < burst_count; ++b)
        {
            float burst_time = static_cast<float>(b) * spread;
            if (t >= burst_time)
            {
                float dt_burst = t - burst_time;
                // Более плавное спадание каждого всплеска для объема
                burst_env += std::exp(-dt_burst * 120.0f);
            }
        }

        // Общая затухающая огибающая
        float main_env = std::exp(-t * (3.0f / decay));

        // 2. Генерация отфильтрованного шума
        float noise = get_deterministic_noise(noise_state);
        lpf_state = lpf_state + alpha * (noise - lpf_state);

        // 3. Низкочастотная тональная составляющая (Pitch drop)
        // В начале атаки частота чуть выше, затем опускается к чистой pitch
        float current_pitch = pitch + pitch * 0.5f * std::exp(-t * 25.0f);
        phase += 2.0 * 3.14159265 * current_pitch * dt;
        float sub_osc = std::sin(phase) * main_env;

        // 4. Смешивание шумовых импульсов и суб-осциллятора
        float combined_signal = (lpf_state * burst_env * 0.7f) + (sub_osc * 0.4f);

        // 5. Итоговая огибающая и ограничение (tanh для "влажной" сатурации)
        float sample = combined_signal * main_env;
        if (wet > 0.1f)
        {
            sample = std::tanh(sample * (1.0f + wet));
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, session_samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_snare (const std::string &formula_str, int samplerate)
{
    float f0 = 200.0f;
    float f1 = 110.0f;
    float decay = 0.18f;
    float noise_level = 0.6f;
    float noise_decay = 0.12f;

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("f0"))       f0 = p["f0"];
    if (p.count("f1"))       f1 = p["f1"];
    if (p.count("decay"))    decay = p["decay"];
    if (p.count("noise"))    noise_level = p["noise"];
    if (p.count("n_decay"))  noise_decay = p["n_decay"];

    int num_samples = static_cast<int>(samplerate * std::max(decay, noise_decay) * 1.1f);
    if (num_samples <= 0)
        num_samples = static_cast<int>(samplerate * 0.25f);

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

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
            float freq = f1 + (f0 - f1) * std::exp (-t * 45.0f);
            phase += 2.0 * M_PI * freq * dt;

            body_env *= body_env_factor;
            sample += std::sin (phase) * body_env * 0.45f;
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

        data[i] = std::tanh (sample * 1.3f);
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_kick (const std::string &formula_str, int samplerate)
{


    float f0 = 150.0f;
    float f1 = 45.0f;
    float decay = 0.35f;
    float click_level = 0.2f;


    // Парсим параметры из строки
    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("f0"))    f0 = p["f0"];
    if (p.count("f1"))    f1 = p["f1"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("click")) click_level = p["click"];


    int num_samples = static_cast<int>(samplerate * decay);
    if (num_samples <= 0)
        num_samples = samplerate * 0.5f;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    double phase = 0.0;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) / samplerate;

        float freq = f1 + (f0 - f1) * std::exp (-t * 20.0f);
        phase += 2.0 * M_PI * freq / samplerate;

        float env = std::exp (-t * (3.0f / decay));
        float sample = std::sin (phase) * env;

        if (t < 0.005f) {
            float click = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * (1.0f - t / 0.005f);
            sample += click * click_level;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f); // 3 мс на сглаживание хвоста
    return buffer;
}


juce::AudioBuffer<float>* render_box_sound (const std::string &formula_str, int samplerate)
{
    // Физиологические дефолты
    float duration   = 0.32f;
    float base_freq  = 55.0f;   // Низкая база для вибрирующего мяса/тканей
    float freq_sweep = -25.0f;  // Плавное падение давления
    float noise_mix  = 0.7f;    // Основной объем — коричневый шум воздуха
    float bass_mix   = 0.4f;    // Плотный суб-басовый гул
    float vibrato_r  = 22.0f;   // Частота вибрато сфинктера (Гц)
    float vibrato_d  = 0.5f;    // Глубина вибрато
    float squeak_mix = 0.25f;   // Высокий свист/писк струи воздуха на старте
    float attack     = 0.002f;  // Быстрое открытие клапана
    float release    = 0.14f;   // Затухание давления
    float gain       = 0.95f;

    // Параметры фильтрации воздуха
    float cutoff_min = 120.0f;  // Нижняя точка фильтра
    float cutoff_max = 900.0f;  // Верхняя точка (пик давления)

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("duration"))   duration   = p["duration"];
    if (p.count("freq"))       base_freq  = p["freq"];
    if (p.count("sweep"))      freq_sweep = p["sweep"];
    if (p.count("noise"))      noise_mix  = p["noise"];
    if (p.count("bass"))       bass_mix   = p["bass"];
    if (p.count("vibrato"))    vibrato_r  = p["vibrato"];
    if (p.count("squeak"))     squeak_mix = p["squeak"];
    if (p.count("attack"))     attack     = p["attack"];
    if (p.count("release"))    release    = p["release"];
    if (p.count("gain"))       gain       = p["gain"];

    int num_samples = static_cast<int>(samplerate * (duration * 1.1f));
    if (num_samples < 100) num_samples = 100;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    uint32_t noise_state = 98765;
    double bass_phase = 0.0;
    double squeak_phase = 0.0;
    float vibrato_phase = 0.0f;

    // Переменные для генерации коричневого шума (интегрированный белый шум)
    float brown_noise = 0.0f;

    // Состояния фильтров
    float lpf_state = 0.0f;
    float hpf_prev_in = 0.0f;
    float hpf_prev_out = 0.0f;

    float dt = 1.0f / static_cast<float>(samplerate);
    float hpf_cutoff = 35.0f; // Отрезаем инфранизкий мусор
    float rc_hpf = 1.0f / (2.0f * M_PI * hpf_cutoff);
    float alpha_hpf = rc_hpf / (rc_hpf + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;
        float u = t / duration;

        // Огибающая давления воздуха (быстрый старт, плавный спад)
        float env = (t < attack)
        ? (t / attack)
        : std::exp (-(t - attack) / release);

        // Дополнительный акцент на самое начало (порция сжатого воздуха)
        float pressure_env = env * (1.0f + 1.5f * std::exp(-t * 25.0f));

        // Быстрое вибрато (модуляция частоты колебания сфинктера)
        float vibrato = std::sin (vibrato_phase) * vibrato_d;
        vibrato_phase += 2.0f * M_PI * vibrato_r * dt;

        // Питч-контур баса
        float current_freq = base_freq + freq_sweep * u;
        current_freq *= (1.0f + vibrato);

        // 1. Басовая составляющая (резонанс полости / плоть)
        bass_phase += 2.0 * M_PI * current_freq * dt;
        float bass_wave = std::sin (bass_phase);
        // Добавляем гармонику для жира
        bass_wave += std::sin (bass_phase * 2.0f) * 0.4f;

        // 2. Коричневый шум (воздушный поток через сужение)
        float white = get_deterministic_noise (noise_state);
        // Интегрируем белый шум в коричневый с мягким затуханием (leak)
        brown_noise = brown_noise * 0.92f + white * 0.28f;

        // 3. Высокий свист (сжатая струя в узком сопле на старте)
        float squeak = 0.0f;
        if (u < 0.4f)
        {
            float squeak_freq = 1800.0f + 600.0f * std::sin(u * 15.0f);
            squeak_phase += 2.0 * M_PI * squeak_freq * dt;
            // Свист модулируется шумом, чтобы звучать органично, а не как чистый синус
            squeak = std::sin (squeak_phase) * (white * 0.5f + 0.5f) * (1.0f - u / 0.4f) * squeak_mix;
        }

        // 4. Динамический фильтр низких частот (зависит от давления воздуха)
        // Когда давление максимальное (в начале), фильтр шире открывается, выпуская больше средних частот
        float dynamic_cutoff = cutoff_min + (cutoff_max - cutoff_min) * pressure_env;
        float rc_lpf = 1.0f / (2.0f * M_PI * dynamic_cutoff);
        float alpha_lpf = dt / (rc_lpf + dt);

        // Смешиваем шум и бас
        float mixed = (brown_noise * noise_mix) + (bass_wave * bass_mix * 0.5f) + squeak;
        mixed *= pressure_env;

        // Применяем динамический LPF
        lpf_state = lpf_state + alpha_lpf * (mixed - lpf_state);

        // Применяем статический HPF (убирает постоянку и бубнение)
        float hpf_out = alpha_hpf * (hpf_prev_out + lpf_state - hpf_prev_in);
        hpf_prev_in = lpf_state;
        hpf_prev_out = hpf_out;

        // Мягкий аналоговый сатуратор (имитация перегруза тканей давлением)
        float s = std::tanh (hpf_out * 2.5f) * gain;

        data[i] = s;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}

/*

  Давай разберем каждый параметр в формуле kick(f0=160; f1=42; decay=0.38; click=0.25):

  f0 (Start Frequency / Начальная частота) = 160 Гц
  Это частота, с которой «стартует» синусоидальный осциллятор в самый первый момент удара. Она задает первичный щелчок, плотный «удар в грудь» или тело бочки. Чем выше это значение, тем более «высокой» или пробивной по атаке кажется бочка (например, для электронных или пробивных бочек ставят 180–220 Гц, для глубоких 808-х — ниже, около 120–130 Гц).

  f1 (End Frequency / Конечная частота) = 42 Гц
  Это то, куда «падает» частота по мере затухания звука. Это фундаментальный тон, басовый хвост бочки (гул). Значение в районе 40–50 Гц — это классический суббасовый фундамент для плотной акустической или клубной бочки.

  decay (Затухание / Длительность) = 0.38 секунд
  Общее время звучания сэмпла в секундах. Оно управляет тем, как быстро спадает амплитудная огибающая (env). Значение 0.38 — это средний хвост (примерно 380 миллисекунд), достаточно плотный, но уже затухающий, чтобы не забивать микс.

  click (Уровень щелчка / Атака шумом) = 0.25
  Интенсивность шумового компонента в первые миллисекунды (t < 0.005f). Он имитирует удар колотушки о пластик барабана. Коэффициент 0.25 добавляет легкий, отчетливый щелчок (25% от максимальной громкости шумовой добавки), чтобы бочка читалась даже на маленьких колонках.

 */

/*
 К ратко о параметрах snare:                                                        *
 f0 / f1: Частоты удара пластика (выше, чем у бочки).

 decay: Как быстро затухает «стук» дерева/пластика.

 noise: Интенсивность песочного шуршания пружин.

 n_decay: Длина хвоста пружин (делает малый барабан более «воздушным» или плотным).
*/

/*
 П редлагаемый синтаксис для .syn файлов                                                                      *
 Закрытый хет (короткий):
 hihat(decay=0.05, tone=7000, noise=0.5, metal=0.5)

 Открытый хет (длинный):
 hihat(decay=0.45, tone=6000, noise=0.6, metal=0.4)

 2. Набор параметров (аргументов)
 Давайте заложим стандартные параметры, которые легко парсить нашим запятым парсером и которые дают полный контроль над звуком:

 decay — время затухания в секундах (например, 0.05 для closed, 0.4 для open).

 tone — частота среза фильтра высоких частот (HPF) в герцах (по умолчанию около 7000 Гц, чтобы убрать грязь и оставить чистый «цик»).

 noise — доля белого шума в миксе (от 0.0 до 1.0).

 metal — доля «металлического» осцилляторного гудящего тона (от 0.0 до 1.0).

 pitch — базовый множитель частоты для металлических осцилляторов (если хочется сделать хет «выше» или «ниже»).


 */


juce::AudioBuffer<float>* render_cowbell_sound (const std::string &formula_str, int samplerate)
{

    float base_freq = 560.0f;
    float ratio     = 1.45f;
    float decay     = 0.18f;
    float tone      = 1200.0f;
    float drive     = 1.2f;

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("freq"))   base_freq = p["freq"];
    if (p.count("ratio"))  ratio     = p["ratio"];
    if (p.count("decay"))  decay     = p["decay"];
    if (p.count("tone"))   tone      = p["tone"];
    if (p.count("drive"))  drive     = p["drive"];


    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

//    std::cout << "[DEBUG] allocating audio buffer, num_samples=" << num_samples << "\n";
    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);

    if (! buffer)
    {
        std::cerr << "[ERROR] Failed to allocate audio buffer for cowbell!\n";
        return nullptr;
    }

    float *data = buffer->getWritePointer (0);

    double phase1 = 0.0;
    double phase2 = 0.0;

    float dt = 1.0f / static_cast<float>(samplerate);

    float lpf_state = 0.0f;
    float rc = 1.0f / (2.0f * M_PI * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq1 = base_freq;
        float freq2 = base_freq * ratio;

        phase1 += 2.0 * M_PI * freq1 * dt;
        phase2 += 2.0 * M_PI * freq2 * dt;

        float sq1 = (std::sin(phase1) >= 0.0f) ? 1.0f : -1.0f;
        float sq2 = (std::sin(phase2) >= 0.0f) ? 1.0f : -1.0f;

        float raw_signal = (sq1 * 0.5f) + (sq2 * 0.5f);

        lpf_state = lpf_state + alpha * (raw_signal - lpf_state);

        float env = std::exp (-t * (4.0f / decay));

        float click = 0.0f;
        if (t < 0.0015f) {
            click = (1.0f - t / 0.0015f) * 0.4f;
        }

        float sample = (lpf_state + click) * env;

        if (drive > 1.0f) {
            sample = std::tanh (sample * drive);
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);

    return buffer;
}


juce::AudioBuffer<float>* render_rimshot (const std::string &formula_str, int samplerate)
{
    float freq   = 440.0f; // Основная частота резонанса обода
    float decay  = 0.12f;  // Общая длительность
    float click  = 0.7f;   // Интенсивность щелчка обода
    float tone   = 2500.0f;// Частота фильтра для щелчка
    float drive  = 1.4f;   // Сатурация

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("freq"))  freq  = p["freq"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("click")) click = p["click"];
    if (p.count("tone"))  tone  = p["tone"];
    if (p.count("drive")) drive = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float> (1, num_samples);
    float* data = buffer->getWritePointer (0);

    double phase = 0.0;
    uint32_t rim_noise_state = 54321;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);

    float rc = 1.0f / (2.0f * M_PI * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Тональное тело обода (высокий затухающий тон с питч-огибающей)
        float current_freq = freq + freq * 0.8f * std::exp (-t * 80.0f);
        phase += 2.0 * M_PI * current_freq * dt;
        float body_env = std::exp (-t * (5.0f / decay));
        float body = std::sin (phase) * body_env * 0.4f;

        // 2. Ультра-короткий деревянный/металлический щелчок на атаке
        rim_noise_state ^= rim_noise_state << 13;
        rim_noise_state ^= rim_noise_state >> 17;
        rim_noise_state ^= rim_noise_state << 5;
        float raw_noise = static_cast<float>(rim_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        float click_env = 0.0f;
        if (t < 0.003f) {
            click_env = (1.0f - t / 0.003f);
        } else {
            click_env = std::exp (-t * 30.0f) * 0.15f;
        }

        float rim_click = lpf_state * click_env * click;

        // Суммируем тело и щелчок
        float sample = body + rim_click;

        if (drive > 1.0f) {
            sample = std::tanh (sample * drive);
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_dnb_kick (const std::string &formula_str, int samplerate)
{
    float start_freq = 150.0f; // Стартовая частота питч-огибающей
    float end_freq   = 45.0f;  // Конечная саб-частота
    float pitch_time = 0.035f; // Скорость падения питча (ударность)
    float decay      = 0.35f;  // Общая длительность
    float click      = 1.5f;   // Интенсивность щелчка атаки
    float drive      = 2.0f;   // Жесткий индастриал-перегруз

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("start_freq")) start_freq = p["start_freq"];
    if (p.count("end_freq"))   end_freq   = p["end_freq"];
    if (p.count("pitch_time")) pitch_time = p["pitch_time"];
    if (p.count("decay"))      decay      = p["decay"];
    if (p.count("click"))      click      = p["click"];
    if (p.count("drive"))      drive      = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float> (1, num_samples);
    float* data = buffer->getWritePointer (0);

    double phase = 0.0;
    float dt = 1.0f / static_cast<float>(samplerate);
    uint32_t noise_state = 13579;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // Экспоненциальная питч-огибающая для плотного удара в грудь
        float freq = end_freq + (start_freq - end_freq) * std::exp (-t / pitch_time);
        phase += 2.0 * M_PI * freq * dt;

        // Основной саб-корпус бочки
        float env = std::exp (-t * (3.0f / decay));
        float sub = std::sin (phase) * env;

        // Транзиентный щелчок (мощный клик на атаке для прорезания микса)
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float click_env = 0.0f;
        if (t < 0.004f) {
            click_env = (1.0f - t / 0.004f) * click;
        }
        float trans = noise * click_env;

        // Смешиваем саб и транзиент
        float sample = sub + trans;

        // Индастриал-сатурация (мягкий/жесткий клиппинг)
        sample = std::tanh (sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.005f);
    return buffer;
}


juce::AudioBuffer<float>* render_dnb_snare (const std::string &formula_str, int samplerate)
{
    float body_freq = 210.0f; // Частота тонального тела малого
    float decay     = 0.25f;  // Длительность хвоста
    float snap      = 2.0f;   // Количество «пружин» (сеточки)
    float tone      = 3500.0f;// Частота фильтра для шума сеточки
    float drive     = 1.8f;   // Плотность и компрессия

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("body_freq")) body_freq = p["body_freq"];
    if (p.count("decay"))     decay     = p["decay"];
    if (p.count("snap"))      snap      = p["snap"];
    if (p.count("tone"))      tone      = p["tone"];
    if (p.count("drive"))     drive     = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float> (1, num_samples);
    float* data = buffer->getWritePointer (0);

    double phase = 0.0;
    uint32_t noise_state = 98765;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);

    float rc = 1.0f / (2.0f * M_PI * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Тональное тело с быстрым падением частоты
        float cur_freq = body_freq + body_freq * 0.5f * std::exp (-t * 50.0f);
        phase += 2.0 * M_PI * cur_freq * dt;
        float body_env = std::exp (-t * (6.0f / decay));
        float body = std::sin (phase) * body_env * 0.6f;

        // 2. Шум сеточки (snare wires) с фильтрацией
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        // Огибающая сеточки: резкий удар + длинный шлейф
        float wire_env = std::exp (-t * (4.0f / decay)) * snap;
        // Добавляем акцент на самый первый момент удара
        if (t < 0.002f) {
            wire_env *= 2.5f;
        }
        float wires = lpf_state * wire_env * 0.5f;

        // Суммируем тело пластика и сеточку
        float sample = body + wires;

        // Перегруз для фирменного плотного звучания
        sample = std::tanh (sample * drive);

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.004f);
    return buffer;
}


juce::AudioBuffer<float>* render_snare_fluffy (const std::string &formula_str, int samplerate)
{
    float body_freq  = 190.0f; // Основная частота тела (150-250 Гц)
    float decay      = 0.22f;  // Общая длительность
    float snap       = 2.2f;   // Интенсивность и плотность сеточки
    float fm_amount  = 1.5f;   // Агрессивность FM-щелчка на атаке
    float tone       = 4200.0f;// Частота среза шума
    float drive      = 2.0f;   // Плотность и сатурация

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("freq"))      body_freq = p["freq"];
    if (p.count("body_freq")) body_freq = p["body_freq"];
    if (p.count("decay"))     decay     = p["decay"];
    if (p.count("snap"))      snap      = p["snap"];
    if (p.count("fm"))        fm_amount = p["fm"];
    if (p.count("tone"))      tone      = p["tone"];
    if (p.count("drive"))     drive     = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float> (1, num_samples);
    float* data = buffer->getWritePointer (0);

    double phase1 = 0.0, phase2 = 0.0, fm_phase = 0.0;
    uint32_t noise_state = 0xACE1u;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);

    float rc = 1.0f / (2.0f * M_PI * tone);
    float alpha = dt / (rc + dt);

    const int delay_len = static_cast<int>(samplerate * 0.002f); // 2 мс микро-резонатор
    std::vector<float> noise_delay(delay_len > 0 ? delay_len : 1, 0.0f);
    int delay_idx = 0;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Питч-огибающая для тела
        float pitch_env = std::exp (-t * 60.0f);
        float cur_freq = body_freq + (body_freq * 0.7f * pitch_env);

        // 2. FM-модуляция на атаке (первые 8 мс)
        float fm_env = std::exp (-t * 250.0f) * fm_amount;
        fm_phase += 2.0 * M_PI * (cur_freq * 2.25f) * dt;
        float fm_mod = std::sin (fm_phase) * fm_env;

        // 3. Двухгармоническое тело (фундаментал + вторая гармоника)
        phase1 += 2.0 * M_PI * cur_freq * dt + fm_mod;
        phase2 += 2.0 * M_PI * (cur_freq * 1.6f) * dt;

        float body_env = std::exp (-t * (5.5f / decay));
        float body = (std::sin (phase1) * 0.6f + std::sin (phase2) * 0.3f) * body_env;

        // 4. Диффузный шум сеточки
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        float delayed_noise = noise_delay[delay_idx];
        noise_delay[delay_idx] = lpf_state;
        delay_idx = (delay_idx + 1) % noise_delay.size();

        float dense_noise = lpf_state + (delayed_noise * 0.4f);

        float snap_env = std::exp (-t * (4.2f / decay)) * snap;
        if (t < 0.003f) {
            snap_env *= 2.2f;
        }
        float wires = dense_noise * snap_env * 0.45f;

        float sample = body + wires;
        if (drive > 1.0f) {
            sample = std::tanh (sample * drive);
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.004f);
    return buffer;
}


juce::AudioBuffer<float>* render_ride_old (const std::string &formula_str, int samplerate)
{
    float freq   = 600.0f;  // Базовая частота колокола/тела
    float decay  = 1.5f;    // Длительность сустейна
    float bell   = 0.6f;    // Интенсивность звона колокола
    float sizzle = 0.8f;    // Плотность шипящего шлейфа
    float drive  = 1.2f;    // Легкая сатурация

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("freq"))   freq   = p["freq"];
    if (p.count("decay"))  decay  = p["decay"];
    if (p.count("bell"))   bell   = p["bell"];
    if (p.count("sizzle")) sizzle = p["sizzle"];
    if (p.count("drive"))  drive  = p["drive"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float> (1, num_samples);
    float* data = buffer->getWritePointer (0);

    uint32_t noise_state = 0x51DE123u;
    double p1 = 0, p2 = 0, p3 = 0, p4 = 0;
    float dt = 1.0f / static_cast<float>(samplerate);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // Негармонические металлические осцилляторы для звона райда
        p1 += 2.0 * M_PI * (freq * 1.0f) * dt;
        p2 += 2.0 * M_PI * (freq * 1.414f) * dt;
        p3 += 2.0 * M_PI * (freq * 2.718f) * dt;
        p4 += 2.0 * M_PI * (freq * 3.141f) * dt;

        float metal = (std::sin(p1) + std::sin(p2) + std::sin(p3) + std::sin(p4)) * 0.25f;

        // Шум для шипения (sizzle wash)
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float bell_env = std::exp(-t * 25.0f) * bell;
        float wash_env = std::exp(-t * (2.0f / decay)) * sizzle;

        float sample = (metal * bell_env + noise * wash_env);
        if (drive > 1.0f) {
            sample = std::tanh(sample * drive);
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.01f);
    return buffer;
}


juce::AudioBuffer<float>* render_metal (const std::string &formula_str, int samplerate)
{
    float decay  = 1.2f;    // Уменьшили дефолтную длительность (было 2.2f)
    float metal  = 750.0f;  // Базовая частота металла
    float tone   = 5500.0f; // Яркость фильтра
    float drive  = 1.3f;    // Мягкая сатурация

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("decay")) decay = p["decay"];
    if (p.count("metal")) metal = p["metal"];
    if (p.count("tone"))  tone  = p["tone"];
    if (p.count("drive")) drive = p["drive"];

    int num_samples = static_cast<int>(samplerate * decay);
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float> (1, num_samples);
    float* data = buffer->getWritePointer (0);

    double p1 = 0.0, p2 = 0.0, p3 = 0.0, p4 = 0.0, p5 = 0.0, p6 = 0.0;
    uint32_t noise_state = 0x98765432u;

    float dt = 1.0f / static_cast<float>(samplerate);

    float bpf_s1 = 0.0f, bpf_s2 = 0.0f;
    float w0 = 2.0f * M_PI * tone * dt;
    if (w0 > 0.8f) w0 = 0.8f;
    float q = 2.0f;
    float alpha = w0 / q;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Металлический кластер
        p1 += 2.0 * M_PI * (metal * 1.000f) * dt;
        p2 += 2.0 * M_PI * (metal * 1.473f) * dt;
        p3 += 2.0 * M_PI * (metal * 2.154f) * dt;
        p4 += 2.0 * M_PI * (metal * 3.112f) * dt;
        p5 += 2.0 * M_PI * (metal * 4.391f) * dt;
        p6 += 2.0 * M_PI * (metal * 5.817f) * dt;

        float osc1 = (std::sin(p1) > 0.0f ? 1.0f : -1.0f);
        float osc2 = (std::sin(p2) > 0.0f ? 1.0f : -1.0f);
        float osc3 = std::sin(p3);
        float osc4 = std::sin(p4);
        float osc5 = std::sin(p5);
        float osc6 = std::sin(p6);

        float metallic_ring = (osc1 * osc3 + osc2 * osc4 + osc5 * osc6) * 0.33f;

        // 2. Белый шум (уменьшили пропорцию до 0.35f, чтобы не шипело как радио)
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float combined = (raw_noise * 0.35f) + (metallic_ring * 0.65f);

        // 3. Фильтрация
        bpf_s1 = bpf_s1 + w0 * (combined - bpf_s1 - alpha * bpf_s2);
        bpf_s2 = bpf_s2 + w0 * bpf_s1;
        float filtered = bpf_s1;

        // 4. Более крутая экспоненциальная огибающая (ускорен спад хвоста)
        float body_env = std::exp(-t * (4.5f / decay));
        float noise_env = std::exp(-t * (6.0f / decay)); // Шум гаснет быстрее металла

        // Щелчок атаки
        float attack = 0.0f;
        if (t < 0.002f) {
            attack = (1.0f - t / 0.002f) * 1.5f;
        }

        float sample = (filtered * noise_env + metallic_ring * body_env * 0.5f + attack) * body_env;

        // 5. Плавное затухание в самом конце буфера (естественный release)
        float release_fade = 1.0f;
        float tail_time = decay * 0.2f; // Последние 20% времени
        if (t > (decay - tail_time)) {
            release_fade = (decay - t) / tail_time;
            if (release_fade < 0.0f) release_fade = 0.0f;
        }

        sample *= release_fade;

        if (drive > 1.0f) {
            sample = std::tanh(sample * drive);
        }

        data[i] = sample;
    }

    // Вызов apply_buffer_fadeout больше не нужен, так как сглаживание уже сделано в release_fade
    return buffer;
}


juce::AudioBuffer<float>* render_crash (const std::string &formula_str, int samplerate)
{
    float size = 18.0f;     // Размер тарелки в дюймах по умолчанию (рок-стандарт)
    float decay = 0.0f;     // Если 0, рассчитывается автоматически от размера
    float tone = 5500.0f;   // Базовая частота среза HPF
    float noise_mix = 0.6f; // Доля шума для воздушного шлейфа
    float metal_mix = 0.4f; // Доля металлических гармоник
    float pitch = 1.0f;
    float detune = 0.35f;   // Плотный металлический разброс
    float drive = 1.2f;     // Сатурация для агрессивности

    auto params = parse_synth_params (formula_str);
    if (params.count("size"))   size = params["size"];
    if (params.count("decay"))  decay = params["decay"];
    if (params.count("tone"))   tone = params["tone"];
    if (params.count("noise"))  noise_mix = params["noise"];
    if (params.count("metal"))  metal_mix = params["metal"];
    if (params.count("pitch"))  pitch = params["pitch"];
    if (params.count("detune")) detune = params["detune"];
    if (params.count("drive"))  drive = params["drive"];

    // Защита диапазона размера (от 14" сплеш/фаст до 22" тяжелый райд-крэш)
    if (size < 12.0f) size = 12.0f;
    if (size > 24.0f) size = 24.0f;

    // Физическое масштабирование от размера:
    // 1. Крупнее размер -> ниже базовый питч (коэффициент масштабирования)
    float size_scale = 16.0f / size; // 16 дюймов берем за единицу
    pitch *= size_scale;

    // 2. Крупнее размер -> темнее тон (ниже срез фильтра)
    tone *= (1.0f - (size - 14.0f) * 0.03f);

    // 3. Автоматический расчет длительности хвоста от размера, если он не задан
    if (decay <= 0.0f) {
        decay = 1.2f + (size - 14.0f) * 0.18f; // 14" = ~1.2с, 18" = ~1.9с, 22" = ~2.6с
    }

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 0xABCD1234u;

    // Базовые частоты металлических пластин для крэша (шире и глубже, чем у хайхета)
    float base_metal_freqs[6] = { 280.5f, 410.2f, 595.8f, 780.4f, 1050.0f, 1380.0f };
    float metal_phases[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    float hpf_prev_input = 0.0f;
    float hpf_prev_output = 0.0f;

    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (2.0f * M_PI * tone);
    float alpha = rc / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // Металлический блок гармоник с детюном
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

        // Детерминированный шум для шипящего шлейфа
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float noise_sample = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        // Микшируем металл и шум
        float raw_mix = (metal_sample * metal_mix) + (noise_sample * noise_mix);

        // Фильтр высоких частот (HPF) для чистоты бронзового спектра
        float hpf_output = alpha * (hpf_prev_output + raw_mix - hpf_prev_input);
        hpf_prev_input = raw_mix;
        hpf_prev_output = hpf_output;

        // Огибающая: хлесткий удар на атаке (первые 3 мс) + долгое экспоненциальное затухание хвоста
        float env = std::exp(-2.5f * (t / decay));
        if (t < 0.003f) {
            env *= (t / 0.003f) * 2.5f; // Резкий цок удара палочки
        }

        float sample = hpf_output * env;

        // Сатурация для плотности и «хруста» тарелки
        if (drive > 1.0f) {
            sample = std::tanh(sample * drive);
        }

        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.015f);
    return buffer;
}

juce::AudioBuffer<float>* render_crash_old2 (const std::string &formula_str, int samplerate)
{
    float size   = 18.0f;   // Размер тарелки в дюймах (14.0 - 22.0)
    float decay  = 0.0f;    // Автоматический расчет от размера
    float tone   = 6000.0f; // Частота яркости
    float drive  = 1.1f;    // Сатурация

    auto params = parse_synth_params (formula_str);
    if (params.count("size"))  size  = params["size"];
    if (params.count("decay")) decay = params["decay"];
    if (params.count("tone"))  tone  = params["tone"];
    if (params.count("drive")) drive = params["drive"];

    if (size < 12.0f) size = 12.0f;
    if (size > 24.0f) size = 24.0f;

    if (decay <= 0.0f) {
        decay = 1.5f + (size - 14.0f) * 0.18f;
    }

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 0x87654321u;
    float dt = 1.0f / static_cast<float>(samplerate);

    float size_scale = 16.0f / size;
    float current_tone = tone * size_scale;

    // Металлические гармоники для послезвучия в хвосте
    float m_freq1 = 450.0f * size_scale;
    float m_freq2 = 780.0f * size_scale;
    float m_freq3 = 1250.0f * size_scale;
    float phase1 = 0.0f;
    float phase2 = 0.0f;
    float phase3 = 0.0f;

    // Фильтр высоких частот (HPF) — срез низов до ~500 Гц
    float hpf_fc = 500.0f;
    float hpf_rc = 1.0f / (2.0f * M_PI * hpf_fc);
    float hpf_alpha = hpf_rc / (hpf_rc + dt);
    float hpf_prev_in = 0.0f, hpf_prev_out = 0.0f;

    // Двухполюсный фильтр низких частот для мягкого шлейфа
    float lpf_state1 = 0.0f;
    float lpf_state2 = 0.0f;
    float rc_lpf = 1.0f / (2.0f * M_PI * current_tone);
    float alpha_lpf = dt / (rc_lpf + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Белый шум
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        // 2. Металлический отзвук в хвосте
        phase1 += m_freq1 * dt; if (phase1 >= 1.0f) phase1 -= 1.0f;
        phase2 += m_freq2 * dt; if (phase2 >= 1.0f) phase2 -= 1.0f;
        phase3 += m_freq3 * dt; if (phase3 >= 1.0f) phase3 -= 1.0f;

        float metal_ring = (std::sin(phase1 * 2.0f * M_PI) * 0.4f +
        std::sin(phase2 * 2.0f * M_PI) * 0.35f +
        std::sin(phase3 * 2.0f * M_PI) * 0.25f);

        // 3. Фильтрация шума
        lpf_state1 += alpha_lpf * (raw_noise - lpf_state1);
        lpf_state2 += alpha_lpf * (lpf_state1 - lpf_state2);
        float soft_shimmer = lpf_state2;

        float raw_mix = (soft_shimmer * 0.75f) + (metal_ring * 0.25f);

        // 4. Очень мощный, но короткий удар/транзиент в самом начале (первые 4 мс)
        float stick_attack = 0.0f;
        if (t < 0.004f) {
            float attack_env = (1.0f - (t / 0.004f));
            stick_attack = raw_noise * attack_env * 3.5f;
        }

        // 5. ДВУХЭТАПНАЯ ОГИБАЮЩАЯ (формирует тот самый «провал» амплитуды после удара как на картинке)
        // Сначала резкий спад в первые 0.1 секунды до уровня хвоста, затем долгое затухание
        float body_env;
        if (t < 0.08f) {
            // Плавный переход от пика удара к тимому шлейфу
            float p = t / 0.08f;
            body_env = 1.0f - p * 0.65f; // Падает с 1.0 до 0.35
        } else {
            // Основное тихое послезвучие (хвост)
            float tail_t = t - 0.08f;
            float tail_decay = decay - 0.08f;
            if (tail_decay < 0.1f) tail_decay = 0.1f;
            body_env = 0.35f * std::exp(-tail_t * (3.0f / tail_decay));
        }

        float sample = (raw_mix * body_env) + stick_attack;

        // 6. Срез низов (HPF по графику ReaEQ)[cite: 3]
        float hpf_out = hpf_alpha * (hpf_prev_out + sample - hpf_prev_in);
        hpf_prev_in = sample;
        hpf_prev_out = hpf_out;
        sample = hpf_out * 1.2f;

        // 7. Мягкое затухание конца буфера
        float release_fade = 1.0f;
        float tail_time = decay * 0.1f;
        if (t > (decay - tail_time)) {
            release_fade = (decay - t) / tail_time;
            if (release_fade < 0.0f) release_fade = 0.0f;
        }
        sample *= release_fade;

        if (drive > 1.0f) {
            sample = std::tanh(sample * drive);
        }

        data[i] = sample;
    }

    return buffer;
}



juce::AudioBuffer<float>* render_crash_03 (const std::string &formula_str, int samplerate)
{
    float size   = 16.0f;   // Размер тарелки
    float decay  = 0.0f;
    float tone   = 12000.0f;
    float drive  = 1.1f;

    auto params = parse_synth_params (formula_str);
    if (params.count("size"))  size  = params["size"];
    if (params.count("decay")) decay = params["decay"];
    if (params.count("tone"))  tone  = params["tone"];
    if (params.count("drive")) drive = params["drive"];

    if (size < 10.0f) size = 10.0f;
    if (size > 22.0f) size = 22.0f;

    if (decay <= 0.0f) {
        decay = 1.2f + (size - 12.0f) * 0.15f;
    }

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 0x87654321u;
    float dt = 1.0f / static_cast<float>(samplerate);

    float size_scale = 15.0f / size;
    float current_tone = tone * size_scale;

    // Фоновые металлические гармоники
    float m_freq1 = 1250.0f * size_scale;
    float m_freq2 = 1950.0f * size_scale;
    float m_freq3 = 2900.0f * size_scale;
    float phase1 = 0.0f;
    float phase2 = 0.0f;
    float phase3 = 0.0f;

    // Высокий срез низов (HPF)
    float hpf_fc = 900.0f;
    float hpf_rc = 1.0f / (2.0f * M_PI * hpf_fc);
    float hpf_alpha = hpf_rc / (hpf_rc + dt);
    float hpf_prev_in = 0.0f, hpf_prev_out = 0.0f;

    // Фильтр для чистого верха
    float lpf_state1 = 0.0f;
    float lpf_state2 = 0.0f;
    float rc_lpf = 1.0f / (2.0f * M_PI * current_tone);
    float alpha_lpf = dt / (rc_lpf + dt);

    float prev_raw_noise = 0.0f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Генерация дымчатого высокочастотного шума
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float high_noise = raw_noise - prev_raw_noise;
        prev_raw_noise = raw_noise;

        // 2. Тихий металлический отзвук
        phase1 += m_freq1 * dt; if (phase1 >= 1.0f) phase1 -= 1.0f;
        phase2 += m_freq2 * dt; if (phase2 >= 1.0f) phase2 -= 1.0f;
        phase3 += m_freq3 * dt; if (phase3 >= 1.0f) phase3 -= 1.0f;

        float metal_ring = (std::sin(phase1 * 2.0f * M_PI) * 0.25f +
        std::sin(phase2 * 2.0f * M_PI) * 0.2f +
        std::sin(phase3 * 2.0f * M_PI) * 0.15f);

        // 3. Фильтрация шлейфа
        lpf_state1 += alpha_lpf * (high_noise - lpf_state1);
        lpf_state2 += alpha_lpf * (lpf_state1 - lpf_state2);
        float smoke_shimmer = lpf_state2 * 1.5f;

        float raw_mix = (smoke_shimmer * 0.8f) + (metal_ring * 0.2f);

        // 4. УСИЛЕННАЯ АТАКА В СТИЛЕ СНЭРА (длительность ~4 мс, увеличенная громкость и плотность)
        float stick_attack = 0.0f;
        if (t < 0.004f) {
            float p = t / 0.004f;
            float snare_style_env = 0.5f * (1.0f + std::cos(p * M_PI));
            // Подняли коэффициент громкости до 5.5 для мощного, пробивного удара
            stick_attack = high_noise * snare_style_env * 5.5f;
        }

        // 5. Двухэтапная огибающая (мощный старт -> провал -> тихий дымчатый хвост)
        float body_env;
        if (t < 0.045f) {
            float p = t / 0.045f;
            body_env = 1.0f - p * 0.8f;
        } else {
            float tail_t = t - 0.045f;
            float tail_decay = decay - 0.045f;
            if (tail_decay < 0.1f) tail_decay = 0.1f;
            body_env = 0.2f * std::exp(-tail_t * (3.5f / tail_decay));
        }

        float sample = (raw_mix * body_env) + stick_attack;

        // 6. Срез низов на 900 Гц
        float hpf_out = hpf_alpha * (hpf_prev_out + sample - hpf_prev_in);
        hpf_prev_in = sample;
        hpf_prev_out = hpf_out;
        sample = hpf_out * 1.2f;

        // 7. Сглаживание конца буфера
        float release_fade = 1.0f;
        float tail_time = decay * 0.1f;
        if (t > (decay - tail_time)) {
            release_fade = (decay - t) / tail_time;
            if (release_fade < 0.0f) release_fade = 0.0f;
        }
        sample *= release_fade;

        if (drive > 1.0f) {
            sample = std::tanh(sample * drive);
        }

        data[i] = sample;
    }

    return buffer;
}


// Realistic crash cymbal synthesis (modal + filtered noise)
// Usage: similar to your original render_crash signature
/*#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <random>
#include <string>
#include <map>

// Assumes parse_synth_params exists and returns std::map<std::string, float>
std::map<std::string, float> parse_synth_params(const std::string& s);
*/
struct Mode {
    float coef1 = 0.0f, coef2 = 0.0f; // recurrence coefs
    float y1 = 0.0f, y2 = 0.0f;       // state
    float amp = 0.0f;                // amplitude
};



juce::AudioBuffer<float>* render_crash_hybrid (const std::string &formula_str, int samplerate)
{
    if (samplerate <= 0) samplerate = 44100;

    float decay     = 1.5f;
    float tone      = 5500.0f;
    float pitch     = 1.0f;
    float drive     = 1.2f;
    float noise_mix = 0.5f;

    auto params = parse_synth_params(formula_str);
    if (params.count("decay"))     decay     = params["decay"];
    if (params.count("tone"))      tone      = params["tone"];
    if (params.count("pitch"))     pitch     = params["pitch"];
    if (params.count("drive"))     drive     = params["drive"];
    if (params.count("noise"))     noise_mix = params["noise"];

    if (decay < 0.1f) decay = 0.1f;
    if (decay > 6.0f) decay = 6.0f;
    if (pitch < 0.3f) pitch = 0.3f;
    if (pitch > 3.0f) pitch = 3.0f;
    if (noise_mix < 0.0f) noise_mix = 0.0f;
    if (noise_mix > 1.0f) noise_mix = 1.0f;

    float cutoff = tone;
    if (cutoff <= 1.0f) {
        cutoff = 2000.0f + cutoff * 8000.0f;
    }
    if (cutoff < 500.0f) cutoff = 500.0f;
    if (cutoff > samplerate * 0.45f) cutoff = samplerate * 0.45f;

    int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
    if (num_samples < 512) num_samples = 512;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    if (!buffer) return nullptr;
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 0xABCD1234u;

    float base_freqs[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
    float phases[6]     = { 0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f };

    float dt = 1.0f / static_cast<float>(samplerate);

    float hpf_prev_input = 0.0f;
    float hpf_prev_output = 0.0f;
    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float alpha = rc / (rc + dt);

    // Сглаживание шума с приподнятым срезом (~2.5 кГц) для большей яркости и искристости
    float noise_lpf_state = 0.0f;
    float noise_alpha = dt / (0.00006f + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Металлический блок осцилляторов
        float metal_sample = 0.0f;
        for (int m = 0; m < 6; ++m)
        {
            phases[m] += (base_freqs[m] * pitch) * dt;
            if (phases[m] >= 1.0f) phases[m] -= 1.0f;
            metal_sample += (phases[m] < 0.5f) ? 1.0f : -1.0f;
        }
        metal_sample /= 6.0f;

        // 2. Яркий, шелковистый шумовой хвост
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        noise_lpf_state += noise_alpha * (raw_noise - noise_lpf_state);
        float noise_diff = raw_noise - noise_lpf_state;

        // Подчеркиваем верха для прозрачности в миксе
        float noise_sample = (noise_lpf_state * 0.7f + noise_diff * 0.8f);

        // 3. Гибридный микс квадратов и яркого шума
        float raw_mix = (metal_sample * (1.0f - noise_mix)) + (noise_sample * noise_mix);

        // 4. Основной фильтр высоких частот (HPF)
        float hpf_output = alpha * (hpf_prev_output + raw_mix - hpf_prev_input);
        hpf_prev_input = raw_mix;
        hpf_prev_output = hpf_output;

        // 5. Огибающая с хлесткой атакой и мягким затуханием
        float env = std::exp(-2.5f * (t / decay));
        if (t < 0.003f) {
            env *= (t / 0.003f) * 2.5f;
        } else {
            float tail_progress = t / decay;
            env *= (1.0f - tail_progress * 0.15f);
        }

        float sample = hpf_output * env;

        if (drive > 1.0f) {
            sample = std::tanh(sample * drive);
        }

        if (!std::isfinite(sample)) sample = 0.0f;
        data[i] = sample;
    }

    apply_buffer_fadeout(buffer, samplerate, 0.02f);
    return buffer;
}

juce::AudioBuffer<float>* render_ride (const std::string &formula_str, int samplerate)
{
    float decay  = 1.8f;
    float pitch  = 1.1f;
    float tone   = 0.6f;
    float drive  = 1.1f;
    float noise  = 0.25f;
    float ping   = 0.6f;

    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("decay")) decay = p["decay"];
    if (p.count("pitch")) pitch = p["pitch"];
    if (p.count("tone"))  tone  = p["tone"];
    if (p.count("drive")) drive = p["drive"];
    if (p.count("noise")) noise = p["noise"];
    if (p.count("ping"))  ping  = p["ping"];

    if (decay < 0.1f) decay = 0.1f;
    if (decay > 8.0f) decay = 8.0f;

    // Расширяем диапазон верхов для максимальной воздушности (до 14 кГц)
    float filter_freq = tone;
    if (filter_freq <= 1.0f) {
        filter_freq = 4000.0f + filter_freq * 10000.0f;
    }
    if (filter_freq < 1000.0f) filter_freq = 1000.0f;
    if (filter_freq > samplerate * 0.45f) filter_freq = samplerate * 0.45f;

    int num_samples = static_cast<int>(samplerate * decay);
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float> (1, num_samples);
    float* data = buffer->getWritePointer (0);

    double p1 = 0.0, p2 = 0.0, p3 = 0.0, p4 = 0.0, p5 = 0.0, p6 = 0.0;
    uint32_t noise_state = 0x87654321u;

    float dt = 1.0f / static_cast<float>(samplerate);

    // Сглаживающий фильтр для воздуха
    float bpf_s1 = 0.0f, bpf_s2 = 0.0f;
    float w0 = 2.0f * M_PI * filter_freq * dt;
    if (w0 > 0.92f) w0 = 0.92f;
    float q = 1.4f; // Мягкая добротность без резонансного гула
    float alpha = w0 / q;

    float base_metal = 1250.0f * pitch;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Тонкий металлический кластер
        p1 += 2.0 * M_PI * (base_metal * 1.000f) * dt;
        p2 += 2.0 * M_PI * (base_metal * 1.618f) * dt;
        p3 += 2.0 * M_PI * (base_metal * 2.542f) * dt;
        p4 += 2.0 * M_PI * (base_metal * 3.833f) * dt;
        p5 += 2.0 * M_PI * (base_metal * 5.211f) * dt;
        p6 += 2.0 * M_PI * (base_metal * 7.145f) * dt;

        float osc1 = std::sin(p1);
        float osc2 = std::sin(p2);
        float osc3 = std::sin(p3);
        float osc4 = std::sin(p4);
        float osc5 = std::sin(p5);
        float osc6 = std::sin(p6);

        float metallic_ring = (osc1 * osc3 + osc2 * osc4 + osc5 * osc6) * 0.22f;

        // 2. Деликатный воздух (высокочастотный шум)
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        float combined = (raw_noise * noise) + (metallic_ring * (1.0f - noise));

        // 3. Фильтрация
        bpf_s1 = bpf_s1 + w0 * (combined - bpf_s1 - alpha * bpf_s2);
        bpf_s2 = bpf_s2 + w0 * bpf_s1;
        float filtered = bpf_s1;

        // 4. Элегантная огибающая хвоста
        float body_env = std::exp(-t * (4.5f / decay));

        // Сверхкороткий, стеклянный «цок» палочки (Ping) — длительность всего 0.8 мс
        float attack = 0.0f;
        if (t < 0.0008f) {
            float attack_progress = t / 0.0008f;
            // Добавляем высокочастотный игольчатый призвук в сам удар
            float stick_sparkle = std::sin(t * 18000.0f * M_PI) * 0.5f;
            attack = (1.0f - attack_progress) * (4.0f * ping) + stick_sparkle * ping;
        }

        float sample = (filtered * body_env * 0.45f + metallic_ring * body_env * 0.25f + attack);

        // 5. Гладкий релиз
        float release_fade = 1.0f;
        float tail_time = decay * 0.2f;
        if (t > (decay - tail_time)) {
            release_fade = (decay - t) / tail_time;
            if (release_fade < 0.0f) release_fade = 0.0f;
        }

        sample *= release_fade;

        if (drive > 1.0f) {
            sample = std::tanh(sample * drive);
        }

        data[i] = sample;
    }

    return buffer;
}



 juce::AudioBuffer<float>* render_glass (const std::string &formula_str, int samplerate)
 {
     float decay  = 1.8f;
     float pitch  = 1.1f;
     float tone   = 0.6f;
     float drive  = 1.1f;
     float noise  = 0.25f;
     float ping   = 0.6f;

     std::map<std::string, float> p = parse_synth_params (formula_str);
     if (p.count("decay")) decay = p["decay"];
     if (p.count("pitch")) pitch = p["pitch"];
     if (p.count("tone"))  tone  = p["tone"];
     if (p.count("drive")) drive = p["drive"];
     if (p.count("noise")) noise = p["noise"];
     if (p.count("ping"))  ping  = p["ping"];

     if (decay < 0.1f) decay = 0.1f;
     if (decay > 8.0f) decay = 8.0f;

     float filter_freq = tone;
     if (filter_freq <= 1.0f) {
         filter_freq = 3000.0f + filter_freq * 9000.0f;
     }
     if (filter_freq < 500.0f) filter_freq = 500.0f;
     if (filter_freq > samplerate * 0.45f) filter_freq = samplerate * 0.45f;

     int num_samples = static_cast<int>(samplerate * decay);
     if (num_samples < 100) num_samples = 100;

     auto* buffer = new juce::AudioBuffer<float> (1, num_samples);
     float* data = buffer->getWritePointer (0);

     double p1 = 0.0, p2 = 0.0, p3 = 0.0, p4 = 0.0, p5 = 0.0, p6 = 0.0;
     uint32_t noise_state = 0x87654321u;

     float dt = 1.0f / static_cast<float>(samplerate);

     float bpf_s1 = 0.0f, bpf_s2 = 0.0f;
     float w0 = 2.0f * M_PI * filter_freq * dt;
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
         if (t < 0.0012f) {
             attack = (1.0f - t / 0.0012f) * 3.5f * ping;
         }

         float sample = (filtered * body_env * 0.5f + glass_ring * body_env * 0.3f + attack);

         float release_fade = 1.0f;
         float tail_time = decay * 0.25f;
         if (t > (decay - tail_time)) {
             release_fade = (decay - t) / tail_time;
             if (release_fade < 0.0f) release_fade = 0.0f;
         }

         sample *= release_fade;

         if (drive > 1.0f) {
             sample = std::tanh(sample * drive);
         }

         data[i] = sample;
     }

     return buffer;
 }

juce::AudioBuffer<float>* render_synth_sample (const std::string &filename, int session_samplerate, bool is_filename)
{
    std::cout << "juce::AudioBuffer<float>* render_synth_sample: " << filename << "\n";


    std::string formula_str;

    if (is_filename)
       formula_str = string_file_load (filename);
    else
        formula_str = filename;



    if (formula_str.rfind ("kick(", 0) == 0)
    {
        return render_kick (formula_str, session_samplerate);

    }



    if (formula_str.rfind ("snare(", 0) == 0)
    {
        return render_snare (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("hihat_close(", 0) == 0 || formula_str.rfind ("hihat_open(", 0) == 0)
    {
        return render_hihat (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("clap(", 0) == 0)
    {
        return render_clap (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("psytrance_kick(", 0) == 0)
    {
        return render_psytrance_kick (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("coldwave_snare(", 0) == 0)
    {
        return render_coldwave_snare (formula_str, session_samplerate);
    }

    if (formula_str.rfind ("computer_tom(", 0) == 0)
    {
        return render_computer_tom (formula_str, session_samplerate);
    }

    if (formula_str.rfind ("computer_snare(", 0) == 0)
    {
        return render_computer_snare (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("gabber_kick(", 0) == 0)
    {
        return render_gabber_kick (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("boom_sound(", 0) == 0)
    {
        return render_boom_sound (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("box(", 0) == 0)
    {
        return render_box_sound (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("cowbell(", 0) == 0)
    {
        return render_cowbell_sound (formula_str, session_samplerate);
    }

    if (formula_str.rfind ("rimshot(", 0) == 0)
    {
        return render_rimshot (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("dnb_kick(", 0) == 0)
    {
        return render_dnb_kick (formula_str, session_samplerate);
    }

    if (formula_str.rfind ("snare_fluffy(", 0) == 0)
    {
        return render_snare_fluffy(formula_str, session_samplerate);
    }


    if (formula_str.rfind ("ride(", 0) == 0)
    {
        return render_ride (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("metal(", 0) == 0)
    {
        return render_crash (formula_str, session_samplerate);
    }

    if (formula_str.rfind ("glass(", 0) == 0)
    {
        return render_glass  (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("crash_hybrid(", 0) == 0)
    {
        return render_crash_hybrid  (formula_str, session_samplerate);
    }

    if (formula_str.rfind ("ride(", 0) == 0)
    {
        return render_ride (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("fart(", 0) == 0)
    {
        return render_fart (formula_str, session_samplerate);
    }


    return nullptr;
}

