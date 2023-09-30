#ifndef DSP_H
#define DSP_H

//#define _USE_MATH_DEFINES

#include <cmath>
#include <limits>
//#include <ostream>
#include <iostream>
#include <vector>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

extern float db_scale;

inline float db2lin (float db)
{
  return (float) exp (db * db_scale);
}

/*
static inline float db2lin (float db)
{
  return powf (10.0f, db / 20);
}
*/


inline float float2db (float v)
{
  if (v == 0.0f)
     return 0.0f;

  if (v > 0.0f)
     return (float) 20 * log10 (v / 1.0);

  return (float) 20 * log10 (v / -1.0);
}


void init_db();


inline bool float_greater_than (float a, float b)
{
  return (a - b) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * std::numeric_limits<double>::epsilon());
}


inline bool float_less_than (float a, float b)
{
  return (b - a) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * std::numeric_limits<double>::epsilon());
}


inline bool float_equal (float x, float y)
{
  return std::abs(x - y) <= std::numeric_limits<double>::epsilon() * std::abs(x);
}


inline float conv (float v, float middle, float max)
{
  if (v == middle)
     return 0;

  if (v > middle)
     return (max - middle - v);
  else
      return middle - v;
}


inline float conv_to_db (float v, float v_min, float v_max, float range_negative, float range_positive)
{
  if (v == 0)
     return 0;

  if (v > 0)
    {
     float x = v_max / range_positive;
     float y = v_max / v;

     return v / (y * x);
    }
  else
      {
       float x = v_min / range_negative;
       float y = v_min / v;

       return v / (y * x);
      }
}


inline float scale_val (float val, float from_min, float from_max, float to_min, float to_max)
{
  return (val - from_min) * (to_max - to_min) /
          (from_max - from_min) + to_min;
}


#define PANLAW_SINCOS 0
#define PANLAW_SQRT 1
#define PANLAW_LINEAR0 2
#define PANLAW_LINEAR6 3
#define PANLAW_SINCOSV2 4


//sin/cos panner, law: -3 dB
#define PANMODE01 1
inline void pan_sincos (float &l, float& r, float p)
{
  float pan = 0.5 * M_PI * p;
  l = cos (pan);
  r = sin (pan);
}


//square root panner, law: -3 dB
#define PANMODE02 2
inline void pan_sqrt (float &l, float& r, float p)
{
  l = sqrt (1 - p);
  r = sqrt (p);
}


//linear panner, law: 0 dB
#define PANMODE03 3
inline void pan_linear0 (float &l, float& r, float p)
{
  l = 0.5 + (1 - p);
  r = 0.5 + p;
}


//linear panner, law: -6 dB
#define PANMODE04 4
inline void pan_linear6 (float &l, float& r, float p)
{
  l = 1 - p;
  r = p;
}



inline void pan_sincos_v2 (float &l, float& r, float p)
{
  float pan = p * M_PI / 2;
  l = l * sin (pan);
  r = r * cos (pan);
}


//power panner, law: -4.5 dB
#define PANMODE05 5
inline void pan_power45 (float &l, float& r, float p)
{
  l  = powf ((1 - p), 0.75) * l;
  r = powf (p, 0.75) * r;
}

//power panner, law: -1.5 dB
//  -1.5dB = 10^(-1.5/20) = 0.841395142 (power taper)
#define PANMODE06 6
inline void pan_power15 (float &l, float& r, float p)
{
  l  = powf ((1 - p), 0.25) * l;
  r = powf (p, 025) * r;
}

//equal power panner, law: -3 dB
//  -3dB = 10^(-3/20) = 0.707945784
#define PANMODE07 7
inline void pan_equal_power3 (float &l, float& r, float p)
{
  l  = sqrt (1 - p) * l; // = power((1-pan),0.5) * MonoIn;
  r = sqrt(p) * r; // = power(pan,0.5) * MonoIn
}


/*

-6dB = 10^(-6/20) = 0.501187234 (linear)
LeftOut  = (1-pan) * MonoIn // = power((1-pan),1) * MonoIn;
RightOut = pan * MonoIn // = power(pan,1) * MonoIn;

 */

//from https://www.musicdsp.org/en/latest/Effects/42-soft-saturation.html
//by Bram de Jong

inline float soft_saturate (float in, float a)
{
  float result = 0;
  if (in < a)
     result = in;

  if (in > a)
      result = a + (in - a)/ powf (1+((in - a) / (1 - a)),2);

 if (in > 1)
    result = (a + 1) / 2;

  //You can normalise the output:
//f(x)'=f(x)*(1/((a+1)/2))

    result = result*(1/((a+1)/2));


  return result;
}


void bjtfilt(float* x, int samples_count, double fs);
void opampfilt(float* x, int samples_count, double fs, double DIST);
void diodeclip(float* x, int samples_count);


template <typename T>
T clampTo01(const T& value) {
    if (value < 0.0) {
        return 0.0;
    } else if (value > 1.0) {
        return 1.0;
    } else {
        return value;
    }
}

class DistortionProcessor {
public:
    float fs;

    DistortionProcessor() : fs(44100) {}

// Функция для вычисления filter с рациональными коэффициентами b и a
float filter(const std::vector<double>& b, const std::vector<double>& a, const float x, std::vector<double>& state) {
    double y = b[0] * x;

    for (size_t i = 1; i < b.size(); i++) {
        if (i <= state.size()) {
            y += b[i] * x - a[i] * state[i - 1];
        }
    }

    for (size_t i = 1; i < state.size(); i++) {
        state[i] = state[i - 1];
    }

    if (state.size() > 0) {
        state[0] = x;
    }

    if (a[0] != 1.0) {
        for (size_t i = 0; i < state.size(); i++) {
            state[i] /= a[0];
        }
    }

    return static_cast<float>(y);
}
// Функция для вычисления bjtfilt
float bjtfilt(float x, float fs) {
    // Рассчитываем коэффициент coeff
    double coeff = M_PI / fs;

    // Определяем коэффициенты фильтра
    std::vector<double> B = {1800.0 * pow(coeff, 2) + 603.0 * coeff + 1.0,
                             3600.0 * pow(coeff, 2) - 2.0,
                             1800.0 * pow(coeff, 2) - 603.0 * coeff + 1.0};

    // Получаем усиление из 36 дБ
    double amp = pow(10, (36.0 / 20.0));
    for (size_t i = 0; i < B.size(); i++) {
        B[i] *= amp;
    }

    std::vector<double> state(B.size() - 1, 0.0);
    return filter(B, B, x, state);
}


    // Op-Amp Gain Stage
    float opampfilt(float x, float DIST) {
        double Rt = 100000 * DIST;
        double Rb = 100000 * (1 - DIST) + 4700;
        double Cz = 0.000001;
        double Cc = 0.000000000250;

        double c = 2 * fs;

        double ab0 = 1 / (Rt * Cc * Rb * Cz);
        double a1 = 1 / (Rb * Cz) + 1 / (Rt * Cc);
        double b1 = a1 + 1 / (Rb * Cc);

        double B0 = ab0 + b1 * c + pow(c, 2);
        double B1 = 2 * ab0 - 2 * pow(c, 2);
        double B2 = ab0 - b1 * c + pow(c, 2);

        double A0 = ab0 + a1 * c + pow(c, 2);
        double A1 = B1;
        double A2 = ab0 - a1 * c + pow(c, 2);

        double y = B0 * x;
        if (prev_1_initialized_) y += B1 * prev_1_;
        if (prev_2_initialized_) y += B2 * prev_2_;

        if (prev_1_initialized_) y -= A1 * prev_1_;
        if (prev_2_initialized_) y -= A2 * prev_2_;

        prev_2_ = prev_1_;
        prev_1_ = y;
        prev_1_initialized_ = true;
        prev_2_initialized_ = true;

        return static_cast<float>(y / A0);
    }

    // Diode clipper
    float diodeclip(float x) {
        float n = 2.5;
        return static_cast<float>(x / pow(1 + pow(fabs(x), n), 1.0 / n));
    }



    float processSignal(float input, float DIST)
    {
    float a = bjtfilt(input, fs);

    std::cout << "a:" << a << std::endl;

    float b = opampfilt(a, DIST);

    std::cout << "b:" << a << std::endl;

    // Ограничиваем значения в диапазоне от 0 до 1
    //float c = clampTo01(b);

        float c = b;


    std::cout << "c:" << c << std::endl;

    float d = diodeclip(c);

    return d;
}






private:
    float prev_1_ = 0.0;
    float prev_2_ = 0.0;
    bool prev_1_initialized_ = false;
    bool prev_2_initialized_ = false;
};


float grittyGuitarDistortion(float inputSample, float distortionLevel);
float guitarDistortion(float inputSample, float distortionLevel);
float gabberDistortion(float inputSample, float distortionLevel);


float warmify(float x, float warmth);

/*
int main() {
    TransientShaper shaper(0.01, 0.1); // Пример временных параметров атаки и спада

    std::vector<float> inputSignal = {0.2, 0.5, 0.8, 0.6, 0.3};

    for (float sample : inputSignal) {
        float processedSample = shaper.processAttack(sample);
        std::cout << "Processed sample: " << processedSample << std::endl;
    }

    return 0;
}
*/



#include <cmath>

class MoogFilter {
public:
    MoogFilter() {
        reset();
    }

    // Установка параметров фильтра
    void setParams(float cutoff, float resonance) {
        if (cutoff < 0.01f) cutoff = 0.01f;
        if (cutoff > 0.99f) cutoff = 0.99f;
        if (resonance < 0.0f) resonance = 0.0f;
        if (resonance > 1.0f) resonance = 1.0f;

        cutoff_ = cutoff;
        resonance_ = resonance;
    }

    // Сброс состояния фильтра
    void reset() {
        for (int i = 0; i < 4; i++) {
            buf_[i] = 0.0f;
        }
    }

    // Применение фильтра к входному сэмплу
    float process(float input) {
        float g = static_cast<float>(2.0 - 2.0 * cutoff_);
        float k = static_cast<float>(4.0 * resonance_);

        float t1 = buf_[0] - k * buf_[3];
        float t2 = buf_[1] - k * buf_[0];
        float t3 = buf_[2] - k * buf_[1];
        float t4 = input - k * buf_[2];

        buf_[0] += g * t1;
        buf_[1] += g * t2;
        buf_[2] += g * t3;
        buf_[3] += g * t4;

        buf_[0] = clip(buf_[0]);
        buf_[1] = clip(buf_[1]);
        buf_[2] = clip(buf_[2]);
        buf_[3] = clip(buf_[3]);

        return buf_[3];
    }

private:
    float clip(float x) {
        if (x > 1.0f) return 1.0f;
        if (x < -1.0f) return -1.0f;
        return x;
    }

    float cutoff_ = 0.5f;
    float resonance_ = 0.5f;
    float buf_[4];
};
/*
int main() {
    MoogFilter filter;
    filter.setParams(0.5f, 0.5f); // Установка параметров фильтра

    // Применение фильтра к входным сэмплам
    float input[5] = {0.1f, 0.2f, -0.1f, -0.2f, 0.3f};
    for (int i = 0; i < 5; i++) {
        float output = filter.process(input[i]);
        // Здесь можно использовать выходной сэмпл output
    }

    return 0;
}
*/

#endif
