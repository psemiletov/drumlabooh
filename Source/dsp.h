#ifndef DSP_H
#define DSP_H

//#define _USE_MATH_DEFINES

#include <cmath>
#include <limits>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

extern double db_scale;

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




//linear panner, law: -6 dB
inline void pan_linear6 (float &l, float& r, float p)
{
  l = 1 - p;
  r = p;
}


//linear panner, law: 0 dB
inline void pan_linear0 (float &l, float& r, float p)
{
  l = 0.5 + (1 - p);
  r = 0.5 + p;
}


//square root panner, law: -3 dB
inline void pan_sqrt (float &l, float& r, float p)
{
  l = sqrt (1 - p);
  r = sqrt (p);
}


//sin/cos panner, law: -3 dB
inline void pan_sincos (float &l, float& r, float p)
{
  float pan = 0.5 * M_PI * p;
  l = cos (pan);
  r = sin (pan);
}

inline void pan_sincos_v2 (float &l, float& r, float p)
{
  float pan = p * M_PI / 2;
  l = l * sin (pan);
  r = r * cos (pan);
}


#endif
