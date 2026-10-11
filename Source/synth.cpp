/*
 w *ritten at 2026 by Peter Semiletov
 this code is the public domain
 */

#include "synth.h"
#include <sstream>
#include <vector>
#include <cmath>
#include <map>
#include <algorithm>
#include <cstdlib>

#include <sstream>
#include <map>


#include <juce_audio_basics/juce_audio_basics.h>
#include <string>



#include "utl.h"


void apply_buffer_fadeout (juce::AudioBuffer<float>* buffer, int samplerate, float fade_duration_sec = 0.003f)
{
    if (!buffer) return;

    int num_samples = buffer->getNumSamples();
    int fade_samples = static_cast<int>(samplerate * fade_duration_sec);

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


// Вспомогательный генератор детерминированного шума (xorshift32)
inline float get_deterministic_noise (uint32_t &state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    // Сначала маска для uint32_t, потом каст в float
    return static_cast<float>(state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;
}


juce::AudioBuffer<float>* render_computer_snare (const std::string &formula_str, int samplerate)
{
    // Настройки в духе "Boing Boom Tschak" (Electric Cafe): ниже тело, густой жирный звук без звона
    float freq = 380.0f;     // Опустили частоту с 950 до плотного средне-низкого диапазона
    float decay = 0.14f;     // Короткий, упругий хвост
    float crunch = 0.55f;    // Плотный шумовой «хруст»
    float tone = 1100.0f;    // Сильно ниже частота фильтра, убирающая весь стеклянный звон

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

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;
        float sample = 0.0f;

        // 1. Плотное электронное тело (прямоугольная волна с легким сбросом частоты для «панча»)
        if (t < decay)
        {
            // Небольшой питч-бенд вниз в самом начале для веса (как в "Tschak")
            float current_freq = freq + freq * 0.5f * std::exp(-t * 60.0f);
            phase += 2.0 * M_PI * current_freq * dt;
            float square = (std::sin(phase) >= 0.0f) ? 1.0f : -1.0f;

            float env = std::exp (-t * (6.0f / decay));
            sample += square * env * 0.45f;
        }

        // 2. Сухой шумовой компонент (приглушенный crunch)
        comp_snare_state ^= comp_snare_state << 13;
        comp_snare_state ^= comp_snare_state >> 17;
        comp_snare_state ^= comp_snare_state << 5;
        float raw_noise = static_cast<float>(comp_snare_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        float noise_env = std::exp (-t * (9.0f / decay));
        if (t < 0.0015f) noise_env *= (t / 0.0015f); // быстрый плотный щелчок на старте

        sample += lpf_state * noise_env * crunch;

        // Плотный аналоговый сатуратор для склейки тела и шума
        sample = std::tanh (sample * 1.4f);

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}

juce::AudioBuffer<float>* render_computer_tom (const std::string &formula_str, int samplerate)
{
    float base_freq = 150.0f;  // Базовая нота тома
    float decay = 0.35f;       // Длительность
    float click_mix = 0.2f;    // Уровень "роботизированного" щелчка

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

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) / samplerate;

        // Фирменный резкий питч-бенд Kraftwerk (начинается выше и быстро падает к базовой ноте)
        float freq = base_freq + base_freq * 1.5f * std::exp (-t * 30.0f);
        phase += 2.0 * M_PI * freq / samplerate;

        // Основное тело (чистая синусоида с экспоненциальным затуханием)
        float env = std::exp (-t * (4.0f / decay));
        float sample = std::sin (phase) * env;

        // Короткий прямоугольный импульс (скрежет процессора/калькулятора) в атаке
        if (t < 0.008f)
        {
            click_phase += 2.0 * M_PI * (base_freq * 3.0f) / samplerate;
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
    // Делаем дефолты более темными и плотными (без звона)
    float f0 = 210.0f;       // Ниже стартовая частота
    float f1 = 115.0f;       // Ниже тело
    float decay = 0.17f;
    float crunch = 0.5f;     // Умеренный уровень шума
    float tone = 900.0f;     // Значительно ниже частота среза фильтра (убираем звон)

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

    // Используем мягкий ФНЧ вместо звенящего полосового фильтра
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (2.0f * 3.14159265f * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;
        float sample = 0.0f;

        // 1. Тональное тело (глухой, плотный каркас)
        if (t < decay)
        {
            float freq = f1 + (f0 - f1) * std::exp (-t * 40.0f);
            phase += 2.0 * M_PI * freq * dt;
            float env = std::exp (-t * (6.0f / decay));
            sample += std::sin (phase) * env * 0.5f;
        }

        // 2. Индустриальный шумовой компонент (темный crunch)
        cw_noise_state ^= cw_noise_state << 13;
        cw_noise_state ^= cw_noise_state >> 17;
        cw_noise_state ^= cw_noise_state << 5;
        float raw_noise = static_cast<float>(cw_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

        // Фильтруем шум через НЧ, чтобы убрать стеклянное шипение
        lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

        float noise_env = std::exp (-t * (8.0f / decay));
        if (t < 0.002f) noise_env *= (t / 0.002f);

        sample += lpf_state * noise_env * crunch;

        // Легкое ограничение (soft clipping) для уплотнения в стиле лоу-фай / пост-панк
        sample = std::tanh(sample * 1.2f);

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f);
    return buffer;
}


juce::AudioBuffer<float>* render_psytrance_kick (const std::string &formula_str, int samplerate)
{
    // Дефолтные параметры для psytrance kick: выше старт, глубже и быстрее сброс
    float f0 = 240.0f;       // Высокий резкий старт для щелчка
    float f1 = 38.0f;        // Низкий плотный саб-фундамент
    float decay = 0.22f;     // Короткий, собранный хвост (чтобы бас-лайн шел встык)
    float click_level = 0.35f; // Более агрессивный клик

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

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) / samplerate;

        // Очень агрессивный экспоненциальный питч-бенд (быстрый спад частоты)
        float freq = f1 + (f0 - f1) * std::exp (-t * 35.0f);
        phase += 2.0 * M_PI * freq / samplerate;

        // Быстрая амплитудная огибающая
        float env = std::exp (-t * (4.0f / decay));
        float sample = std::sin (phase) * env;

        // Плотный шумовой и гармонический клик в первые 4 миллисекунды
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
    std::cout << "HIHAT: " << formula_str << "\n";

    float decay = 0.08f;
    float tone = 7000.0f;
    float noise_mix = 0.6f;
    float metal_mix = 0.4f;
    float pitch = 1.0f;

    bool is_open = (formula_str.rfind ("hihat_open(", 0) == 0);
    if (is_open)
    {
        decay = 0.35f;
    }

    // Исправлено: вызываем parse_synth_params вместо parse_formula_params
    auto params = parse_synth_params (formula_str);
    if (params.count("decay"))     decay = params["decay"];
    if (params.count("tone"))      tone = params["tone"];
    if (params.count("noise"))     noise_mix = params["noise"];
    if (params.count("metal"))     metal_mix = params["metal"];
    if (params.count("pitch"))     pitch = params["pitch"];

    int num_samples = static_cast<int>(samplerate * (decay * 1.2f));
    if (num_samples < 100) num_samples = 100;

    auto* buffer = new juce::AudioBuffer<float>(1, num_samples);
    float* data = buffer->getWritePointer(0);

    uint32_t noise_state = 54321;

    float metal_freqs[6] = { 405.3f, 535.1f, 723.4f, 845.2f, 1150.0f, 1420.0f };
    float metal_phases[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    float hpf_prev_input = 0.0f;
    float hpf_prev_output = 0.0f;

    float dt = 1.0f / static_cast<float>(samplerate);
    float rc = 1.0f / (2.0f * 3.14159265f * tone);
    float alpha = rc / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        float metal_sample = 0.0f;
        for (int m = 0; m < 6; ++m)
        {
            metal_phases[m] += (metal_freqs[m] * pitch) * dt;
            if (metal_phases[m] >= 1.0f) metal_phases[m] -= 1.0f;
            float sq = (metal_phases[m] < 0.5f) ? 1.0f : -1.0f;
            metal_sample += sq;
        }
        metal_sample /= 6.0f;

        float noise_sample = get_deterministic_noise(noise_state);

        float raw_mix = (metal_sample * metal_mix) + (noise_sample * noise_mix);

        float hpf_output = alpha * (hpf_prev_output + raw_mix - hpf_prev_input);
        hpf_prev_input = raw_mix;
        hpf_prev_output = hpf_output;

        float env = std::exp(-3.0f * (t / decay));
        if (t < 0.002f) {
            env *= (t / 0.002f);
        }

        data[i] = hpf_output * env;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f); // 3 мс на сглаживание хвоста
    return buffer;
}



/*

// Вспомогательная функция для парсинга параметров через ';'
std::map<std::string, float> parse_synth_params (const std::string &formula_str)
{

    std::map<std::string, float> params;

    // Находим скобки
    size_t start = formula_str.find ('(');
    size_t end = formula_str.rfind (')');
    if (start == std::string::npos || end == std::string::npos || end <= start)
        return params; // Возвращает пустой мап, генератор применит дефолты

        std::string inner = formula_str.substr (start + 1, end - start - 1);

    // Разбиваем по точке с запятой
    std::stringstream ss(inner);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        // Убираем пробелы (если нужно) или сразу ищем '='
        size_t eq = item.find ('=');
        if (eq != std::string::npos)
        {
            std::string key = item.substr(0, eq);
            std::string val_str = item.substr(eq + 1);

            // Тримминг пробелов по вкусу, затем конвертация в float
            try {
                float val = std::stof(val_str);
                // Убираем возможные пробелы из ключа
                key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
                params[key] = val;
            } catch (...) {}
        }
    }

    return params;
}
*/



juce::AudioBuffer<float>* render_clap (const std::string &formula_str, int session_samplerate)
{
    std::cout << "CLAP: " << formula_str << "\n";

    float decay = 0.28f;
    float tone = 1200.0f;      // Чуть ниже частота, чтобы убрать «стекло»
    float spread = 0.011f;     // 11 мс между микро-хлопками
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

    // Состояние простого однополюсного фильтра (LPF + HPF для мягкого тела)
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(session_samplerate);

    // Коэффициент фильтрации низких частот (срезаем визгливый верх)
    float rc = 1.0f / (2.0f * 3.14159265f * tone);
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;

        // 1. Сырой шум
        float noise = get_deterministic_noise(noise_state);

        // 2. Пачка хлопков (микро-атака)
        float burst_env = 0.0f;
        for (int b = 0; b < burst_count; ++b)
        {
            float burst_time = static_cast<float>(b) * spread;
            if (t >= burst_time)
            {
                float dt_burst = t - burst_time;
                // Быстрые колоколообразные огибающие для каждого хлопка
                burst_env += std::exp(-dt_burst * 400.0f);
            }
        }

        // 3. Плавный хвост реверберации
        float tail_env = std::exp(-t * (3.5f / decay));

        // Соединяем пачку и хвост
        float total_env = (burst_env * 0.8f) + (tail_env * 0.2f);

        // 4. Мягкая фильтрация низких частот (делает звук «телесным», убирая стеклянный звон)
        lpf_state = lpf_state + alpha * (noise - lpf_state);

        // Дополнительно делаем легкий HPF, чтобы не было гула ниже ~500 Гц
        // (просто вычитаем часть очень низких частот)
        float filtered = lpf_state;

        data[i] = filtered * total_env * 1.2f;
    }

    apply_buffer_fadeout (buffer, session_samplerate, 0.003f);

    return buffer;
}

/*

juce::AudioBuffer<float>* render_snare (const std::string &formula_str, int samplerate)
{
    float f0 = 220.0f;       // Начальная частота корпуса
    float f1 = 130.0f;       // Конечная частота корпуса
    float decay = 0.2f;      // Длительность тела
    float noise_level = 0.7f; // Громкость пружин
    float noise_decay = 0.3f; // Длительность шуршания пружин

    // Парсим параметры (теперь через запятую!)
    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("f0"))       f0 = p["f0"];
    if (p.count("f1"))       f1 = p["f1"];
    if (p.count("decay"))    decay = p["decay"];
    if (p.count("noise"))    noise_level = p["noise"];
    if (p.count("n_decay"))  noise_decay = p["n_decay"];

    int num_samples = static_cast<int>(samplerate * std::max(decay, noise_decay));
    if (num_samples <= 0)
        num_samples = samplerate * 0.3f;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    double phase = 0.0;
    float noise_state = 0.0f; // Для простого фильтра шума

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) / samplerate;
        float sample = 0.0f;

        // 1. Тональная часть (корпус / пластик)
        if (t < decay)
        {
            float freq = f1 + (f0 - f1) * std::exp (-t * 35.0f);
            phase += 2.0 * M_PI * freq / samplerate;
            float env = std::exp (-t * (5.0f / decay));
            sample += std::sin (phase) * env * 0.5f;
        }

        // 2. Шумовая часть (пружины / snare wires)
        if (t < noise_decay)
        {
            float white_noise = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
            // Простейший фильтр низких частот для смягчения шума
            noise_state = noise_state * 0.5f + white_noise * 0.5f;
            float noise_env = std::exp (-t * (3.0f / noise_decay));
            sample += noise_state * noise_env * noise_level;
        }

        data[i] = sample;
    }

    apply_buffer_fadeout (buffer, samplerate, 0.003f); // 3 мс на сглаживание хвоста
    return buffer;
}
*/

juce::AudioBuffer<float>* render_snare (const std::string &formula_str, int samplerate)
{
    // Дефолтные параметры для собранного, хлесткого снейра
    float f0 = 200.0f;        // Начальная частота пластика
    float f1 = 110.0f;        // Конечная частота корпуса
    float decay = 0.18f;      // Общая длительность тела (делаем короче)
    float noise_level = 0.6f; // Громкость пружин
    float noise_decay = 0.12f;// Быстрое затухание пружин (чтобы не было шипения в конце)

    // Парсим параметры из формулы
    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("f0"))      f0 = p["f0"];
    if (p.count("f1"))      f1 = p["f1"];
    if (p.count("decay"))   decay = p["decay"];
    if (p.count("noise"))   noise_level = p["noise"];
    if (p.count("n_decay")) noise_decay = p["n_decay"];

    // Длина буфера по максимальному времени затухания
    int num_samples = static_cast<int>(samplerate * std::max(decay, noise_decay) * 1.1f);
    if (num_samples <= 0)
        num_samples = samplerate * 0.25f;

    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    double phase = 0.0;

    // Детерминированный шум для снейра (чтобы тоже звучал стабильно от сессии к сессии)
    uint32_t snare_noise_state = 12345;
    float lpf_state = 0.0f;
    float dt = 1.0f / static_cast<float>(samplerate);

    // Фильтр для пружин (срезаем визгливый верх и гудящий низ, оставляя плотный песок)
    float rc = 1.0f / (2.0f * 3.14159265f * 2500.0f); // частота среза ~2.5 кГц
    float alpha = dt / (rc + dt);

    for (int i = 0; i < num_samples; ++i)
    {
        float t = static_cast<float>(i) * dt;
        float sample = 0.0f;

        // 1. Тональная часть (тело / пластик барабана)
        if (t < decay)
        {
            // Быстрый питч-бенд (удар палочки)
            float freq = f1 + (f0 - f1) * std::exp (-t * 45.0f);
            phase += 2.0 * M_PI * freq * dt;

            // Экспоненциальная огибающая тела
            float env = std::exp (-t * (6.0f / decay));
            sample += std::sin (phase) * env * 0.45f;
        }

        // 2. Шумовая часть (пружины / snare wires)
        if (t < noise_decay)
        {
            // Используем детерминированный шум вместо случайного rand()
            snare_noise_state ^= snare_noise_state << 13;
            snare_noise_state ^= snare_noise_state >> 17;
            snare_noise_state ^= snare_noise_state << 5;
            float raw_noise = static_cast<float>(snare_noise_state & 0x7FFFFFFF) / 1073741824.0f - 1.0f;

            // Смягчаем шум фильтром
            lpf_state = lpf_state + alpha * (raw_noise - lpf_state);

            // Резкая огибающая пружин (быстрый старт, короткий плотный хвост без долгого шипения)
            float noise_env = std::exp (-t * (8.0f / noise_decay));

            // Легкий щелчок в самую первую миллисекунду для плотности атаки
            if (t < 0.003f) {
                noise_env *= (t / 0.003f) * 1.5f;
            }

            sample += lpf_state * noise_env * noise_level;
        }

        data[i] = sample;
    }

    // Фирменный короткий fade-out, чтобы исключить любые щелчки на стыках
    apply_buffer_fadeout (buffer, samplerate, 0.003f);

    return buffer;
}

juce::AudioBuffer<float>* render_kick (const std::string &formula_str, int samplerate)
{

std::cout << "juce::AudioBuffer<float>* render_kick BEGIN \n";

std::cout << "DEBUB 01\n";
    float f0 = 150.0f;
    float f1 = 45.0f;
    float decay = 0.35f;
    float click_level = 0.2f;

std::cout << "DEBUB 02\n";

    // Парсим параметры из строки
    std::map<std::string, float> p = parse_synth_params (formula_str);
    if (p.count("f0"))    f0 = p["f0"];
    if (p.count("f1"))    f1 = p["f1"];
    if (p.count("decay")) decay = p["decay"];
    if (p.count("click")) click_level = p["click"];


    std::cout << "DEBUB 03\n";


    int num_samples = static_cast<int>(samplerate * decay);
    if (num_samples <= 0)
        num_samples = samplerate * 0.5f;

    std::cout << "DEBUB 04\n";



    juce::AudioBuffer<float> *buffer = new juce::AudioBuffer<float> (1, num_samples);
    float *data = buffer->getWritePointer (0);

    std::cout << "DEBUB 05\n";


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

    std::cout << "DEBUB 06\n";

    std::cout << "juce::AudioBuffer<float>* render_kick END \n";

    apply_buffer_fadeout (buffer, samplerate, 0.003f); // 3 мс на сглаживание хвоста
    return buffer;
}

juce::AudioBuffer<float>* render_synth_sample (const std::string &filename, int session_samplerate)
{
    std::cout << "juce::AudioBuffer<float>* render_synth_sample: " << filename << "\n";

    std::string formula_str = string_file_load (filename);



    if (formula_str.rfind ("kick(", 0) == 0)
       {
         std::cout << "KICK\n";

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


    return nullptr;
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
