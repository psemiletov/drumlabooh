/*
Peter Semiletov, 2023-24
*/


#ifndef DSP_H
#define DSP_H

//#define _USE_MATH_DEFINES

#include <cmath>
#include <limits>
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
  return (val - from_min) * (to_max - to_min) / (from_max - from_min) + to_min;
}


//sin/cos panner, law: -3 dB
//equal power panning
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

//power panner, law: -4.5 dB
#define PANMODE05 5
#define MYHALF_PI 1.5707964

inline void pan_powsin_45 (float &l, float &r, float p)
{
  l = pow (sin ((1 - p) * MYHALF_PI), 1.5);
  r = pow (sin (p * MYHALF_PI), 1.5);
}


//power panner, law: -6 dB
#define PANMODE06 6
inline void pan_powsin_6(float &l, float &r, float p)
{
  l = pow (sin ((1 - p) * MYHALF_PI), 2);
  r = pow (sin (p * MYHALF_PI), 2);
  
}

//sinus panner, law: -1.3 dB
#define PANMODE07 7
inline void pan_sin_1_3 (float &l, float &r, float p) 
{
 // Вычисление амплитуд для левого и правого каналов
  float left = sinf ((1.0f - p) * MYHALF_PI);
  float right = sinf (p * MYHALF_PI);

 // Найдем максимальное значение из амплитуд для нормализации
  float max_amp = std::max (fabs(left), fabs (right));

  if (max_amp == 0.0f) 
     {
      l = 0.0f;
      r = 0.0f;
     }
  else 
      {
       // Нормализация амплитуд с учетом -1.3 dB
       float norm_factor = powf (10.0f, -1.3f / 20.0f) / max_amp;
       l = left * norm_factor;
       r = right * norm_factor;
     }
}


float warmify(float x, float warmth);
float softLimit(float input);


#endif
