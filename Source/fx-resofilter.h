#ifndef FXRESOFILTER_H
#define FXRESOFILTER_H


/*
Peter Semiletov, 2023
*/

/*
this code based on Martin Finke's filter tutorial, https://www.martin-finke.de/articles/audio-plugins-013-filter/

where the filter itself based on the resonant filter 
by Paul Kellett http://www.musicdsp.org/showone.php?id=29
*/

#include <cstring>
#include <cmath>

enum filter_mode
     {
      FILTER_MODE_LOWPASS = 0,
      FILTER_MODE_HIGHPASS,
      FILTER_MODE_BANDPASS
     };


class CResoFilter
{
public:

  float resonance;
  int mode;
  float feedback_amount;
  float cutoff; // filter.set_cutoff ((float) dsb_cutoff_freq / session samplerate);
  //actually, min 0.01, max 0.99, step 0.001 def 0.99
  //cutoff must be < 1.0 or will be div by zero at calc_feedback_amount

  float bufl0;
  float bufl1;
  float bufl2;
  float bufl3;
  float bufr0;
  float bufr1;
  float bufr2;
  float bufr3;
    
  CResoFilter();
    
  void reset();
  float process (float sample);

  inline void set_cutoff (float v)
              {
               if (cutoff == v)
                  return;

               cutoff = v;
               calc_feedback_amount();
             };

  inline void set_resonance (float v)
              {
               if (resonance == v)
                   return;

               resonance = v;
               calc_feedback_amount();
              };

  inline void calc_feedback_amount() {feedback_amount = resonance + (float) resonance / (1.0 - cutoff);};
};


#endif // FXRESOFILTER_H
