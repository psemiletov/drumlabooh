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
constexpr double TWO_PI = 2.0 * M_PI;


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

    const float dt = 1.0f / static_cast<float>(samplerate);
    //const float TWO_PI = 2.0f * static_cast<float>(M_PI);

    float rc_lpf = 1.0f / (TWO_PI * lpf_cutoff);
    float alpha_lpf = dt / (rc_lpf + dt);

    float rc_hpf = 1.0f / (TWO_PI * hpf_cutoff);
    float alpha_hpf = rc_hpf / (rc_hpf + dt);

    float micro_burst_env = 0.0f;
    float micro_burst_phase = 0.0f;
    int   micro_burst_timer = 0;
    int   next_burst_samples = static_cast<int>(samplerate * 0.02f);

    float body_env_factor = std::exp(-dt / release);
    float noise_env_factor = std::exp(-dt / (release * 0.5f));
    float body_env = 1.0f;
    float noise_env = 1.0f;

    // Предрасчет постоянных инкрементов для wobble и bubble, чтобы не умножать каждый раз
    const float wobble_phase_inc = TWO_PI * wobble_rate * dt;
    const float bubble_phase_inc = TWO_PI * 14.0f * dt;
    const float micro_burst_freq_inc = TWO_PI * (base_freq * 1.6f) * dt;

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
        wobblePhase += wobble_phase_inc;
        freq1 *= (1.0f + wobble);

        phase1 += static_cast<double>(TWO_PI) * freq1 * dt;
        phase2 += static_cast<double>(TWO_PI) * freq2 * dt;

        // Быстрое приведение фаз в диапазон [0, 2*PI] для предотвращения потери точности double при больших значениях
        if (phase1 > TWO_PI) phase1 -= TWO_PI;
        if (phase2 > TWO_PI) phase2 -= TWO_PI;

        float tone_sig = std::sin (static_cast<float>(phase1)) * 0.75f + (std::sin (static_cast<float>(phase2 * 2.0f)) * 0.25f);

        float bubble = 1.0f + bubble_amt * std::sin (bubblePhase) * std::cos (bubblePhase * 0.4f);
        bubblePhase += bubble_phase_inc;
        if (bubblePhase > TWO_PI) bubblePhase -= TWO_PI;

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
                micro_burst_phase += micro_burst_freq_inc;
                if (micro_burst_phase > TWO_PI) micro_burst_phase -= TWO_PI;
                granular_sig = std::sin (micro_burst_phase) * micro_burst_env;
                micro_burst_env *= 0.993f; // Точный аналог std::exp(-dt * 70.0f) при dt = 1/44100 (~0.998), но быстрее
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


juce::AudioBuffer<float>* render_boom_sound2 (const std::string &formula_str, int samplerate)
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
    float tone = 7000.0f;      // Срез верхних частот (LPF)
    float hpf_freq = 4500.0f; // Срез нижних частот (HPF)
    float q = 0.707f;          // Добротность / Резонанс
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
    if (params.count("hpf"))     hpf_freq = params["hpf"];
    if (params.count("q"))       q = params["q"];
    if (params.count("res"))     q = params["res"]; // Поддержка синонима res
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

    // Предрассчитываем фазовые инкременты для осцилляторов один раз (избавились от лишней математики в цикле)
    float phase_incs[6];
    for (int m = 0; m < 6; ++m)
    {
        float freq_mod = base_metal_freqs[m] * (1.0f + static_cast<float>(m) * detune * 0.15f);
        phase_incs[m] = (freq_mod * pitch) * (1.0f / static_cast<float>(samplerate));
    }

    float dt = 1.0f / static_cast<float>(samplerate);

    // 1. Ограничение и коэффициенты для резонансного HPF (SVF)
    if (hpf_freq < 100.0f) hpf_freq = 100.0f;
    if (hpf_freq > samplerate * 0.45f) hpf_freq = samplerate * 0.45f;
    if (q < 0.5f) q = 0.5f;
    if (q > 10.0f) q = 10.0f;

    // SVF переменные состояния
    float f = 2.0f * std::sin(static_cast<float>(M_PI) * hpf_freq * dt); // Коэффициент частоты
    float damp = 1.0f / q;                               // Затухание (обратно q)
    float hp_s1 = 0.0f;
    float hp_s2 = 0.0f;

    // 2. Коэффициент LPF (Tone)
    float rc_lpf = 1.0f / (2.0f * static_cast<float>(M_PI) * tone);
    float alpha_lpf = rc_lpf / (rc_lpf + dt);
    float lpf_prev = 0.0f;

    float env_factor = std::exp(-dt * (3.0f / decay));
    float env = 1.0f;

    const float inv_attack_time = 1.0f / 0.002f; // Замена деления на умножение
    const bool apply_drive = (drive > 1.0f);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // Генерация металлического ядра (быстрый цикл без умножений внутри)
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

        // Применяем 2-полюсный резонансный High-Pass Filter (Chamberlin SVF)
        float hp_out = raw_mix - hp_s2 - damp * hp_s1;
        float bp_out = f * hp_out + hp_s1;
        float lp_out = f * bp_out + hp_s2;

        hp_s1 = bp_out;
        hp_s2 = lp_out;

        // Защита от потенциальной нестабильности при крайних значениях f
        if (std::isnan(hp_out) || std::isinf(hp_out)) hp_out = 0.0f;

        // Применяем LPF (Tone) для сглаживания верхов
        lpf_prev = alpha_lpf * lpf_prev + (1.0f - alpha_lpf) * hp_out;

        env *= env_factor;
        float current_env = env;
        if (t < 0.002f) {
            current_env *= (t * inv_attack_time); // Умножение вместо деления
        }

        float sample = lpf_prev * current_env;

        if (apply_drive) {
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
    float tone = 1400.0f;
    float spread = 0.011f;
    int burst_count = 4;
    float drive = 1.5f;

    auto params = parse_synth_params (formula_str);
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

    // Двухполюсный фильтр
    float lpf_state1 = 0.0f;
    float lpf_state2 = 0.0f;

    const float rc = 1.0f / (2.0f * 3.14159265f * tone);
    const float alpha = dt / (rc + dt);

    // Константы затухания для инкрементного умножения
    const float tail_env_factor = std::exp(-dt * (3.0f / decay));
    float tail_env = 1.0f;

    const float burst_decay_factor = std::exp(-dt * 450.0f);

    // Предрассчитываем точное время старта каждого хлопка
    float burst_times[12];
    float burst_accents[12];
    float b_env[12] = {0.0f};
    bool b_active[12] = {false};

    for (int b = 0; b < burst_count; ++b)
    {
        burst_times[b] = static_cast<float>(b) * spread;
        burst_accents[b] = (b == 0) ? 1.6f : (1.0f - 0.05f * static_cast<float>(b));
    }

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // Генерация белого шума
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        float noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        // Пачка хлопков с точным суб-семпловым временем без std::exp внутри цикла
        float burst_env = 0.0f;
        for (int b = 0; b < burst_count; ++b)
        {
            if (!b_active[b] && t >= burst_times[b])
            {
                b_active[b] = true;
                // Точный старт с учетом под-семплового остатка времени
                float dt_burst = t - burst_times[b];
                b_env[b] = std::exp(-dt_burst * 450.0f) * burst_accents[b];
            }
            else if (b_active[b])
            {
                b_env[b] *= burst_decay_factor;
            }
            burst_env += b_env[b];
        }

        // Затухающий хвост через инкрементный множитель
        float current_tail_env = tail_env;
        tail_env *= tail_env_factor;

        float total_env = (burst_env * 0.85f) + (current_tail_env * 0.15f);

        // Двухкаскадный фильтр
        lpf_state1 += alpha * (noise - lpf_state1);
        lpf_state2 += alpha * (lpf_state1 - lpf_state2);

        float clap_body = (lpf_state1 * 0.5f + lpf_state2 * 1.5f);
        float sample = clap_body * total_env;

        sample = std::tanh(sample * drive);

        if (!std::isfinite(sample)) sample = 0.0f;
        data[i] = sample;
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
    uint32_t noise_state = 0x13579BDFu; // Детерминированный генератор вместо rand()

    const float dt = 1.0f / static_cast<float>(samplerate);
    const double TWO_PI = 2.0 * M_PI;
    const double TWO_PI_DT = TWO_PI * dt;
    const float env_decay_coeff = 3.0f / decay;
    const float inv_click_duration = 1.0f / 0.005f;

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq = f1 + (f0 - f1) * std::exp (-t * 20.0f);
        phase += TWO_PI_DT * freq;
        if (phase > TWO_PI) phase -= TWO_PI;

        float env = std::exp (-t * env_decay_coeff);
        float sample = std::sin (phase) * env;

        if (t < 0.005f) {
            noise_state ^= noise_state << 13;
            noise_state ^= noise_state >> 17;
            noise_state ^= noise_state << 5;
            float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;
            float click = raw_noise * (1.0f - t * inv_click_duration);
            sample += click * click_level;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f); // 3 мс на сглаживание хвоста
    return buffer;
}


juce::AudioBuffer<float>* render_kick2 (const std::string &formula_str, int samplerate)
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


juce::AudioBuffer<float>* render_cowbell_sound2 (const std::string &formula_str, int samplerate)
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

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);

    if (!buffer)
    {
        std::cerr << "[ERROR] Failed to allocate audio buffer for cowbell!\n";
        return nullptr;
    }

    float *data = buffer->getWritePointer (0);

    double phase1 = 0.0;
    double phase2 = 0.0;

    const float dt = 1.0f / static_cast<float>(samplerate);
    //const double TWO_PI = 2.0 * M_PI;

    // Предрассчитываем фазовые инкременты (избавляемся от умножения частот в цикле)
    const double inc1 = TWO_PI * base_freq * dt;
    const double inc2 = TWO_PI * (base_freq * ratio) * dt;

    float lpf_state = 0.0f;
    float rc = 1.0f / (2.0f * static_cast<float>(M_PI) * tone);
    float alpha = dt / (rc + dt);

    // Инкрементный множитель огибающей (убираем std::exp из цикла)
    const float env_factor = std::exp(-dt * (4.0f / decay));
    float env = 1.0f;

    // Константы для клика (замена деления на умножение)
    const float inv_click_time = 1.0f / 0.0015f;
    const bool apply_drive = (drive > 1.0f);

    float t = 0.0f;

    for (int i = 0; i < num_samples; ++i, t += dt)
    {
        // 1. Фазовые инкременты с защитой от переполнения
        phase1 += inc1; if (phase1 > TWO_PI) phase1 -= TWO_PI;
        phase2 += inc2; if (phase2 > TWO_PI) phase2 -= TWO_PI;

        float sq1 = (std::sin(static_cast<float>(phase1)) >= 0.0f) ? 1.0f : -1.0f;
        float sq2 = (std::sin(static_cast<float>(phase2)) >= 0.0f) ? 1.0f : -1.0f;

        float raw_signal = (sq1 * 0.5f) + (sq2 * 0.5f);

        lpf_state += alpha * (raw_signal - lpf_state);

        // Инкрементный расчет огибающей
        float current_env = env;
        env *= env_factor;

        float click = 0.0f;
        if (t < 0.0015f) {
            click = (1.0f - t * inv_click_time) * 0.4f;
        }

        float sample = (lpf_state + click) * current_env;

        if (apply_drive) {
            sample = std::tanh (sample * drive);
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);

    return buffer;
}


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

//        const double TWO_PI = 2.0 * M_PI;


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
    float rc = 1.0f / (TWO_PI * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float freq1 = base_freq;
        float freq2 = base_freq * ratio;

        phase1 += TWO_PI * freq1 * dt;
        phase2 += TWO_PI * freq2 * dt;

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

// Assumes parse_synth_params exists and returns std::map<std::string, float>
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

    uint32_t strike_noise_state = 0x55AA3311u;



    // Базовые частоты 808 меандров (тело тарелки)

    float base_freqs[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };

    float phases[6]     = { 0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f };



    // Высокочастотные бронзовые обертоны для короткого звонкого удара

    float strike_freqs[4] = { 3800.0f, 5100.0f, 6600.0f, 8400.0f };

    float strike_phases[4] = { 0.0f, 0.0f, 0.0f, 0.0f };



    float dt = 1.0f / static_cast<float>(samplerate);



    float hpf_prev_input = 0.0f;

    float hpf_prev_output = 0.0f;

    float rc = 1.0f / (2.0f * M_PI * cutoff);

    float alpha = rc / (rc + dt);



    // Более высокий срез для шумового хвоста, чтобы он был «тонким» и воздушным, а не басовито-жирным

    float noise_lpf_state = 0.0f;

    float noise_alpha = dt / (0.00003f + dt);



    for (int i = 0; i < num_samples; ++i)

    {

        float t = static_cast<float>(i) * dt;



        // 1. Металлический блок осцилляторов (Тело тарелки)

        float metal_sample = 0.0f;

        for (int m = 0; m < 6; ++m)

        {

            phases[m] += (base_freqs[m] * pitch) * dt;

            if (phases[m] >= 1.0f) phases[m] -= 1.0f;

            metal_sample += (phases[m] < 0.5f) ? 1.0f : -1.0f;

        }

        metal_sample /= 6.0f;



        // 2. Сверхкороткий бронзовый транзиент атаки (Звяк)

        float strike_sample = 0.0f;

        float pitch_drop = 1.0f + 3.0f * std::exp(-t * 3000.0f); // Молниеносный спад частоты



        for (int s = 0; s < 4; ++s)

        {

            strike_phases[s] += (strike_freqs[s] * pitch * pitch_drop) * dt;

            if (strike_phases[s] >= 1.0f) strike_phases[s] -= 1.0f;

            strike_sample += std::sin(strike_phases[s] * 2.0f * M_PI);

        }

        strike_sample *= 0.25f;



        // Ультра-короткая огибающая звяка (~3-4 миллисекунды вместо 8)

        float strike_env = std::exp(-t * 900.0f);



        // Сухой микро-щелчок палочки (1.5 миллисекунды)

        strike_noise_state ^= strike_noise_state << 13;

        strike_noise_state ^= strike_noise_state >> 17;

        strike_noise_state ^= strike_noise_state << 5;

        float stick_click = static_cast<float>(strike_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        stick_click *= std::exp(-t * 1800.0f);



        // 3. Тонкий, прозрачный шумовой хвост

        noise_state ^= noise_state << 13;

        noise_state ^= noise_state >> 17;

        noise_state ^= noise_state << 5;

        float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;



        noise_lpf_state += noise_alpha * (raw_noise - noise_lpf_state);

        float noise_diff = raw_noise - noise_lpf_state;

        // Акцент на самую верхушку спектра для воздушности

        float noise_sample = (noise_lpf_state * 0.4f + noise_diff * 1.2f);



        // 4. Микс компонентов

        float raw_mix = (metal_sample * (1.0f - noise_mix))

        + (noise_sample * noise_mix)

        + (strike_sample * strike_env * 1.2f)

        + (stick_click * 0.6f);



        // 5. HPF фильтрация

        float hpf_output = alpha * (hpf_prev_output + raw_mix - hpf_prev_input);

        hpf_prev_input = raw_mix;

        hpf_prev_output = hpf_output;



        // 6. Быстро спадающая, прозрачная общая огибающая хвоста

        float env = std::exp(-3.5f * std::pow(t / decay, 0.85f));

        if (t < 0.0008f) {

            env *= (t / 0.0008f);

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

    uint32_t ping_noise_state = 0x12345678u;



    float dt = 1.0f / static_cast<float>(samplerate);



    // Сглаживающий фильтр для воздуха

    float bpf_s1 = 0.0f, bpf_s2 = 0.0f;

    float w0 = TWO_PI * filter_freq * dt;

    if (w0 > 0.92f) w0 = 0.92f;

    float q = 1.4f; // Мягкая добротность без резонансного гула

    float alpha = w0 / q;



    float base_metal = 1250.0f * pitch;



    for (int i = 0; i < num_samples; ++i)

    {

        float t = static_cast<float>(i) * dt;



        // 1. Тонкий металлический кластер

        p1 += TWO_PI  * (base_metal * 1.000f) * dt;

        p2 += TWO_PI  * (base_metal * 1.618f) * dt;

        p3 += TWO_PI  * (base_metal * 2.542f) * dt;

        p4 += TWO_PI  * (base_metal * 3.833f) * dt;

        p5 += TWO_PI  * (base_metal * 5.211f) * dt;

        p6 += TWO_PI  * (base_metal * 7.145f) * dt;



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



        // 3. Мягкий и чистый деревянный «пинг» (щелчок палочки) без цифрового треска

        ping_noise_state ^= ping_noise_state << 13;

        ping_noise_state ^= ping_noise_state >> 17;

        ping_noise_state ^= ping_noise_state << 5;

        float raw_ping_noise = static_cast<float>(ping_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;



        // Экспоненциальная плавная огибающая пинга (~2.5 миллисекунды)

        float stick_ping_env = std::exp(-t * 400.0f) * ping;

        float stick_ping = raw_ping_noise * stick_ping_env;



        // Микшируем тело тарелки и мягкий пинг ДО фильтрации, чтобы атака звучала слитно

        float combined = (raw_noise * noise) + (metallic_ring * (1.0f - noise)) + (stick_ping * 1.5f);



        // 4. Фильтрация (теперь пинг тоже проходит через фильтр райда, убирая резкие края)

        bpf_s1 = bpf_s1 + w0 * (combined - bpf_s1 - alpha * bpf_s2);

        bpf_s2 = bpf_s2 + w0 * bpf_s1;

        float filtered = bpf_s1;



        // 5. Элегантная огибающая хвоста

        float body_env = std::exp(-t * (4.5f / decay));



        float sample = (filtered * body_env * 0.55f + metallic_ring * body_env * 0.25f);



        // 6. Гладкий релиз

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


 juce::AudioBuffer<float>* render_psykick (const std::string &formula_str, int samplerate)
 {
     float f0 = 240.0f;
     float f1 = 38.0f;
     float decay = 0.22f;
     float click_level = 0.25f;
     float drive = 0.5f;        // Интенсивность дисторшна
     float curve = 35.0f;       // Крутизна питча
     float fm = 0.3f;           // FM-модуляция (дает "псай-лазерный" характер)
     float click_hp = 3000.0f;

     std::map<std::string, float> p = parse_synth_params (formula_str);
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

     juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
     float *data = buffer->getWritePointer (0);

     double phase = 0.0;
     uint32_t click_noise_state = 9999;

     float dt = 1.0f / static_cast<float>(samplerate);
     float env_factor = std::exp(-dt * (4.0f / decay));
     float env = 1.0f;

     float hp_alpha = 1.0f / (1.0f + TWO_PI * click_hp * dt);
     float prev_noise_in = 0.0f;
     float prev_noise_out = 0.0f;

     for (int i = 0; i < num_samples; ++i)
     {
         float t = static_cast<float>(i) * dt;

         // Базовая частота с огибающей
         float freq_env = std::exp (-t * curve);
         float freq = f1 + (f0 - f1) * freq_env;

         // FM-модуляция: модулируем фазу в начале удара (дает характерный псай-панч)
         float fm_mod = std::sin(2.0 * M_PI * freq * 2.0 * t) * fm * freq_env * 50.0f;
         phase += TWO_PI * (freq + fm_mod) * dt;

         env *= env_factor;
         float body_signal = std::sin (phase) * env;

         // Агрессивный Waveshaper / Hard/Soft Clipping для плотности и гармоник
         if (drive > 0.001f)
         {
             // Усиление сигнала перед ограничением
             float pre_gain = 1.0f + drive * 6.0f;
             float raw_driven = body_signal * pre_gain;

             // Комбинация жесткого клиппинга и tanh для максимального "кача"
             float clipped = std::max(-1.0f, std::min(1.0f, raw_driven));
             body_signal = juce::jmap(drive, 0.0f, 1.0f, body_signal, clipped * 0.9f);
         }

         float sample = body_signal;

         // Клик (шум + быстрый щелчок)
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

     apply_buffer_fadeout (buffer, samplerate, 0.003f);
     return buffer;
 }


 juce::AudioBuffer<float>* render_goakick (const std::string &formula_str, int samplerate)
 {
     float f0 = 220.0f;          // Старт питча
     float f1 = 48.0f;           // Базовая частота
     float decay = 0.28f;        // Хвост
     float punch = 0.40f;        // Уровень тонального щелчка
     float warmth = 0.35f;       // Аналоговое насыщение
     float bit_depth = 16.0f;    // Эмуляция сэмплера Akai

     std::map<std::string, float> p = parse_synth_params (formula_str);
     if (p.count("f0"))        f0 = p["f0"];
     if (p.count("f1"))        f1 = p["f1"];
     if (p.count("decay"))     decay = p["decay"];
     if (p.count("punch"))     punch = p["punch"];
     if (p.count("warmth"))    warmth = p["warmth"];
     if (p.count("bit_depth")) bit_depth = p["bit_depth"];

     int num_samples = static_cast<int>(samplerate * (decay * 1.1f));
     if (num_samples < 100) num_samples = 100;

     juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
     float *data = buffer->getWritePointer (0);

     double phase_body = 0.0;
     double phase_punch = 0.0;

     const float dt = 1.0f / static_cast<float>(samplerate);
     //const double TWO_PI = 2.0 * M_PI;

     // Предрасчет коэффициентов огибающих для избежания std::exp внутри цикла
     float env_factor = std::exp(-dt * (3.5f / decay));
     float env = 1.0f;

     // Питч-огибающая: вместо std::exp(-t * 28.0f) на каждом шаге используем инкрементный множитель
     float pitch_env_factor = std::exp(-dt * 28.0f);
     float pitch_env = 1.0f;

     // Шаг квантования ЦАП (вынесено из цикла)
     const float steps = std::pow(2.0f, bit_depth - 1.0f);
     const bool apply_bitcrush = (bit_depth < 15.9f);
     const bool apply_warmth = (warmth > 0.05f);
     const float warmth_drive = 1.0f + warmth * 1.8f;
     const float warmth_offset = 0.05f * warmth;

     for (int i = 0; i < num_samples; ++i)
     {
         float t = static_cast<float>(i) * dt;

         // 1. Двухэтапный спад питча через инкрементный множитель (без тяжелого exp)
         float freq_body = f1 + (f0 - f1) * pitch_env;
         pitch_env *= pitch_env_factor;

         phase_body += TWO_PI * freq_body * dt;
         if (phase_body > TWO_PI) phase_body -= TWO_PI;

         // 2. Генерация тела: смесь синусоиды и быстрой треугольной волны без std::asin
         env *= env_factor;
         float sin_wave = std::sin(static_cast<float>(phase_body));

         // Быстрое получение треугольной волны из фазы (вместо тяжелого (2/PI)*asin(sin(x)))
         // Нормализуем фазу phase_body в диапазон [-1, 1] для треугольника
         float phase_norm = static_cast<float>(phase_body / M_PI); // от 0 до 2
         float tri_wave = 0.0f;
         if (phase_norm < 1.0f) {
             tri_wave = -1.0f + 2.0f * phase_norm;
         } else {
             tri_wave = 3.0f - 2.0f * phase_norm;
         }

         float body_signal = juce::jmap(warmth, 0.0f, 1.0f, sin_wave, tri_wave) * env;

         // 3. Акустический щелчок колотушки (работает только первые 15 мс)
         float punch_signal = 0.0f;
         if (t < 0.015f)
         {
             float p_env = (1.0f - t / 0.015f);
             float freq_punch = 160.0f * p_env;
             phase_punch += TWO_PI * freq_punch * dt;
             if (phase_punch > TWO_PI) phase_punch -= TWO_PI;
             punch_signal = std::sin(static_cast<float>(phase_punch)) * p_env * punch;
         }

         float sample = body_signal + punch_signal;

         // 4. Аналоговая сатурация
         if (apply_warmth)
         {
             sample += warmth_offset;
             sample = std::tanh(sample * warmth_drive);
         }

         // 5. Ло-фай эмуляция ЦАП (Akai S1000)
         if (apply_bitcrush)
         {
             sample = std::round(sample * steps) / steps;
         }

         data[i] = sample;
     }

     apply_buffer_fadeout (buffer, samplerate, 0.003f);
     return buffer;
 }



 juce::AudioBuffer<float>* render_goasnare (const std::string &formula_str, int samplerate)
 {
     float tone_freq = 180.0f;     // Базовая частота тела барабана
     float tone_decay = 0.08f;     // Длина тонального удара (деревянного клика)
     float noise_freq = 3200.0f;   // Центральная частота фильтра пружины/шума
     float noise_decay = 0.16f;    // Длина шумового хвоста
     float snappy = 0.50f;         // Баланс: 0.0 = только тон, 1.0 = только шум
     float drive = 0.35f;          // Перегруз аналогового пульта
     float bit_depth = 16.0f;      // Эмуляция Akai сэмплера (12.0 - 16.0)

     std::map<std::string, float> p = parse_synth_params (formula_str);
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

     juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
     float *data = buffer->getWritePointer (0);

     double phase1 = 0.0;
     double phase2 = 0.0;
     uint32_t noise_state = 54321;

     const float dt = 1.0f / static_cast<float>(samplerate);
     //const double TWO_PI = 2.0 * M_PI;

     // Предрасчет множителей огибающих (избавляемся от std::exp в цикле)
     float tone_env_factor = std::exp(-dt * (4.5f / tone_decay));
     float noise_env_factor = std::exp(-dt * (4.0f / noise_decay));
     float pitch_env_factor = std::exp(-dt * 40.0f);

     float env_tone = 1.0f;
     float env_noise = 1.0f;
     float pitch_env = 1.0f;

     // Простой 2-полюсный Band-Pass фильтр для шума (около noise_freq)
     float w0 = static_cast<float>(TWO_PI) * noise_freq * dt;
     float q = 1.8f; // Резонанс полосового фильтра
     float alpha = std::sin(w0) / (2.0f * q);
     float b0 = alpha;
     float b2 = -alpha;
     float a0 = 1.0f + alpha;
     float a1 = -2.0f * std::cos(w0);
     float a2 = 1.0f - alpha;

     float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

     // Шаг квантования для ло-фай сэмплера (вынесено из цикла)
     const float steps = std::pow(2.0f, bit_depth - 1.0f);
     const bool apply_bitcrush = (bit_depth < 15.9f);
     const bool apply_drive = (drive > 0.01f);
     const float drive_mult = 1.0f + drive * 2.5f;

     const float tone_mix = 1.0f - snappy * 0.7f;
     const float noise_mix = snappy * 1.3f;

     for (int i = 0; i < num_samples; ++i)
     {
         float t = static_cast<float>(i) * dt;

         // 1. Тональное тело через инкрементный pitch_env (без std::exp в цикле)
         float f1 = tone_freq + 60.0f * pitch_env;
         float f2 = tone_freq * 1.55f + 80.0f * pitch_env;
         pitch_env *= pitch_env_factor;

         phase1 += TWO_PI * f1 * dt;
         phase2 += TWO_PI * f2 * dt;
         if (phase1 > TWO_PI) phase1 -= TWO_PI;
         if (phase2 > TWO_PI) phase2 -= TWO_PI;

         env_tone *= tone_env_factor;
         float tone_signal = (std::sin(static_cast<float>(phase1)) * 0.6f + std::sin(static_cast<float>(phase2)) * 0.4f) * env_tone;

         // 2. Генератор шума и полосовая фильтрация (Пружина)
         noise_state ^= noise_state << 13;
         noise_state ^= noise_state >> 17;
         noise_state ^= noise_state << 5;
         float raw_noise = static_cast<float>(noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

         // Band-pass фильтр
         float filtered_noise = (b0/a0)*raw_noise + (b2/a0)*x2 - (a1/a0)*y1 - (a2/a0)*y2;
         x2 = x1; x1 = raw_noise;
         y2 = y1; y1 = filtered_noise;

         env_noise *= noise_env_factor;
         float noise_signal = filtered_noise * env_noise;

         // 3. Микширование тона и шума
         float sample = tone_signal * tone_mix + noise_signal * noise_mix;

         // 4. Аналоговая сатурация микшерного пульта
         if (apply_drive)
         {
             sample = std::tanh(sample * drive_mult);
         }

         // 5. Ло-фай квантование Akai S1000
         if (apply_bitcrush)
         {
             sample = std::round(sample * steps) / steps;
         }

         data[i] = sample;
     }

     apply_buffer_fadeout (buffer, samplerate, 0.003f);
     return buffer;
 }



juce::AudioBuffer<float>* render_synth_sample (const std::string &filename, int session_samplerate, bool is_filename)
{
//    std::cout << "juce::AudioBuffer<float>* render_synth_sample: " << filename << "\n";


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



    if (formula_str.rfind ("goasnare(", 0) == 0)
    {
        return render_goasnare (formula_str, session_samplerate);
    }



    if (formula_str.rfind ("hihat_close(", 0) == 0 || formula_str.rfind ("hihat_open(", 0) == 0)
    {
        return render_hihat (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("clap(", 0) == 0)
    {
        return render_clap (formula_str, session_samplerate);
    }


    if (formula_str.rfind ("psykick(", 0) == 0)
    {
        return render_psytrance_kick (formula_str, session_samplerate);
    }

    if (formula_str.rfind ("psytrance_kick(", 0) == 0)
    {
        return render_psykick(formula_str, session_samplerate);
    }


    if (formula_str.rfind ("goakick(", 0) == 0)
    {
        return render_goakick(formula_str, session_samplerate);
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

