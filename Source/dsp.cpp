/*
Peter Semiletov, 2023
*/

#include <vector>
#include <cmath>

#include "dsp.h"


using namespace std;


float db_scale;

void init_db()
{
/*
#if defined(__clang__)
//
#elif defined(__GNUC__) || defined(__GNUG__)

  unsigned int mxcsr = __builtin_ia32_stmxcsr ();
  mxcsr |= MXCSR_DAZ | MXCSR_FTZ;
  __builtin_ia32_ldmxcsr (mxcsr);

#elif defined(_MSC_VER)
//
#endif
*/


  db_scale = log (10.0) * 0.05;


}


//analog
float warmify(float x, float warmth)
{
  // Проверяем, что "warmth" находится в пределах от 0 до 1
  warmth = std::min(1.0f, std::max(0.0f, warmth));

  // Применяем теплое, аналоговое воздействие на сигнал
  float warm_x = x * (1.0f - warmth) + std::sin(x * M_PI) * warmth;

  // Ограничиваем значения в диапазоне от -1 до 1
  warm_x = std::min(1.0f, std::max(-1.0f, warm_x));

  return warm_x;
}


/*
// Функция для мягкого понижения уровня сигнала, если он превышает порог в 18 дБ (выше нуля)
float softLimit(float input, float threshold_dB)
{
  // Проверяем, превышает ли входной сигнал порог
  if (input > 0.0f && 20.0f * log10f(input) > threshold_dB)
     {
      // Рассчитываем множитель для понижения уровня
      float reductionFactor = pow(10.0f, (threshold_dB - 20.0f * log10f(input)) / 20.0f);
      // Применяем мягкое понижение уровня
      return input * reductionFactor;
    }

  // Если сигнал не превышает порог, возвращаем его без изменений
  return input;
}
*/

const float thresholdLevel = pow(10.0f, 18.0f / 20.0f); // Значение, соответствующее +18 дБ


float softLimit(float input) {

    // Проверяем, превышает ли входной сигнал порог
    if (input > thresholdLevel) {
        // Рассчитываем множитель для понижения уровня
        float reductionFactor = thresholdLevel / input;

        // Применяем мягкое понижение уровня
        return input * reductionFactor;
    }

    // Если сигнал не превышает порог, возвращаем его без изменений
    return input;
}