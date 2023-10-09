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


#define PANLAW_SINCOS 0
#define PANLAW_SQRT 1
#define PANLAW_LINEAR0 2
#define PANLAW_LINEAR6 3
#define PANLAW_SINCOSV2 4


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


float warmify(float x, float warmth);
float softLimit(float input);


#endif
