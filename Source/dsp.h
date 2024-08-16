/*
Peter Semiletov, 2023
*/


#ifndef DSP_H
#define DSP_H
/*
#if defined(__clang__)
//
#elif defined(__GNUC__) || defined(__GNUG__)

#define MXCSR_DAZ (1<<6)
#define MXCSR_FTZ (1<<15)

#elif defined(_MSC_VER)
//
#endif
*/

//#define _USE_MATH_DEFINES

#include <cmath>
#include <limits>
//#include <ostream>
#include <iostream>
#include <vector>

#include <algorithm>

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

/*
inline float float2db (float v)
{
  if (v == 0.0f)
     return 0.0f;

  if (v > 0.0f)
     return (float) 20 * log10 (v / 1.0f);

  return (float) 20 * log10 (v / -1.0f);
}
*/

void init_db();



///

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

/*
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
*/

inline float scale_val (float val, float from_min, float from_max, float to_min, float to_max)
{
  return (val - from_min) * (to_max - to_min) /
          (from_max - from_min) + to_min;
}

/*
#define PANLAW_SINCOS 0
#define PANLAW_SQRT 1
#define PANLAW_LINEAR0 2
#define PANLAW_LINEAR6 3
#define PANLAW_SINCOSV2 4
*/

//sin/cos panner, law: -3 dB
#define PANMODE01 1
inline void pan_sincos (float &l, float& r, float p)
{
  float pan = 0.5f * M_PI * p;
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

//-6!
/*inline void pan_sqrt(float &l, float &r, float p) {
    p = std::clamp(p, 0.0f, 1.0f);  // Ограничение p в диапазоне [0, 1]
    
    // Нормализация коэффициентом sqrt(2) для закона -3 дБ
    float norm_factor = sqrt(2.0f);
    
    l = sqrt(1.0f - p) / norm_factor;
    r = sqrt(p) / norm_factor;
}
*/
//linear panner, law: 0 dB
#define PANMODE03 3
inline void pan_linear0 (float &l, float& r, float p)
{
  l = 0.5f + (1 - p);
  r = 0.5f + p;
}


//linear panner, law: -6 dB
#define PANMODE04 4
inline void pan_linear6 (float &l, float& r, float p)
{
  l = 1 - p;
  r = p;
}


/*
inline void pan_sincos_v2 (float &l, float& r, float p)
{
  float pan = p * M_PI / 2;
  l = l * sin (pan);
  r = r * cos (pan);
}
*/

//power panner, law: -4.5 dB
#define PANMODE05 5

#define MYHALF_PI 1.5707964

// Функция панорамирования с законом -4.5 дБ
inline void pan_powsin_45(float &l, float &r, float p)
{
  l = pow( sin((1 - p)*MYHALF_PI) , 1.5);
  r = pow( sin(p*MYHALF_PI), 1.5);
  
}



/*inline void pan_power45 (float &l, float& r, float p)
{
  l = powf ((1 - p), 0.75) * l;
  r = powf (p, 0.75) * r;
}
*/
//на деле - 7.5
/*inline void pan_power45(float &l, float &r, float p) {
    p = std::clamp(p, 0.0f, 1.0f);  // Ограничение p в диапазоне [0, 1]

    // Нормализация для закона -4.5 дБ в центре
    float norm_factor = powf(10.0f, -4.5f / 20.0f);  // ≈ 0.5946

    // Применение panning формулы с нормализацией для закона -4.5 дБ
    l = sqrt(1.0f - p) * norm_factor;
    r = sqrt(p) * norm_factor;
}
*/

//- 3.5
/*inline void pan_power45(float &l, float &r, float p) {
    p = std::clamp(p, 0.0f, 1.0f);  // Ограничение p в диапазоне [0, 1]

    // Вычисление уровней громкости для левого и правого каналов
    float left = sqrt(1.0f - p);
    float right = sqrt(p);

    // Применение коэффициента нормализации для закона -4.5 дБ в центре
    // Важно: масштабируем уровни с учетом их квадратичной суммы, чтобы в центре было уменьшение на -4.5 дБ
    float scale = 1.0f / sqrt(powf(left, 2) + powf(right, 2) * powf(10.0f, -4.5f / 10.0f));

    l = left * scale;
    r = right * scale;
}
*/


//power panner, law: -1.5 dB
//  -1.5dB = 10^(-1.5/20) = 0.841395142 (power taper)
#define PANMODE06 6
/*inline void pan_power15 (float &l, float& r, float p)
{
  l  = powf ((1 - p), 0.25) * l;
  r = powf (p, 025) * r;
}
*/

inline void pan_powsin_6(float &l, float &r, float p)
{
  l = pow( sin((1 - p)*MYHALF_PI) , 2);
  r = pow( sin(p*MYHALF_PI), 2);
  
}

//equal power panner, law: -3 dB
//  -3dB = 10^(-3/20) = 0.707945784
#define PANMODE07 7
/*inline void pan_equal_power3 (float &l, float& r, float p)
{
  l  = sqrt (1 - p) * l; // = power((1-pan),0.5) * MonoIn;
  r = sqrt(p) * r; // = power(pan,0.5) * MonoIn
}

*/

inline void pan_sin_1_3(float &l, float &r, float p) {
    p = std::clamp(p, 0.0f, 1.0f);  // Ограничение p в диапазоне [0, 1]

    // Вычисление амплитуд для левого и правого каналов
    float left = sinf((1.0f - p) * MYHALF_PI);
    float right = sinf(p * MYHALF_PI);

    // Найдем максимальное значение из амплитуд для нормализации
    float max_amp = std::max(fabs(left), fabs(right));

    // Если max_amp равен нулю, устанавливаем значения в ноль
    if (max_amp == 0.0f) {
        l = 0.0f;
        r = 0.0f;
    } else {
        // Нормализация амплитуд с учетом -1.3 dB
        float norm_factor = powf(10.0f, -1.3f / 20.0f) / max_amp; // ≈ 0.943
        l = left * norm_factor;
        r = right * norm_factor;
    }
}



inline void pan_powsin_1_3(float &l, float &r, float p) {
    p = std::clamp(p, 0.0f, 1.0f);  // Ограничение p в диапазоне [0, 1]

    // Вычисление громкости для левого и правого каналов
    float left = powf(sinf((1.0f - p) * MYHALF_PI), 2);
    float right = powf(sinf(p * MYHALF_PI), 2);

    // Коэффициент нормализации для обеспечения правильного уровня в центре
    float norm_factor = powf(10.0f, -1.3f / 20.0f);  // ≈ 0.943

    // Общая мощность до нормализации
    float total_power = sqrtf(left * left + right * right);

    // Нормализация так, чтобы уровень в центре соответствовал -1.3 дБ
    float scale = norm_factor / total_power;

    l = left * scale;
    r = right * scale;
}



#define PANMODE08 8
//square root panner, law: -6 dB
inline void pan_sqrt6(float &l, float &r, float p) {
    p = std::clamp(p, 0.0f, 1.0f);  // Ограничение p в диапазоне [0, 1]
    l = sqrt(1.0f - p);
    r = sqrt(p);
}


float warmify(float x, float warmth);
float softLimit(float input);


#endif
